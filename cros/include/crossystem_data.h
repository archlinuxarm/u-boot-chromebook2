/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __CHROMEOS_CROSSYSTEM_DATA_H__
#define __CHROMEOS_CROSSYSTEM_DATA_H__

#include <cros/vboot_flag.h>
#include <vboot_nvstorage.h>
#include <vboot_struct.h>

#define ID_LEN		256

enum {
	ACTIVE_EC_FIRMWARE_RO = 0,
	ACTIVE_EC_FIRMWARE_RW = 1,
	ACTIVE_EC_FIRMWARE_UNCHANGE = 2, /* Use value set by coreboot */
};

enum {
	FIRMWARE_TYPE_RECOVERY	= 0,
	FIRMWARE_TYPE_NORMAL	= 1,
	FIRMWARE_TYPE_DEVELOPER	= 2
};

enum {
	BOOT_REASON_OTHER  = 0,
	BOOT_REASON_S3DIAG = 9
};

enum {
	CHSW_RECOVERY_X86     = (1 << 1),
	CHSW_RECOVERY_EC      = (1 << 2),
	CHSW_DEVELOPER_SWITCH = (1 << 5),
	CHSW_FIRMWARE_WP_DIS  = (1 << 9)
};

enum {
	ACTIVE_MAINFW_RECOVERY = 0,
	ACTIVE_MAINFW_RW_A     = 1,
	ACTIVE_MAINFW_RW_B     = 2
};

enum {
	RECOVERY_REASON_NONE = 0,
	RECOVERY_REASON_ME   = 1
};
/* TODO(reinauer) other recovery reasons? */

enum {
	NONVOLATILE_STORAGE_NONE	= 0,
	NONVOLATILE_STORAGE_NVRAM	= 1,
	NONVOLATILE_STORAGE_DISK	= 2,
	NONVOLATILE_STORAGE_CROS_EC	= 3,
};

/* the data blob format */
typedef struct {
	/* Header of crossystem data blob */
	uint32_t	total_size;
	uint8_t		signature[10];
	uint16_t	version;

	/*
	 * Chrome OS-required GPIOs:
	 * - boot_*      : GPIO truth value at boot time
	 * - polarity_*  : Polarity (1=high active) of the GPIO pin
	 * - gpio_port_* : Port of the GPIO pin
	 */
	uint8_t		boot_write_protect_switch;
	uint8_t		boot_recovery_switch;
	uint8_t		boot_developer_switch;
	uint8_t		boot_oprom_loaded;
	uint8_t		polarity_write_protect_switch;
	uint8_t		polarity_recovery_switch;
	uint8_t		polarity_developer_switch;
	uint8_t		polarity_oprom_loaded;
	uint32_t	gpio_port_write_protect_switch;
	uint32_t	gpio_port_recovery_switch;
	uint32_t	gpio_port_developer_switch;
	uint32_t	gpio_port_oprom_loaded;

	/* Offset of FMAP on flashrom */
	uint32_t	fmap_offset;

	/*
	 * Firmware and system information:
	 * - active_ec_firmware   : 0=RO, 1=RW
	 * - firmware_type        : 0=recovery, 1=normal, 2=developer
	 * - hardware_id          : The hardware ID of this machine
	 * - readonly_firmware_id : ID of the read-only firmware
	 * - firmware_id          : ID of the active main firmware
	 */
	uint8_t		active_ec_firmware;
	uint8_t		firmware_type;
	uint8_t         oprom_matters;
	uint8_t		pad4;
	uint8_t 	hardware_id[ID_LEN];
	uint8_t		readonly_firmware_id[ID_LEN];
	uint8_t		firmware_id[ID_LEN];

	union {
		/* We reserve 208 bytes for board specific data */
		uint8_t board_reserved_size[0xd0];

		struct {
			/* Location of non-volatile context */
			uint64_t	nonvolatile_context_lba;
			uint16_t	nonvolatile_context_offset;
			uint16_t	nonvolatile_context_size;
			uint8_t		nonvolatile_context_storage;
		} arm;
	} board;

	/*
	 * VbSharedData contains fields of long word (64-bit) type, and so it
	 * should better be aligned on a 8-byte boundary on ARM platforms.
	 * Although ARM processors can access unaligned addresses, this feature
	 * is generally not enabled in U-Boot.
	 */
	uint8_t		vb_shared_data[VB_SHARED_DATA_REC_SIZE];
} __attribute__((packed)) crossystem_data_t;

/*
 * This structure is also used in coreboot. Any changes to this version have
 * to be made to that version as well
 */
typedef struct {
	/* ChromeOS specific */
	uint32_t	vbt0;		/* 00 boot reason */
	uint32_t	vbt1;		/* 04 active main firmware */
	uint32_t	vbt2;		/* 08 active ec firmware */
	uint16_t	vbt3;		/* 0c CHSW */
	uint8_t		vbt4[256];	/* 0e HWID */
	uint8_t		vbt5[64];	/* 10e FWID */
	uint8_t		vbt6[64];	/* 14e FRID - 275 */
	uint32_t	vbt7;		/* 18e active main firmware type */
	uint32_t	vbt8;		/* 192 recovery reason */
	uint32_t	vbt9;		/* 196 fmap base address */
	uint8_t		vdat[3072];	/* 19a */
	uint32_t	vbt10;		/* d9a */
	uint32_t	mehh[8];	/* d9e management enging hash */
					/* dbe */
} __attribute__((packed)) chromeos_acpi_t;

#define assert_offset(MEMBER, OFFSET) \
	typedef char static_assertion_##MEMBER_is_at_offset_##OFFSET[ \
		(offsetof(crossystem_data_t, MEMBER) == (OFFSET)) ? 1 : -1]

assert_offset(total_size,			0x0000);
assert_offset(signature,			0x0004);
assert_offset(version,				0x000e);

assert_offset(boot_write_protect_switch,	0x0010);
assert_offset(boot_recovery_switch,		0x0011);
assert_offset(boot_developer_switch,		0x0012);
assert_offset(boot_oprom_loaded,		0x0013);
assert_offset(polarity_write_protect_switch,	0x0014);
assert_offset(polarity_recovery_switch,		0x0015);
assert_offset(polarity_developer_switch,	0x0016);
assert_offset(polarity_oprom_loaded,		0x0017);
assert_offset(gpio_port_write_protect_switch,	0x0018);
assert_offset(gpio_port_recovery_switch,	0x001c);
assert_offset(gpio_port_developer_switch,	0x0020);
assert_offset(gpio_port_oprom_loaded,		0x0024);

assert_offset(fmap_offset,			0x0028);

assert_offset(active_ec_firmware,		0x002c);
assert_offset(firmware_type,			0x002d);
assert_offset(oprom_matters,			0x002e);
assert_offset(hardware_id,			0x0030);
assert_offset(readonly_firmware_id,		0x0130);
assert_offset(firmware_id,			0x0230);

assert_offset(board.arm.nonvolatile_context_lba,	0x0330);
assert_offset(board.arm.nonvolatile_context_offset,	0x0338);
assert_offset(board.arm.nonvolatile_context_size,	0x033a);

assert_offset(vb_shared_data,			0x0400);

#undef assert_offset

/**
 * This initializes the data blob that we will pass to kernel, and later be
 * used by crossystem. Note that:
 * - It does not initialize information of the main firmware, e.g., fwid. This
 *   information must be initialized in subsequent calls to the setters below.
 * - The recovery reason is default to VBNV_RECOVERY_NOT_REQUESTED.
 *
 * @param cdata is the data blob shared with crossystem
 * @param write_protect_switch points to a GPIO descriptor
 * @param recovery_switch points to a GPIO descriptor
 * @param developer_switch points to a GPIO descriptor
 * @param fmap_offset is the offset of FMAP in flashrom
 * @param hardware_id is of length ID_LEN
 * @param readonly_firmware_id is of length ID_LEN
 * @return 0 if it succeeds; non-zero if it fails
 */
int crossystem_data_init(crossystem_data_t *cdata,
		struct vboot_flag_details *write_protect_switch,
		struct vboot_flag_details *recovery_switch,
		struct vboot_flag_details *developer_switch,
		struct vboot_flag_details *oprom_loaded,
		uint8_t oprom_matters,
		uint32_t fmap_offset,
		uint8_t active_ec_firmware,
		uint8_t *hardware_id,
		uint8_t *readonly_firmware_id);

/**
 * This checks sanity of the crossystem data blob. Readwrite main firmware
 * should check the sanity of crossystem data that bootstub passes to it.
 *
 * @param cdata is the data blob shared with crossystem
 * @return 0 if it succeeds; non-zero if the sanity check fails
 */
int crossystem_data_check_integrity(crossystem_data_t *cdata);

/**
 * This sets the main firmware version and type.
 *
 * @param cdata is the data blob shared with crossystem
 * @param firmware_type
 * @param firmware_id is of length ID_LEN
 * @return 0 if it succeeds; non-zero if it fails
 */
int crossystem_data_set_main_firmware(crossystem_data_t *cdata,
		uint8_t firmware_type,
		uint8_t *firmware_id);

/**
 * This embeds kernel shared data into fdt.
 *
 * @param cdata is the data blob shared with crossystem
 * @param fdt points to a device tree
 * @return 0 if it succeeds, non-zero if it fails
 */
int crossystem_data_embed_into_fdt(crossystem_data_t *cdata, void *fdt);

#ifdef CONFIG_X86
/**
 * This embeds kernel shared data into the ACPI tables.
 *
 * @return 0 if it succeeds, non-zero if it fails
 */
int crossystem_data_update_acpi(crossystem_data_t *cdata);
#endif

/**
 * This prints out the data blob content to debug output.
 */
void crossystem_data_dump(crossystem_data_t *cdata);

#endif /* __CHROMEOS_CROSSYSTEM_DATA_H__ */
