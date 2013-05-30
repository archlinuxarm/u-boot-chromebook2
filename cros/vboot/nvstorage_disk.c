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
#include <cros/common.h>
#include <cros/nvstorage.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

/*
 * We had a discussion about the non-volatile storage device for keeping
 * the cookies. Due to the lack of SPI flash driver in kernel, kernel cannot
 * access cookies in SPI flash. So the final descision is to store the
 * cookies in eMMC device where we are certain that kernel can access.
 */

/*
 * Gets the first internal disk and caches the result in a static variable.
 * Returns 0 for success, non-zero for failure.
 */
static int get_internal_disk(VbDiskInfo **disk_ptr)
{
	static VbDiskInfo internal_disk;

	if (internal_disk.handle == NULL) {
		VbDiskInfo *disk_info;
		uint32_t disk_count;

		if (VbExDiskGetInfo(&disk_info, &disk_count,
				VB_DISK_FLAG_FIXED) || disk_count == 0) {
			VBDEBUG("No internal disk found!\n");
			return 1;
		}
		internal_disk = disk_info[0];
		VbExDiskFreeInfo(disk_info, internal_disk.handle);
	}

	*disk_ptr = &internal_disk;
	return 0;
}

/*
 * Allocates 1-block-sized memory to block_buf_ptr and fills it as the first
 * block of the disk.
 * Returns 0 for success, non-zero for failure.
 */
static int get_nvcxt_block_of_disk(const VbDiskInfo *disk,
		uint8_t **block_buf_ptr)
{
	uint8_t *block_buf = NULL;

	block_buf = VbExMalloc(disk->bytes_per_lba);

	if (VbExDiskRead(disk->handle,
				CHROMEOS_VBNVCONTEXT_LBA, 1, block_buf)) {
		VBDEBUG("Failed to read internal disk!\n");
		VbExFree(block_buf);
		return 1;
	}

	*block_buf_ptr = block_buf;
	return 0;
}

VbError_t nvstorage_read_disk(uint8_t *buf)
{
	VbDiskInfo *internal_disk;
	uint8_t *block_buf;

	if (get_internal_disk(&internal_disk))
		return 1;

	if (get_nvcxt_block_of_disk(internal_disk, &block_buf))
		return 1;

	memcpy(buf, block_buf, VBNV_BLOCK_SIZE);

	VbExFree(block_buf);
	return VBERROR_SUCCESS;
}

VbError_t nvstorage_write_disk(const uint8_t *buf)
{
	VbDiskInfo *internal_disk;
	uint8_t *block_buf;

	if (get_internal_disk(&internal_disk))
		return 1;

	if (get_nvcxt_block_of_disk(internal_disk, &block_buf))
		return 1;

	memcpy(block_buf, buf, VBNV_BLOCK_SIZE);

	if (VbExDiskWrite(internal_disk->handle,
				CHROMEOS_VBNVCONTEXT_LBA, 1, block_buf)) {
		VBDEBUG("Failed to write internal disk!\n");
		VbExFree(block_buf);
		return 1;
	}

	VbExFree(block_buf);

#ifdef CONFIG_EXYNOS5
	/*
	 * XXX(chrome-os-partner:10415): On Exynos, reliable write operations
	 * need write busy time; so add a delay here.  In the long run, we
	 * should avoid using eMMC as VbNvContext storage media.
	 */
	mdelay(1);
#endif

	return VBERROR_SUCCESS;
}
