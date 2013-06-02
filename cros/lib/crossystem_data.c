/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <gbb_header.h> /* for GoogleBinaryBlockHeader */
#include <cros/common.h>
#include <cros/crossystem_data.h>
#include <cros/cros_fdtdec.h>
#include <cros/nvstorage.h>
#include <linux/string.h>

#ifdef CONFIG_OF_LIBFDT
#include <fdtdec.h>
#include <fdt_support.h>
#include <libfdt.h>
#endif

#ifdef CONFIG_X86
#include <asm/arch-coreboot/sysinfo.h>
#endif

#define CROSSYSTEM_DATA_SIGNATURE "CHROMEOS"

/* This is used to keep bootstub and readwite main firmware in sync */
#define CROSSYSTEM_DATA_VERSION 1

enum VdatFwIndex {
	VDAT_RW_A = 0,
	VDAT_RW_B = 1,
	VDAT_RECOVERY = 0xFF
};

enum BinfFwIndex {
	BINF_RECOVERY = 0,
	BINF_RW_A = 1,
	BINF_RW_B = 2
};

static const char *__cros_fdt_get_mem_type(void)
{
	return NULL;
}

const char *cros_fdt_get_mem_type(void)
	__attribute__((weak, alias("__cros_fdt_get_mem_type")));

int crossystem_data_init(crossystem_data_t *cdata,
			 struct vboot_flag_details *write_protect_switch,
			 struct vboot_flag_details *developer_switch,
			 struct vboot_flag_details *oprom_loaded,
			 uint8_t oprom_matters,
			 uint32_t fmap_offset,
			 uint8_t active_ec_firmware,
			 uint8_t *hardware_id,
			 uint8_t *readonly_firmware_id)
{
	VBDEBUG("crossystem data at %p\n", cdata);

	memset(cdata, '\0', sizeof(*cdata));

	cdata->total_size = sizeof(*cdata);
	cdata->version = CROSSYSTEM_DATA_VERSION;
	memcpy(cdata->signature, CROSSYSTEM_DATA_SIGNATURE,
	       sizeof(CROSSYSTEM_DATA_SIGNATURE));

	cdata->boot_write_protect_switch = write_protect_switch->value;
	cdata->boot_developer_switch = developer_switch->value;
	cdata->boot_oprom_loaded = oprom_loaded->value;

	cdata->polarity_write_protect_switch =
		write_protect_switch->active_high;
	cdata->polarity_developer_switch = developer_switch->active_high;
	cdata->polarity_oprom_loaded = oprom_loaded->active_high;

	cdata->gpio_port_write_protect_switch = write_protect_switch->port;
	cdata->gpio_port_developer_switch = developer_switch->port;
	cdata->gpio_port_oprom_loaded = oprom_loaded->port;

	cdata->oprom_matters = oprom_matters;
	cdata->fmap_offset = fmap_offset;

	cdata->active_ec_firmware = active_ec_firmware;
	memcpy(cdata->hardware_id, hardware_id, sizeof(cdata->hardware_id));
	memcpy(cdata->readonly_firmware_id, readonly_firmware_id,
	       sizeof(cdata->readonly_firmware_id));

#ifdef CONFIG_ARM
	cdata->board.arm.nonvolatile_context_lba = CHROMEOS_VBNVCONTEXT_LBA;
	cdata->board.arm.nonvolatile_context_offset = 0;
	cdata->board.arm.nonvolatile_context_size = VBNV_BLOCK_SIZE;
	cdata->board.arm.nonvolatile_context_storage = nvstorage_get_type();
#endif

	return 0;
}

int crossystem_data_check_integrity(crossystem_data_t *cdata)
{
	if (cdata->total_size != sizeof(*cdata)) {
		VBDEBUG("blob size mismatch: %08x != %08x\n",
				cdata->total_size, sizeof(*cdata));
		return 1;
	}

	if (memcmp(cdata->signature, CROSSYSTEM_DATA_SIGNATURE,
				sizeof(CROSSYSTEM_DATA_SIGNATURE))) {
		VBDEBUG("invalid signature: \"%s\"\n", cdata->signature);
		return 1;
	}

	if (cdata->version != CROSSYSTEM_DATA_VERSION) {
		VBDEBUG("version mismatch: %08x != %08x\n",
				cdata->version, CROSSYSTEM_DATA_VERSION);
		return 1;
	}

	/* Okay, the crossystem data blob passes the sanity check */
	return 0;
}

int crossystem_data_set_main_firmware(crossystem_data_t *cdata,
		uint8_t firmware_type,
		uint8_t *firmware_id)
{
	cdata->firmware_type = firmware_type;
	memcpy(cdata->firmware_id, firmware_id, sizeof(cdata->firmware_id));
	return 0;
}

#ifdef CONFIG_OF_LIBFDT
static int fdt_ensure_subnode(void *fdt, int parentoffset, const char *name)
{
	int err;

	err = fdt_add_subnode(fdt, parentoffset, name);
	if (err == -FDT_ERR_EXISTS)
		return fdt_subnode_offset(fdt, parentoffset, name);

	return err;
}

static int process_cdata(crossystem_data_t *cdata, void *fdt)
{
	const char *ddr_type;
	int gpio_prop[3];
	int nodeoffset;
	int err;

#define set_scalar_prop(name, f) \
	fdt_setprop_cell(fdt, nodeoffset, name, cdata->f)
#define set_array_prop(name, f) \
	fdt_setprop(fdt, nodeoffset, name, cdata->f, sizeof(cdata->f))
#define set_conststring_prop(name, str) \
	fdt_setprop_string(fdt, nodeoffset, name, str)
#define set_bool_prop(name, f) \
	((cdata->f) ? fdt_setprop(fdt, nodeoffset, name, NULL, 0) : 0)
#define CALL(expr) \
		do { err = (expr); \
			if (err < 0) { \
				VBDEBUG("Failure at %s\n", #expr); \
				return err; \
			} \
		} while (0)
	err = 0;
	CALL(fdt_ensure_subnode(fdt, 0, "firmware"));
	nodeoffset = err;
	CALL(fdt_ensure_subnode(fdt, nodeoffset, "chromeos"));
	nodeoffset = err;
	printf("nodeoffset = %d\n", nodeoffset);

	CALL(fdt_setprop_string(fdt, nodeoffset, "compatible",
				"chromeos-firmware"));

	CALL(set_scalar_prop("total-size", total_size));
	CALL(set_array_prop("signature", signature));
	CALL(set_scalar_prop("version", version));

	CALL(set_bool_prop("boot-write-protect-switch",
			boot_write_protect_switch));
	CALL(set_bool_prop("boot-recovery-switch",
			boot_recovery_switch));
	CALL(set_bool_prop("boot-developer-switch",
			boot_developer_switch));

	gpio_prop[1] = cpu_to_fdt32(cdata->gpio_port_recovery_switch);
	gpio_prop[2] = cpu_to_fdt32(cdata->polarity_recovery_switch);
	CALL(fdt_setprop(fdt, nodeoffset, "recovery-switch",
			   gpio_prop, sizeof(gpio_prop)));

	gpio_prop[1] = cpu_to_fdt32(cdata->gpio_port_developer_switch);
	gpio_prop[2] = cpu_to_fdt32(cdata->polarity_developer_switch);
	CALL(fdt_setprop(fdt, nodeoffset, "developer-switch",
			   gpio_prop, sizeof(gpio_prop)));

	gpio_prop[1] = cpu_to_fdt32(cdata->gpio_port_oprom_loaded);
	gpio_prop[2] = cpu_to_fdt32(cdata->polarity_oprom_loaded);
	CALL(fdt_setprop(fdt, nodeoffset, "oprom-loaded",
			   gpio_prop, sizeof(gpio_prop)));

	CALL(set_scalar_prop("fmap-offset", fmap_offset));

	switch (cdata->active_ec_firmware) {
	case ACTIVE_EC_FIRMWARE_UNCHANGE: /* Default to RO */
	case ACTIVE_EC_FIRMWARE_RO:
		CALL(set_conststring_prop("active-ec-firmware", "RO"));
		break;
	case ACTIVE_EC_FIRMWARE_RW:
		CALL(set_conststring_prop("active-ec-firmware", "RW"));
		break;
	}

	switch (cdata->firmware_type) {
	case FIRMWARE_TYPE_RECOVERY:
		CALL(set_conststring_prop("firmware-type", "recovery"));
		break;
	case FIRMWARE_TYPE_NORMAL:
		CALL(set_conststring_prop("firmware-type", "normal"));
		break;
	case FIRMWARE_TYPE_DEVELOPER:
		CALL(set_conststring_prop("firmware-type", "developer"));
		break;
	}

	CALL(set_array_prop("hardware-id", hardware_id));
	CALL(set_array_prop("firmware-version", firmware_id));
	CALL(set_array_prop("readonly-firmware-version",
			readonly_firmware_id));

#ifdef CONFIG_ARM
	switch (cdata->board.arm.nonvolatile_context_storage) {
	case NONVOLATILE_STORAGE_NVRAM:
		CALL(fdt_setprop(fdt, nodeoffset,
				"nonvolatile-context-storage",
				"nvram", sizeof("nvram")));
		break;
	case NONVOLATILE_STORAGE_CROS_EC:
		CALL(fdt_setprop(fdt, nodeoffset,
				"nonvolatile-context-storage",
				"mkbp", sizeof("mkbp")));
		break;
	case NONVOLATILE_STORAGE_DISK:
		CALL(fdt_setprop(fdt, nodeoffset,
				"nonvolatile-context-storage",
				"disk", sizeof("disk")));
		CALL(set_scalar_prop("nonvolatile-context-lba",
				board.arm.nonvolatile_context_lba));
		CALL(set_scalar_prop("nonvolatile-context-offset",
				board.arm.nonvolatile_context_offset));
		CALL(set_scalar_prop("nonvolatile-context-size",
				board.arm.nonvolatile_context_size));
		break;
	default:
		VBDEBUG("Could not match nonvolatile_context_storage: %d\n",
				cdata->board.arm.nonvolatile_context_storage);
		err = 1;
	}
#endif /* CONFIG_ARM */

	CALL(set_array_prop("vboot-shared-data", vb_shared_data));

	ddr_type = cros_fdt_get_mem_type();
	if (ddr_type) {
		CALL(fdt_setprop(fdt, nodeoffset, "ddr-type", ddr_type,
				   strlen(ddr_type)));
	}

#undef set_scalar_prop
#undef set_array_prop
#undef set_conststring_prop
#undef set_bool_prop
#undef CALL

	return 0;
}

int crossystem_data_embed_into_fdt(crossystem_data_t *cdata, void *fdt)
{
	int err;

	err = process_cdata(cdata, fdt);
	if (err)
		VBDEBUG("fail to store all properties into fdt\n");

	return err;
}
#endif /* ^^^^ CONFIG_OF_LIBFDT  NOT defined ^^^^ */

#ifdef CONFIG_X86
/* TODO(sjg@chromium.org): Put this in the fdt and move x86 over to use fdt */
static int crossystem_fw_index_vdat_to_binf(int index)
{
	switch (index) {
	case VDAT_RW_A:     return BINF_RW_A;
	case VDAT_RW_B:     return BINF_RW_B;
	case VDAT_RECOVERY: return BINF_RECOVERY;
	default:            return BINF_RECOVERY;
	}
};

int crossystem_data_update_acpi(crossystem_data_t *cdata)
{
	int len;
	chromeos_acpi_t *acpi_table = (chromeos_acpi_t *)lib_sysinfo.vdat_addr;
	VbSharedDataHeader *vdat = (VbSharedDataHeader *)&acpi_table->vdat;

	acpi_table->vbt0 = BOOT_REASON_OTHER;
	acpi_table->vbt1 =
		crossystem_fw_index_vdat_to_binf(vdat->firmware_index);
	/* Use value set by coreboot if we don't want to change it */
	if (cdata->active_ec_firmware != ACTIVE_EC_FIRMWARE_UNCHANGE)
		acpi_table->vbt2 = cdata->active_ec_firmware;
	acpi_table->vbt3 =
		(cdata->boot_write_protect_switch ? CHSW_FIRMWARE_WP_DIS : 0) |
		(cdata->boot_recovery_switch ? CHSW_RECOVERY_X86 : 0) |
		(cdata->boot_developer_switch ? CHSW_DEVELOPER_SWITCH : 0);

	len = min(ID_LEN, sizeof(acpi_table->vbt4));
	memcpy(acpi_table->vbt4, cdata->hardware_id, len);
	len = min(ID_LEN, sizeof(acpi_table->vbt5));
	memcpy(acpi_table->vbt5, cdata->firmware_id, len);
	len = min(ID_LEN, sizeof(acpi_table->vbt6));
	memcpy(acpi_table->vbt6, cdata->readonly_firmware_id, len);

#ifdef CONFIG_FACTORY_IMAGE
	acpi_table->vbt7 = 3; /* '3' means 'netboot' to crossystem */
#else
	acpi_table->vbt7 = cdata->firmware_type;
#endif
	acpi_table->vbt8 = RECOVERY_REASON_NONE;
	acpi_table->vbt9 = cdata->fmap_offset;

	strncpy((char *)acpi_table->vbt10,
			(const char *)cdata->firmware_id, 64);
	return 0;
}
#endif

void crossystem_data_dump(crossystem_data_t *cdata)
{
#define _p(format, field) \
	VBDEBUG(" %-30s: " format "\n", #field, cdata->field)
	_p("%08x",	total_size);
	_p("\"%s\"",	signature);
	_p("%d",	version);

	_p("%d",	boot_write_protect_switch);
	_p("%d",	boot_recovery_switch);
	_p("%d",	boot_developer_switch);
	_p("%d",	boot_oprom_loaded);
	_p("%d",	polarity_write_protect_switch);
	_p("%d",	polarity_recovery_switch);
	_p("%d",	polarity_developer_switch);
	_p("%d",	polarity_oprom_loaded);
	_p("%d",	gpio_port_write_protect_switch);
	_p("%d",	gpio_port_recovery_switch);
	_p("%d",	gpio_port_developer_switch);
	_p("%d",	gpio_port_oprom_loaded);

	_p("%08x",	fmap_offset);

	_p("%d",	active_ec_firmware);
	_p("%d",	firmware_type);
	_p("%d",	oprom_matters);
	_p("\"%s\"",	hardware_id);
	_p("\"%s\"",	readonly_firmware_id);
	_p("\"%s\"",	firmware_id);

#ifdef CONFIG_ARM
	_p("%08llx",	board.arm.nonvolatile_context_lba);
	_p("%08x",	board.arm.nonvolatile_context_offset);
	_p("%08x",	board.arm.nonvolatile_context_size);
	_p("%08x",	board.arm.nonvolatile_context_storage);
#endif
#undef _p
}
