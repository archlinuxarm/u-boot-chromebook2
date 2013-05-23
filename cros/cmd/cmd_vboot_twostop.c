/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <cros_ec.h>
#include <fdtdec.h>
#include <lcd.h>
#include <malloc.h>
#include <linux/compiler.h>
#include <cros/boot_kernel.h>
#include <cros/common.h>
#include <cros/crossystem_data.h>
#include <cros/cros_fdtdec.h>
#include <cros/cros_init.h>
#include <cros/firmware_storage.h>
#include <cros/gbb.h>
#include <cros/hasher_state.h>
#include <cros/memory_wipe.h>
#include <cros/nvstorage.h>
#include <cros/power_management.h>
#include <cros/vboot_flag.h>
#include <linux/lzo.h>
#include <spi.h>
#ifndef CONFIG_SANDBOX
#include <usb.h>
#endif

#ifdef CONFIG_SPI_FLASH
/* to read SW write-protect settings from flash chip */
#include <spi_flash.h>
#endif

#ifdef CONFIG_VIDEO_TEGRA
/* for tegra_lcd_check_next_stage() */
#include <asm/arch/display.h>
#endif

#ifdef CONFIG_EXYNOS_DISPLAYPORT
/* for exynos_lcd_check_next_stage() */
#include <asm/arch/s5p-dp.h>
#endif

#include <gbb_header.h> /* for GoogleBinaryBlockHeader */
#include <tss_constants.h>
#include <vboot_api.h>

#ifdef CONFIG_SYS_COREBOOT
#include <asm/arch/sysinfo.h>
#endif

/*
 * The current design of twostop firmware, if we use x86 firmware design as a
 * metaphor, twostop firmware has:
 * - One bootstub that select one of the main firmware
 * - One read-only main firmware which can do recovery and normal/dev boot
 * - Two readwrite main firmware which are virtually identical to x86 readwrite
 *   firmware, that is, they only have code path to normal/dev boot
 *
 * The readwrite main firmware does not reinitialize itself (this differs to the
 * prior twostop design). As a consequence, a fixed protocol between bootstub
 * and readwrite main firmware must be defined, specifying which hardware need
 * or need not be initialized, what parameters are passed from bootstub to main
 * firmware, and etc.
 *
 * The parameters are:
 * - VbSharedData
 * - GBB
 * - Crossystem data
 * Note that the format of the parameters must be versioned so that newer
 * readwrite firmware can still work with old bootstub.
 */

/*
 * TODO The current readwrite firmware is a full-fledged U-Boot. As a
 * consequence, it will reinitialize most of the device that the bootstub
 * already initialized. We should eliminate such reinitialization not just
 * because it is slow, but also because it could be problematic.
 *
 * Given that, we must define a clear protocol specifying which device are
 * initialized by the bootstub, and which are by the readwrite firmware.
 */

DECLARE_GLOBAL_DATA_PTR;

/* The margin to keep extra stack region that not to be wiped. */
#define STACK_MARGIN		1024


/*
 * Combine VbSelectFirmware_t with VbError_t for this one file.
 * TODO(wfrichar): Clean this up, either by changing vboot or refactoring here.
 */
enum {
	/* VbSelectFirmware_t */
	TWOSTOP_SELECT_FIRMWARE_RECOVERY =    VB_SELECT_FIRMWARE_RECOVERY,
	TWOSTOP_SELECT_FIRMWARE_A        =    VB_SELECT_FIRMWARE_A,
	TWOSTOP_SELECT_FIRMWARE_B        =    VB_SELECT_FIRMWARE_B,
	TWOSTOP_SELECT_FIRMWARE_READONLY =    VB_SELECT_FIRMWARE_READONLY,
	/* More choices */
	TWOSTOP_SELECT_ERROR,
	TWOSTOP_SELECT_POWER_OFF,
	TWOSTOP_SELECT_COMMAND_LINE
};

/*
 * TODO(sjg@chromium.org)
 *
 * This is non-zero if we have read the BMP block into our gbb region.
 * This provides a work-around to allow us to read the BMP block when we
 * think it will be needed. The correct solution is to have vboot ask for
 * particular fragments of the GBB as needed. TBD.
 */
static int have_read_gbb_bmp_block;
static void *static_gbb;		/* Pointer to GBB data */
static struct twostop_fmap fmap;

int cros_cboot_twostop_read_bmp_block(void)
{
	/* Yet another use of this evil #define */
#ifndef CONFIG_HARDWARE_MAPPED_SPI

	firmware_storage_t file;
	int ret;

	if (have_read_gbb_bmp_block)
		return 0;

	if (!fmap.readonly.fmap.length &&
	    cros_fdtdec_flashmap(gd->fdt_blob, &fmap)) {
		VBDEBUG("failed to decode fmap\n");
		return -1;
	}

	if (firmware_storage_open_spi(&file)) {
		VBDEBUG("failed to open firmware storage\n");
		return -1;
	}

	ret = gbb_read_bmp_block(static_gbb, &file, fmap.readonly.gbb.offset,
				  fmap.readonly.gbb.length);
	file.close(&file);
	if (ret)
		return -1;
	have_read_gbb_bmp_block = 1;
#endif /* CONFIG_HARDWARE_MAPPED_SPI */
	return 0;
}

#if defined(VBOOT_DEBUG) || defined(DEBUG)
#define MY_ENUM_TO_STR(a) #a
static const char *
str_selection(uint32_t selection)
{
	switch (selection) {
	case TWOSTOP_SELECT_FIRMWARE_RECOVERY:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_FIRMWARE_RECOVERY);
		break;
	case TWOSTOP_SELECT_FIRMWARE_A:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_FIRMWARE_A);
		break;
	case TWOSTOP_SELECT_FIRMWARE_B:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_FIRMWARE_B);
		break;
	case TWOSTOP_SELECT_FIRMWARE_READONLY:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_FIRMWARE_READONLY);
		break;
	case TWOSTOP_SELECT_ERROR:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_ERROR);
		break;
	case TWOSTOP_SELECT_POWER_OFF:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_POWER_OFF);
		break;
	case TWOSTOP_SELECT_COMMAND_LINE:
		return MY_ENUM_TO_STR(TWOSTOP_SELECT_COMMAND_LINE);
		break;
	}
	return "<UNKNOWN>";
}
#undef MY_ENUM_TO_STR
#endif /* VBOOT_DEBUG || DEBUG */

/*
 * Implement a weak default function for boards that optionally
 * need to initialize the USB stack to detect their keyboard.
 */
int __board_use_usb_keyboard(void)
{
	/* default: no USB keyboard as primary input */
	return 0;
}
int board_use_usb_keyboard(int boot_mode)
	__attribute__((weak, alias("__board_use_usb_keyboard")));

/*
 * Check if two stop boot sequence can be interrupted. If configured - use the
 * device tree contents to determine it. Some other means (like checking the
 * environment) could be added later.
 *
 * Returns VB_INIT_FLAG_RO_NORMAL_SUPPORT if interruption is allowed or 0
 * otherwise.
 */
static int check_ro_normal_support(void)
{
	int rc = 0;
#ifdef CONFIG_OF_CONTROL
	if (cros_fdtdec_config_has_prop(gd->fdt_blob,
						"twostop-optional"))
		rc = VB_INIT_FLAG_RO_NORMAL_SUPPORT;
#endif
	VBDEBUG("%stwostop-optional\n", rc ? "" : "not ");
	return rc;
}

static int
twostop_init_cparams(struct twostop_fmap *fmap, void *gbb,
		     void *vb_shared_data, VbCommonParams *cparams)
{
	cparams->gbb_data = gbb;
	cparams->gbb_size = fmap->readonly.gbb.length;
#ifdef CONFIG_SYS_COREBOOT
	cparams->shared_data_blob =
		&((chromeos_acpi_t *)lib_sysinfo.vdat_addr)->vdat;
	cparams->shared_data_size =
		sizeof(((chromeos_acpi_t *)lib_sysinfo.vdat_addr)->vdat);
#else
	cparams->shared_data_blob = vb_shared_data;
	cparams->shared_data_size = VB_SHARED_DATA_REC_SIZE;
#endif
#define P(format, field) \
	VBDEBUG("- %-20s: " format "\n", #field, cparams->field)

	VBDEBUG("cparams:\n");
	P("%p",   gbb_data);
	P("%08x", gbb_size);
	P("%p",   shared_data_blob);
	P("%08x", shared_data_size);

#undef P

	return 0;
}

#if defined(CONFIG_OF_CONTROL) && defined(CONFIG_ARM)

#ifdef CONFIG_LCD
static int lcd_fb_size(void)
{
	return panel_info.vl_row * panel_info.vl_col *
		NBITS(panel_info.vl_bpix) / 8;
}
#endif

extern uint8_t _start;
extern uint8_t __bss_end;

static void setup_arch_unused_memory(memory_wipe_t *wipe,
	crossystem_data_t *cdata, VbCommonParams *cparams)
{
	struct fdt_memory config, ramoops, lp0;

	if (cros_fdtdec_memory(gd->fdt_blob, "/memory", &config))
		VbExError("FDT decode memory section error\n");

	memory_wipe_add(wipe, config.start, config.end);

#if !(defined(CONFIG_SYS_ICACHE_OFF) && defined(CONFIG_SYS_DCACHE_OFF))
	/* Exclude the TLB */
	memory_wipe_sub(wipe, gd->arch.tlb_addr,
			gd->arch.tlb_addr + gd->arch.tlb_size);
#endif

	/* Excludes kcrashmem if in FDT */
	if (cros_fdtdec_memory(gd->fdt_blob, "/ramoops", &ramoops))
		VBDEBUG("RAMOOPS not contained within FDT\n");
	else
		memory_wipe_sub(wipe, ramoops.start, ramoops.end);

	/* Excludes the LP0 vector; only applicable to tegra platforms */
	if (cros_fdtdec_memory(gd->fdt_blob, "/lp0", &lp0))
		VBDEBUG("LP0 not contained within FDT\n");
	else
		memory_wipe_sub(wipe, lp0.start, lp0.end);

#ifdef CONFIG_LCD
	{
		/* Excludes the frame buffer. */
		int fb_size = lcd_fb_size();

		memory_wipe_sub(wipe,
				(uintptr_t)gd->fb_base,
				(uintptr_t)gd->fb_base + fb_size);
	}
#endif
}

#elif defined(CONFIG_SYS_COREBOOT)

extern uint8_t __text_start;
extern uint8_t __bss_end;

static void setup_arch_unused_memory(memory_wipe_t *wipe,
	crossystem_data_t *cdata, VbCommonParams *cparams)
{
	int i;

	/* Add ranges that describe RAM. */
	for (i = 0; i < lib_sysinfo.n_memranges; i++) {
		struct memrange *range = &lib_sysinfo.memrange[i];
		if (range->type == CB_MEM_RAM) {
			memory_wipe_add(wipe, range->base,
				range->base + range->size);
		}
	}
	/*
	 * Remove ranges that don't. These should take precedence, so they're
	 * done last and in their own loop.
	 */
	for (i = 0; i < lib_sysinfo.n_memranges; i++) {
		struct memrange *range = &lib_sysinfo.memrange[i];
		if (range->type != CB_MEM_RAM) {
			memory_wipe_sub(wipe, range->base,
				range->base + range->size);
		}
	}
}

#else

#if !defined(CONFIG_SANDBOX)
static void setup_arch_unused_memory(memory_wipe_t *wipe,
	crossystem_data_t *cdata, VbCommonParams *cparams)
{
	/* TODO(thutt@chromium.org): add memory wipe capability */
	VBDEBUG("No memory wipe performed!");
}
#endif
#endif

#if !defined(CONFIG_SANDBOX)
static uintptr_t get_current_sp(void)
{
	uintptr_t addr;

	addr = (uintptr_t)&addr;
	return addr;
}
#endif

static void wipe_unused_memory(crossystem_data_t *cdata,
	VbCommonParams *cparams)
{
#if !defined(CONFIG_SANDBOX)
	memory_wipe_t wipe;
	int fdt_size __maybe_unused;

	fdt_size = fdt_totalsize(gd->fdt_blob);
	memory_wipe_init(&wipe);
	setup_arch_unused_memory(&wipe, cdata, cparams);

	/* Exclude relocated u-boot structures. */
	memory_wipe_sub(&wipe, get_current_sp() - STACK_MARGIN, gd->ram_top);

	/* Exclude the shared data between bootstub and main firmware. */
	memory_wipe_sub(&wipe, (uintptr_t)cdata,
			(uintptr_t)cdata + sizeof(*cdata));
	memory_wipe_sub(&wipe, (uintptr_t)cparams->gbb_data,
			(uintptr_t)cparams->gbb_data + cparams->gbb_size);

	memory_wipe_execute(&wipe);
#endif
}

/* Request the EC reboot to RO when the AP shuts down. */
static int request_ec_reboot_to_ro(void)
{
#ifdef CONFIG_CROS_EC
	struct cros_ec_dev *mdev = board_get_cros_ec_dev();

	if (!mdev) {
		VBDEBUG("%s: no cro_ec device: cannot request EC reboot to RO\n",
			__func__);
		return -1;
	}

	return cros_ec_reboot(mdev, EC_REBOOT_COLD,
			   EC_REBOOT_FLAG_ON_AP_SHUTDOWN);
#else
	return 0;
#endif
}

static int flash_sw_wp_is_enabled(firmware_storage_t *file)
{
	uint8_t yes_it_is = 0;

#ifdef CONFIG_SPI_FLASH
	int r = 0;
	r = spi_flash_read_sw_wp_status(file->context, &yes_it_is);
	if (r) {
		VBDEBUG("spi_flash_read_sw_wp_status() failed: %d\n", r);
		return 0;
	}
#endif

	VBDEBUG("flash SW WP is %d\n", yes_it_is);
	return yes_it_is;
}



/* Fill in active EC firmware information. */
static int set_active_ec_firmware(crossystem_data_t* cdata)
{
	int in_rw = 0;
	int rv;

	/* If software sync is disabled, just leave this as original value. */
	if (!cros_fdtdec_config_has_prop(gd->fdt_blob, "ec-software-sync")) {
		cdata->active_ec_firmware = ACTIVE_EC_FIRMWARE_UNCHANGE;
		return 0;
	}

	rv = VbExEcRunningRW(&in_rw);
	if (rv != VBERROR_SUCCESS)
		return rv;
	cdata->active_ec_firmware = (in_rw ? ACTIVE_EC_FIRMWARE_RW :
					     ACTIVE_EC_FIRMWARE_RO);
	return 0;
}

static VbError_t
twostop_init_vboot_library(firmware_storage_t *file, void *gbb,
			   uint32_t gbb_offset, size_t gbb_size,
			   crossystem_data_t *cdata, VbCommonParams *cparams)
{
	VbError_t err;
	VbInitParams iparams;
	int virtual_dev_switch =
		cros_fdtdec_config_has_prop(gd->fdt_blob,
					    "virtual-dev-switch");
#ifdef CONFIG_CROS_EC
	struct cros_ec_dev *mdev = board_get_cros_ec_dev();
#endif

	memset(&iparams, 0, sizeof(iparams));
	iparams.flags = check_ro_normal_support();

#ifdef CONFIG_CROS_EC
	if (mdev) {
		uint32_t ec_events = 0;
		const uint32_t kb_rec_mask =
			EC_HOST_EVENT_MASK(EC_HOST_EVENT_KEYBOARD_RECOVERY);

		/* Read keyboard recovery flag from EC, then clear it */
		if (cros_ec_get_host_events(mdev, &ec_events)) {
			/*
			 * TODO: what can we do if that fails?  Request
			 * recovery?  We don't simply want to fail, because
			 * that'll prevent us from going into recovery mode.
			 * We don't want to go into recovery mode
			 * automatically, because that'll break snow.
			 */
			VBDEBUG("VbInit: unable to read EC events\n");
			ec_events = 0;
		}
		if (ec_events & kb_rec_mask) {
			iparams.flags |= VB_INIT_FLAG_REC_BUTTON_PRESSED;
			if (cros_ec_clear_host_events(mdev, kb_rec_mask))
				VBDEBUG("VbInit: unable to clear "
					"EC KB recovery event\n");
		}
	}
#endif

	if (cdata->boot_write_protect_switch)
		iparams.flags |= VB_INIT_FLAG_WP_ENABLED;
	if (cdata->boot_recovery_switch)
		iparams.flags |= VB_INIT_FLAG_REC_BUTTON_PRESSED;
	if (cdata->boot_developer_switch)
		iparams.flags |= VB_INIT_FLAG_DEV_SWITCH_ON;
	if (cdata->boot_oprom_loaded)
		iparams.flags |= VB_INIT_FLAG_OPROM_LOADED;
	if (cdata->oprom_matters)
		iparams.flags |= VB_INIT_FLAG_OPROM_MATTERS;
	if (virtual_dev_switch)
		iparams.flags |= VB_INIT_FLAG_VIRTUAL_DEV_SWITCH;
	if (cros_fdtdec_config_has_prop(gd->fdt_blob, "ec-software-sync"))
		iparams.flags |= VB_INIT_FLAG_EC_SOFTWARE_SYNC;
	if (cros_fdtdec_config_has_prop(gd->fdt_blob, "ec-slow-update"))
		iparams.flags |= VB_INIT_FLAG_EC_SLOW_UPDATE;
	if (flash_sw_wp_is_enabled(file))
		iparams.flags |= VB_INIT_FLAG_SW_WP_ENABLED;
	VBDEBUG("iparams.flags: %08x\n", iparams.flags);

	if ((err = VbInit(cparams, &iparams))) {
		VBDEBUG("VbInit: %u\n", err);

		/*
		 * If vboot wants EC to reboot to RO, make request now,
		 * because there isn't a clear path to pass this request
		 * through to do_vboot_twostop().
		 */
		if (err == VBERROR_EC_REBOOT_TO_RO_REQUIRED)
			request_ec_reboot_to_ro();

		return err;
	}

#ifdef CONFIG_VIDEO_TEGRA
	tegra_lcd_check_next_stage(gd->fdt_blob, 0);
#endif
#ifdef CONFIG_EXYNOS_DISPLAYPORT
	exynos_lcd_check_next_stage(gd->fdt_blob, 0);
#endif
	VBDEBUG("iparams.out_flags: %08x\n", iparams.out_flags);

	if (virtual_dev_switch) {
		cdata->boot_developer_switch =
			(iparams.out_flags & VB_INIT_OUT_ENABLE_DEVELOPER) ?
			1 : 0;
		VBDEBUG("cdata->boot_developer_switch=%d\n",
				cdata->boot_developer_switch);
	}

	if (iparams.out_flags & VB_INIT_OUT_CLEAR_RAM)
		wipe_unused_memory(cdata, cparams);

	/* Load required information of GBB */
	if (iparams.out_flags & VB_INIT_OUT_ENABLE_DISPLAY) {
		if (gbb_read_bmp_block(gbb, file, gbb_offset, gbb_size))
			return VBERROR_INVALID_GBB;
		have_read_gbb_bmp_block = 1;
	}
	if (cdata->boot_developer_switch ||
			iparams.out_flags & VB_INIT_OUT_ENABLE_RECOVERY) {
		if (gbb_read_recovery_key(gbb, file, gbb_offset, gbb_size))
			return VBERROR_INVALID_GBB;
	}

	return VBERROR_SUCCESS;
}

static uint32_t
twostop_make_selection(struct twostop_fmap *fmap, firmware_storage_t *file,
		       VbCommonParams *cparams, void **fw_blob_ptr,
		       uint32_t *fw_size_ptr,
		       struct fmap_firmware_entry **entryp)
{
	uint32_t selection = TWOSTOP_SELECT_ERROR;
#if !defined(CONFIG_SANDBOX)
	VbError_t err;
#endif
	uint32_t vlength;
	VbSelectFirmwareParams fparams;
	hasher_state_t s;

	*entryp = NULL;
	memset(&fparams, '\0', sizeof(fparams));

	vlength = fmap->readwrite_a.vblock.length;
	assert(vlength == fmap->readwrite_b.vblock.length);

	fparams.verification_size_A = fparams.verification_size_B = vlength;

#ifndef CONFIG_HARDWARE_MAPPED_SPI
	fparams.verification_block_A = cros_memalign_cache(vlength);
	if (!fparams.verification_block_A) {
		VBDEBUG("failed to allocate vblock A\n");
		goto out;
	}
	fparams.verification_block_B = cros_memalign_cache(vlength);
	if (!fparams.verification_block_B) {
		VBDEBUG("failed to allocate vblock B\n");
		goto out;
	}
#endif
	if (file->read(file, fmap->readwrite_a.vblock.offset, vlength,
				BT_EXTRA fparams.verification_block_A)) {
		VBDEBUG("fail to read vblock A\n");
		goto out;
	}
	if (file->read(file, fmap->readwrite_b.vblock.offset, vlength,
				BT_EXTRA fparams.verification_block_B)) {
		VBDEBUG("fail to read vblock B\n");
		goto out;
	}

	s.fw[0].vblock = fparams.verification_block_A;
	s.fw[1].vblock = fparams.verification_block_B;

	s.fw[0].offset = fmap->readwrite_a.boot.offset;
	s.fw[1].offset = fmap->readwrite_b.boot.offset;

	s.fw[0].size = fmap->readwrite_a.boot.length;
	s.fw[1].size = fmap->readwrite_b.boot.length;

#ifndef CONFIG_HARDWARE_MAPPED_SPI
	s.fw[0].cache = cros_memalign_cache(s.fw[0].size);
	if (!s.fw[0].cache) {
		VBDEBUG("failed to allocate cache A\n");
		goto out;
	}
	s.fw[1].cache = cros_memalign_cache(s.fw[1].size);
	if (!s.fw[1].cache) {
		VBDEBUG("failed to allocate cache B\n");
		goto out;
	}
#endif

	s.file = file;
	cparams->caller_context = &s;

#if defined(CONFIG_SANDBOX)
	fparams.verification_block_A = NULL;
	fparams.verification_size_A = 0;
	fparams.verification_block_B = NULL;
	fparams.verification_size_B = 0;
	fparams.selected_firmware = VB_SELECT_FIRMWARE_A;
#else
	if ((err = VbSelectFirmware(cparams, &fparams))) {
		VBDEBUG("VbSelectFirmware: %d\n", err);

		/*
		 * If vboot wants EC to reboot to RO, make request now,
		 * because there isn't a clear path to pass this request
		 * through to do_vboot_twostop().
		 */
		if (err == VBERROR_EC_REBOOT_TO_RO_REQUIRED)
			request_ec_reboot_to_ro();

		goto out;
	}
#endif
	VBDEBUG("selected_firmware: %d\n", fparams.selected_firmware);
	selection = fparams.selected_firmware;

out:

	FREE_IF_NEEDED(fparams.verification_block_A);
	FREE_IF_NEEDED(fparams.verification_block_B);

	if (selection == VB_SELECT_FIRMWARE_A) {
		uint32_t offset = fmap->readwrite_a.boot_rwbin.offset -
			fmap->readwrite_a.boot.offset;
		*fw_blob_ptr = s.fw[0].cache + offset;
		*fw_size_ptr = s.fw[0].size - offset;
		*entryp = &fmap->readwrite_a;
		FREE_IF_NEEDED(s.fw[1].cache);
	} else if (selection == VB_SELECT_FIRMWARE_B) {
		uint32_t offset = fmap->readwrite_b.boot_rwbin.offset -
			fmap->readwrite_b.boot.offset;
		*fw_blob_ptr = s.fw[1].cache + offset;
		*fw_size_ptr = s.fw[1].size - offset;
		*entryp = &fmap->readwrite_b;
		FREE_IF_NEEDED(s.fw[0].cache);
	}

	return selection;
}

static uint32_t
twostop_select_and_set_main_firmware(struct twostop_fmap *fmap,
				     firmware_storage_t *file, void *gbb,
				     size_t gbb_size, crossystem_data_t *cdata,
				     void *vb_shared_data, int *boot_mode,
				     void **fw_blob_ptr, uint32_t *fw_size_ptr,
				     struct fmap_firmware_entry **entryp)
{
	uint32_t selection;
	uint32_t id_offset = 0, id_length = 0;
	int firmware_type;
	struct fmap_firmware_entry *entry;
#ifndef CONFIG_HARDWARE_MAPPED_SPI
	uint8_t firmware_id[ID_LEN];
#else
	uint8_t *firmware_id;
#endif
	VbCommonParams cparams;

	*entryp = NULL;
	bootstage_mark_name(BOOTSTAGE_VBOOT_SELECT_AND_SET,
			"twostop_select_and_set_main_firmware");
	if (twostop_init_cparams(fmap, gbb, vb_shared_data, &cparams)) {
		VBDEBUG("failed to init cparams\n");
		return TWOSTOP_SELECT_ERROR;
	}

	if (twostop_init_vboot_library(file, gbb, fmap->readonly.gbb.offset,
				       gbb_size, cdata, &cparams)
			!= VBERROR_SUCCESS) {
		VBDEBUG("failed to init vboot library\n");
		return TWOSTOP_SELECT_ERROR;
	}

	/*
	 * TODO(sjg@chromium.org): Ick. Should unify readwrte_a/b and
	 * readonly, and then we can use entry for all purposee.
	 */
	selection = twostop_make_selection(fmap, file, &cparams,
			fw_blob_ptr, fw_size_ptr, &entry);

	VBDEBUG("selection: %s\n", str_selection(selection));

	if (selection == TWOSTOP_SELECT_ERROR)
		return TWOSTOP_SELECT_ERROR;

	switch(selection) {
	case VB_SELECT_FIRMWARE_RECOVERY:
	case VB_SELECT_FIRMWARE_READONLY:
		id_offset = fmap->readonly.firmware_id.offset;
		id_length = fmap->readonly.firmware_id.length;
		break;
	case VB_SELECT_FIRMWARE_A:
	case VB_SELECT_FIRMWARE_B:
		id_offset = entry->firmware_id.offset;
		id_length = entry->firmware_id.length;
		break;
	default:
		VBDEBUG("impossible selection value: %d\n", selection);
		assert(0);
	}

	if (file->read(file, id_offset,
				MIN(sizeof(firmware_id), id_length),
				BT_EXTRA firmware_id)) {
		VBDEBUG("failed to read active firmware id\n");
		firmware_id[0] = '\0';
	}

	if (selection == VB_SELECT_FIRMWARE_RECOVERY)
		firmware_type = FIRMWARE_TYPE_RECOVERY;
	else if (cdata->boot_developer_switch)
		firmware_type = FIRMWARE_TYPE_DEVELOPER;
	else
		firmware_type = FIRMWARE_TYPE_NORMAL;

	*boot_mode = firmware_type;

	VBDEBUG("active main firmware type : %d\n", firmware_type);
	VBDEBUG("active main firmware id   : \"%s\"\n", firmware_id);

	if (crossystem_data_set_main_firmware(cdata,
				firmware_type, firmware_id)) {
		VBDEBUG("failed to set active main firmware\n");
		return TWOSTOP_SELECT_ERROR;
	}
	*entryp = entry;

	return selection;
}

#if !defined(CONFIG_SANDBOX)
static uint32_t
twostop_jump(crossystem_data_t *cdata, void *fw_blob, uint32_t fw_size,
	     struct fmap_firmware_entry *entry)
{
	void *dest = (void *)CONFIG_SYS_TEXT_BASE;

	VBDEBUG("jump to readwrite main firmware at %#x, pos %p, size %#x\n",
			dest, fw_blob, fw_size);

	/*
	 * TODO: This version of U-Boot must be loaded at a fixed location. It
	 * could be problematic if newer version U-Boot changed this address.
	 */
	switch (entry->compress) {
#ifdef CONFIG_LZO
	case CROS_COMPRESS_LZO: {
		uint unc_len;
		int ret;

		bootstage_start(BOOTSTAGE_ID_ACCUM_DECOMP, "decompress_image");
		ret = lzop_decompress(fw_blob, fw_size, dest, &unc_len);
		if (ret < 0) {
			VBDEBUG("LZO: uncompress or overwrite error %d "
				"- must RESET board to recover\n", ret);
			return TWOSTOP_SELECT_ERROR;
		}
		bootstage_accum(BOOTSTAGE_ID_ACCUM_DECOMP);
		break;
	}
#endif
	case CROS_COMPRESS_NONE:
		memmove(dest, fw_blob, fw_size);
		break;
	default:
		VBDEBUG("Unsupported compression type %d\n", entry->compress);
		return TWOSTOP_SELECT_ERROR;
	}

	/*
	 * TODO We need to reach the Point of Unification here, but I am not
	 * sure whether the following function call flushes L2 cache or not. If
	 * it does, we should avoid that.
	 */
	cleanup_before_linux();

	((void(*)(void))CONFIG_SYS_TEXT_BASE)();

	/* It is an error if readwrite firmware returns */
	return TWOSTOP_SELECT_ERROR;
}
#endif

static int
twostop_init(struct twostop_fmap *fmap, firmware_storage_t *file,
	     void **gbbp, size_t gbb_size, crossystem_data_t *cdata,
	     void *vb_shared_data)
{
	struct vboot_flag_details wpsw, devsw, oprom;
	GoogleBinaryBlockHeader *gbbh;
	uint8_t hardware_id[ID_LEN];
#ifndef CONFIG_HARDWARE_MAPPED_SPI
	uint8_t  readonly_firmware_id[ID_LEN];
#else
	uint8_t *readonly_firmware_id;
#endif
	int oprom_matters = 0;
	int ret = -1;
	void *gbb;

	bootstage_mark_name(BOOTSTAGE_VBOOT_TWOSTOP_INIT, "twostop_init");
	if (vboot_flag_fetch(VBOOT_FLAG_WRITE_PROTECT, &wpsw) ||
	    vboot_flag_fetch(VBOOT_FLAG_DEVELOPER, &devsw) ||
	    vboot_flag_fetch(VBOOT_FLAG_OPROM_LOADED, &oprom)) {
		VBDEBUG("failed to fetch gpio\n");
		return -1;
	}
	vboot_flag_dump(VBOOT_FLAG_WRITE_PROTECT, &wpsw);
	vboot_flag_dump(VBOOT_FLAG_DEVELOPER, &devsw);
	vboot_flag_dump(VBOOT_FLAG_OPROM_LOADED, &oprom);

	if (cros_fdtdec_config_has_prop(gd->fdt_blob, "oprom-matters")) {
		VBDEBUG("FDT says oprom-matters\n");
		oprom_matters = 1;
	}

	if (!fmap->readonly.fmap.length &&
	    cros_fdtdec_flashmap(gd->fdt_blob, fmap)) {
		VBDEBUG("failed to decode fmap\n");
		return -1;
	}
	dump_fmap(fmap);

	/* We revert the decision of using firmware_storage_open_twostop() */
	if (firmware_storage_open_spi(file)) {
		VBDEBUG("failed to open firmware storage\n");
		return -1;
	}

					/* Read read-only firmware ID */
	if (file->read(file, fmap->readonly.firmware_id.offset,
		       MIN(sizeof(readonly_firmware_id),
			   fmap->readonly.firmware_id.length),
		       BT_EXTRA readonly_firmware_id)) {
		VBDEBUG("failed to read firmware ID\n");
		readonly_firmware_id[0] = '\0';
	}
	VBDEBUG("read-only firmware id: \"%s\"\n", readonly_firmware_id);

					/* Load basic parts of gbb blob */
#ifdef CONFIG_HARDWARE_MAPPED_SPI
	if (gbb_init(gbbp, file, fmap->readonly.gbb.offset, gbb_size)) {
		VBDEBUG("failed to read gbb\n");
		goto out;
	}
	gbb = *gbbp;
#else
	gbb = *gbbp;
	if (gbb_init(gbb, file, fmap->readonly.gbb.offset, gbb_size)) {
		VBDEBUG("failed to read gbb\n");
		goto out;
	}
#endif

	gbbh = (GoogleBinaryBlockHeader *)gbb;
	memcpy(hardware_id, gbb + gbbh->hwid_offset,
	       MIN(sizeof(hardware_id), gbbh->hwid_size));
	VBDEBUG("hardware id: \"%s\"\n", hardware_id);

	/* Initialize crossystem data */
	/*
	 * TODO There is no readwrite EC firmware on our current ARM boards. But
	 * we should have a mechanism to probe (or acquire this information from
	 * the device tree) whether the active EC firmware is R/O or R/W.
	 */
	if (crossystem_data_init(cdata,
				 &wpsw, &devsw, &oprom,
				 oprom_matters,
				 fmap->readonly.fmap.offset,
				 ACTIVE_EC_FIRMWARE_RO,
				 hardware_id,
				 readonly_firmware_id)) {
		VBDEBUG("failed to init crossystem data\n");
		goto out;
	}

	ret = 0;
#ifdef CONFIG_VIDEO_TEGRA
	tegra_lcd_check_next_stage(gd->fdt_blob, 0);
#endif
#ifdef CONFIG_EXYNOS_DISPLAYPORT
	exynos_lcd_check_next_stage(gd->fdt_blob, 0);
#endif

out:
	if (ret)
		file->close(file);

	return ret;
}

static uint32_t
twostop_main_firmware(struct twostop_fmap *fmap, void *gbb,
		      crossystem_data_t *cdata, void *vb_shared_data)
{
	VbError_t err;
	VbSelectAndLoadKernelParams kparams;
	VbCommonParams cparams;
	size_t size = 0;

#ifdef CONFIG_BOOTSTAGE_STASH
	bootstage_unstash((void *)CONFIG_BOOTSTAGE_STASH,
			CONFIG_BOOTSTAGE_STASH_SIZE);
#endif
	bootstage_mark_name(BOOTSTAGE_VBOOT_TWOSTOP_MAIN_FIRMWARE,
			"twostop_main_firmware");
	if (twostop_init_cparams(fmap, gbb, vb_shared_data, &cparams)) {
		VBDEBUG("failed to init cparams\n");
		return TWOSTOP_SELECT_ERROR;
	}

	/*
	 * Note that in case "kernel" is not found in the device tree, the
	 * "size" value is going to remain unchanged.
	 */
	kparams.kernel_buffer = cros_fdtdec_alloc_region(gd->fdt_blob,
		"kernel", &size);
	kparams.kernel_buffer_size = size;

	VBDEBUG("kparams:\n");
	VBDEBUG("- kernel_buffer:      : %p\n", kparams.kernel_buffer);
	VBDEBUG("- kernel_buffer_size: : %08x\n",
			kparams.kernel_buffer_size);

#ifdef CONFIG_EXYNOS_DISPLAYPORT
	exynos_lcd_check_next_stage(gd->fdt_blob, 0);
#endif

	if ((err = VbSelectAndLoadKernel(&cparams, &kparams))) {
		VBDEBUG("VbSelectAndLoadKernel: %d\n", err);
		switch (err) {
		case VBERROR_SHUTDOWN_REQUESTED:
			return TWOSTOP_SELECT_POWER_OFF;
		case VBERROR_BIOS_SHELL_REQUESTED:
			return TWOSTOP_SELECT_COMMAND_LINE;
		case VBERROR_EC_REBOOT_TO_RO_REQUIRED:
			request_ec_reboot_to_ro();
			return TWOSTOP_SELECT_POWER_OFF;
		}
		return TWOSTOP_SELECT_ERROR;
	}

	VBDEBUG("kparams:\n");
	VBDEBUG("- kernel_buffer:      : %p\n", kparams.kernel_buffer);
	VBDEBUG("- kernel_buffer_size: : %08x\n",
			kparams.kernel_buffer_size);
	VBDEBUG("- disk_handle:        : %p\n", kparams.disk_handle);
	VBDEBUG("- partition_number:   : %08x\n",
			kparams.partition_number);
	VBDEBUG("- bootloader_address: : %08llx\n",
			kparams.bootloader_address);
	VBDEBUG("- bootloader_size:    : %08x\n",
			kparams.bootloader_size);
	VBDEBUG("- partition_guid:     :");
#ifdef VBOOT_DEBUG
	int i;
	for (i = 0; i < 16; i++)
		VbExDebug(" %02x", kparams.partition_guid[i]);
	VbExDebug("\n");
#endif /* VBOOT_DEBUG */

	/* EC might jump between RO and RW during software sync. We need to
	 * update active EC copy in cdata. */
	set_active_ec_firmware(cdata);
	crossystem_data_dump(cdata);
#if defined(CONFIG_SANDBOX)
	return TWOSTOP_SELECT_COMMAND_LINE;
#else
	boot_kernel(&kparams, cdata);

	/* It is an error if boot_kenel returns */
	return TWOSTOP_SELECT_ERROR;
#endif
}

/**
 * Get address of the cdata (and gbb, if not mapping SPI flash directly), and
 * optionally verify them.
 *
 * @param gbb returns pointer to GBB when SPI flash is not mapped directly.
 *            Contains pointer to gbb otherwise.
 * @param cdata		returns pointer to crossystem data
 * @param verify	1 to verify data, 0 to skip this step
 * @return 0 if ok, -1 on error
 */
static int setup_gbb_and_cdata(void **gbb, size_t *gbb_size,
			       crossystem_data_t **cdata, int verify)
{
	size_t size;

#ifndef CONFIG_HARDWARE_MAPPED_SPI
	*gbb = cros_fdtdec_alloc_region(gd->fdt_blob,
			"google-binary-block", gbb_size);

	if (!*gbb) {
		VBDEBUG("google-binary-block missing "
			"from fdt, or malloc failed\n");
		return -1;
	}

#endif
	*cdata = cros_fdtdec_alloc_region(gd->fdt_blob,
						  "cros-system-data", &size);
	if (!*cdata) {
		VBDEBUG("cros-system-data missing "
				"from fdt, or malloc failed\n");
		return -1;
	}

	/*
	 * TODO(clchiou): readwrite firmware should check version of the data
	 * blobs
	 */
	if (verify && crossystem_data_check_integrity(*cdata)) {
		VBDEBUG("invalid crossystem data\n");
		return -1;
	}

	if (verify && gbb_check_integrity(*gbb)) {
		VBDEBUG("invalid gbb at %p\n", *gbb);
		return -1;
	}
	return 0;
}

static uint32_t
twostop_boot(int stop_at_select)
{
	firmware_storage_t file;
	crossystem_data_t *cdata = NULL;
	void *gbb;
	size_t gbb_size = 0;
	void *vb_shared_data;
	void *fw_blob = NULL;
	uint32_t fw_size = 0;
	uint32_t selection;
	int boot_mode = FIRMWARE_TYPE_NORMAL;
	struct fmap_firmware_entry *entry;

	if (setup_gbb_and_cdata(&gbb, &gbb_size, &cdata, 0))
		return TWOSTOP_SELECT_ERROR;

	vb_shared_data = cdata->vb_shared_data;
	if (twostop_init(&fmap, &file, &gbb, gbb_size, cdata,
			 vb_shared_data)) {
		VBDEBUG("failed to init twostop boot\n");
		return TWOSTOP_SELECT_ERROR;
	}
	static_gbb = gbb;

	selection = twostop_select_and_set_main_firmware(&fmap, &file,
			gbb, gbb_size, cdata, vb_shared_data,
			&boot_mode, &fw_blob, &fw_size, &entry);
	VBDEBUG("selection of bootstub: %s\n", str_selection(selection));

	file.close(&file); /* We don't care even if it fails */

	if (stop_at_select)
		return selection;

	/* Don't we bother to free(fw_blob) if there was an error? */
#if !defined(CONFIG_SANDBOX)
	if (selection == TWOSTOP_SELECT_ERROR)
		return TWOSTOP_SELECT_ERROR;

	if (selection == VB_SELECT_FIRMWARE_A ||
	    selection == VB_SELECT_FIRMWARE_B)
		return twostop_jump(cdata, fw_blob, fw_size, entry);

	assert(selection == VB_SELECT_FIRMWARE_READONLY ||
	       selection == VB_SELECT_FIRMWARE_RECOVERY);
#endif
	/*
	 * TODO: Now, load drivers for rec/normal/dev main firmware.
	 */
#ifdef CONFIG_USB_KEYBOARD
	if (board_use_usb_keyboard(boot_mode)) {
		int cnt;
		/* enumerate USB devices to find the keyboard */
		cnt = usb_init();
		if (cnt >= 0)
			drv_usb_kbd_init();
	}
#endif

	VBDEBUG("boot_mode: %d\n", boot_mode);

	selection = twostop_main_firmware(&fmap, gbb, cdata, vb_shared_data);
	VBDEBUG("selection of read-only main firmware: %s\n",
			str_selection(selection));

	if (selection != TWOSTOP_SELECT_COMMAND_LINE)
		return selection;

	/*
	 * TODO: Now, load all other drivers, such as networking, as we are
	 * returning back to the command line.
	 */

	return TWOSTOP_SELECT_COMMAND_LINE;
}

static uint32_t
twostop_readwrite_main_firmware(void)
{
	crossystem_data_t *cdata;
	void *gbb;
	size_t gbb_size;

	if (!fmap.readonly.fmap.length &&
	    cros_fdtdec_flashmap(gd->fdt_blob, &fmap)) {
		VBDEBUG("failed to decode fmap\n");
		return TWOSTOP_SELECT_ERROR;
	}
	dump_fmap(&fmap);

#ifdef CONFIG_HARDWARE_MAPPED_SPI
	gbb = (void *) (fmap.readonly.gbb.offset + fmap.flash_base);
#endif
	if (setup_gbb_and_cdata(&gbb, &gbb_size, &cdata, 1))
		return TWOSTOP_SELECT_ERROR;
	static_gbb = gbb;

#ifdef CONFIG_ARM
	uint8_t ro_nvtype = cdata->board.arm.nonvolatile_context_storage;

	/*
	 * Default to disk for older RO firmware which does not provide
	 * storage type.
	 */
	if (ro_nvtype == NONVOLATILE_STORAGE_NONE)
		ro_nvtype = NONVOLATILE_STORAGE_DISK;
	if (nvstorage_set_type(ro_nvtype))
		return TWOSTOP_SELECT_ERROR;
	cdata->board.arm.nonvolatile_context_storage = ro_nvtype;
#endif /* CONFIG_ARM */

	/*
	 * VbSelectAndLoadKernel() assumes the TPM interface has already been
	 * initialized by VbSelectFirmware(). Since we haven't called
	 * VbSelectFirmware() in the readwrite firmware, we need to explicitly
	 * initialize the TPM interface. Note that this only re-initializes the
	 * interface, not the TPM itself.
	 */
	if (VbExTpmInit() != TPM_SUCCESS) {
		VBDEBUG("failed to init tpm interface\n");
		return TWOSTOP_SELECT_ERROR;
	}

	/* TODO Now, initialize device that bootstub did not initialize */

	return twostop_main_firmware(&fmap, gbb, cdata, cdata->vb_shared_data);
}

/* FIXME(wfrichar): Work in progress. crosbug.com/p/11215 */
/* Write-protect portions of the RW flash until the next boot. */
VbError_t VbExProtectFlash(enum VbProtectFlash_t region)
{
#ifdef CONFIG_CAN_PROTECT_RW_FLASH
	switch (region) {
	case VBPROTECT_RW_A:
		VBDEBUG("VBPROTECT_RW_A => 0x%08x 0x%x\n",
			fmap.readwrite_a.all.offset,
			fmap.readwrite_a.all.length);
		spi_write_protect_region(fmap.readwrite_a.all.offset,
					 fmap.readwrite_a.all.length, 0);
		break;
	case VBPROTECT_RW_B:
		VBDEBUG("VBPROTECT_RW_B => 0x%08x 0x%x\n",
			fmap.readwrite_b.all.offset,
			fmap.readwrite_b.all.length);
		spi_write_protect_region(fmap.readwrite_b.all.offset,
					 fmap.readwrite_b.all.length, 0);
		break;
	case VBPROTECT_RW_DEVKEY:
		VBDEBUG("VBPROTECT_RW_DEVKEY => 0x%08x 0x%x\n",
			fmap.readwrite_devkey.offset,
			fmap.readwrite_devkey.length);
		spi_write_protect_region(fmap.readwrite_devkey.offset,
					 fmap.readwrite_devkey.length, 1);
		break;
	default:
		VBDEBUG("unknown region %d\n", region);
		return VBERROR_INVALID_PARAMETER;
	}
	return VBERROR_SUCCESS;
#else
	VBDEBUG("%s not implemented on this platform\n", __func__);
	return VBERROR_UNKNOWN;
#endif
}

static int
do_vboot_twostop(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t selection;
	int ro_firmware;

	bootstage_mark_name(BOOTSTAGE_VBOOT_TWOSTOP, "do_vboot_twostop");

	/*
	 * Empty keyboard buffer before boot.  In case EC did not clear its
	 * buffer between power cycles, this prevents vboot of current power
	 * cycle being affected by keystrokes of previous power cycle.
	 */
	while (tstc())
		getc();

	if (cros_init()) {
		VBDEBUG("fail to init cros library\n");
		goto on_error;
	}

	/*
	 * TODO: We should clear screen later if we load graphics optionally.
	 * In normal mode, we don't need to load graphics driver and clear
	 * screen.
	 */
	display_clear();

	/*
	 * TODO(sjg@chromium.org): root cause issue crosbug.com/p/11075
	 *
	 * Ensure there are no keys in the keyboard buffer, so that we don't
	 * accidentally see a space key and go into recovery mode.
	 */
	while (tstc())
		(void)getc();

	/*
	 * A processor reset jumps to the reset entry point (which is the
	 * read-only firmware), otherwise we have entered U-Boot from a
	 * software jump.
	 *
	 * Note: If a read-only firmware is loaded to memory not because of a
	 * processor reset, this instance of read-only firmware should go to the
	 * readwrite firmware code path.
	 */
	ro_firmware = is_processor_reset();
	VBDEBUG("Starting %s firmware\n", ro_firmware ? "read-only" :
			"read-write");
	if (ro_firmware)
		selection = twostop_boot(0);
	else
		selection = twostop_readwrite_main_firmware();

	VBDEBUG("selection of main firmware: %s\n",
			str_selection(selection));

	if (selection == TWOSTOP_SELECT_COMMAND_LINE)
		return 0;

	if (selection == TWOSTOP_SELECT_POWER_OFF)
		power_off();

	assert(selection == TWOSTOP_SELECT_ERROR);

on_error:
	cold_reboot();
	return 0;
}

U_BOOT_CMD(vboot_twostop, 1, 1, do_vboot_twostop,
		"verified boot twostop firmware", NULL);

static int
do_vboot_load_oprom(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t selection;
	struct vboot_flag_details oprom;

	if (cros_init()) {
		VBDEBUG("fail to init cros library\n");
		return -1;
	}

	/* We should be in RO now. */
	if (!is_processor_reset()) {
		VBDEBUG("This command should only be executed in RO.\n");
		return -1;
	}

	if (!cros_fdtdec_config_has_prop(gd->fdt_blob, "oprom-matters")) {
		VBDEBUG("FDT doesn't say oprom-matters.\n");
		return -1;
	}

	if (vboot_flag_fetch(VBOOT_FLAG_OPROM_LOADED, &oprom)) {
		VBDEBUG("Failed to fetch OPROM gpio\n");
		return -1;
	}

	vboot_flag_dump(VBOOT_FLAG_OPROM_LOADED, &oprom);
	if (oprom.value) {
		VBDEBUG("OPROM already loaded\n");
		return 0;
	}

	/*
	 * Initialize necessary data and stop at firmware selection. If
	 * OPROM is not loaded and is needed, we should get an error here.
	 */
	selection = twostop_boot(1);

	if (selection == TWOSTOP_SELECT_ERROR) {
		cold_reboot();
		return 0;
	} else {
		VBDEBUG("Vboot doesn't say we need OPROM.\n");
		return -1;
	}
}

U_BOOT_CMD(vboot_load_oprom, 1, 1, do_vboot_load_oprom,
	   "load oprom if it is needed", NULL);
