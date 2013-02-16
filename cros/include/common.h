/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_COMMON_H_
#define CHROMEOS_COMMON_H_

#include <vboot_api.h>

#if defined VBOOT_DEBUG
#define VBDEBUG(fmt, args...) \
	VbExDebug("%s: %s: " fmt, __FILE__, __func__, ##args)
#elif defined DEBUG
#define VBDEBUG debug
#else
#define VBDEBUG(fmt, args...)
#endif

/*
 * VBDEBUG(), which indirectly calls printf(), has an internal buffer on output
 * string, and so cannot output very long string. Thus, if you want to print a
 * very long string, please use VBDEBUG_PUTS(), which calls puts().
 */
#if defined VBOOT_DEBUG || defined DEBUG
#define VBDEBUG_PUTS(str) puts(str)
#else
#define VBDEBUG_PUTS(str)
#endif


enum {
	BOOTSTAGE_VBOOT_TWOSTOP = BOOTSTAGE_ID_USER,
	BOOTSTAGE_VBOOT_TWOSTOP_INIT,
	BOOTSTAGE_VBOOT_SELECT_AND_SET,
	BOOTSTAGE_VBOOT_TWOSTOP_MAIN_FIRMWARE,
	BOOTSTAGE_VBOOT_VBINIT_ENTER,
	BOOTSTAGE_VBOOT_VBINIT_EXIT,
	BOOTSTAGE_VBOOT_SELECT_FIRMWARE_ENTER,
	BOOTSTAGE_VBOOT_SELECT_FIRMWARE_EXIT,
	BOOTSTAGE_VBOOT_SELECT_LOAD_KERNEL_ENTER,
	BOOTSTAGE_VBOOT_SELECT_LOAD_KERNEL_EXIT,

	BOOTSTAGE_ACCUM_VBOOT_BOOT_DEVICE_INFO,
	BOOTSTAGE_ACCUM_VBOOT_BOOT_DEVICE_READ,

	BOOTSTAGE_VBOOT_LAST,
};

struct cros_ec_dev;

/**
 * Allocate a memory space aligned to cache line size.
 *
 * @param n	Size to be allocated
 * @return pointer to the allocated space or NULL on error.
 */
void *cros_memalign_cache(size_t n);

/* this function is implemented along with vboot_api */
int display_clear(void);

/**
 * Test code for performing a software sync
 *
 * This is used for dogfood devices where we want to update the RO EC.
 *
 * @param dev		cros_ec device
 * @param region_mask	Bit mask of regions to update:
 *				1 << EC_FLASH_REGION_RO: read-only
 *				1 << EC_FLASH_REGION_RW: read-write
 * @param force		Force update without checking existing contents
 * @param verify	Verify EC contents after writing
 */
int cros_test_swsync(struct cros_ec_dev *dev, int region_mask, int force,
		     int verify);

/**
 * Select a byte of the EC image to corrupt
 *
 * Next time verified boot calls VbExEcGetExpectedRW we will corrupt a single
 * byte of the image.
 *
 * @param offset	Offset to corrupt (-1 for none)
 * @param byte		Byte value to put into that offset
 */
void cros_ec_set_corrupt_image(int offset, int byte);

/**
 * Ensure that bitmaps are loaded into our gbb area
 *
 * @return 0 if ok, -1 on error
 */
int cros_cboot_twostop_read_bmp_block(void);

#endif /* CHROMEOS_COMMON_H_ */
