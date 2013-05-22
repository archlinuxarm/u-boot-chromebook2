/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_GBB_H_
#define CHROMEOS_GBB_H_

#include <cros/firmware_storage.h>

/**
 * This loads the basic parts of GBB from flashrom. These include:
 *  - GBB header
 *  - hardware id
 *  - rootkey for verifying readwrite firmware
 *
 * @param gbb		Buffer for holding GBB
 * @param file		Flashrom device handle
 * @param gbb_offset	Offset of GBB in flashrom device
 * @param gbb_size	Size of the buffer holding GBB
 * @return zero if this succeeds, non-zero if this fails
 */
int gbb_init(void *gbb, firmware_storage_t *file, uint32_t gbb_offset,
	     size_t gbb_size);

/**
 * This loads the BMP block of GBB from flashrom.
 *
 * @param gbb		Buffer for holding GBB
 * @param file		Flashrom device handle
 * @param gbb_offset	Offset of GBB in flashrom device
 * @param gbb_size	Size of the buffer holding GBB
 * @return zero if this succeeds, non-zero if this fails
 */
int gbb_read_bmp_block(void *gbb, firmware_storage_t *file,
		       uint32_t gbb_offset, size_t gbb_size);

/*
 * This loads the recovery key of GBB from flashrom.
 *
 * @param gbb		Buffer for holding GBB
 * @param file		Flashrom device handle
 * @param gbb_offset	Offset of GBB in flashrom device
 * @param gbb_size	Size of the buffer holding GBB
 * @return zero if this succeeds, non-zero if this fails
 */
int gbb_read_recovery_key(void *gbb, firmware_storage_t *file,
			  uint32_t gbb_offset, size_t gbb_size);

/**
 * This is a sanity check of GBB blob.
 *
 * @param gbb		Buffer for holding GBB
 * @return zero if the check passes, non-zero if the check fails
 */
int gbb_check_integrity(uint8_t *gbb);

/**
 * Get the GBB flags as they were set at init
 *
 * @return gbb_flags
 */
uint32_t gbb_get_flags(void);

#endif /* CHROMEOS_GBB_H_ */
