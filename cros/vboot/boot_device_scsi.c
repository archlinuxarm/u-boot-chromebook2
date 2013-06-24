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
#include <scsi.h>
#include <cros/boot_device.h>

static int boot_device_scsi_start(uint32_t disk_flags)
{
	/* boot_interface->start() returns the number of
	 * hard drives/boot devices for that interface
	 */
	return scsi_get_disk_count();
}

static int boot_device_scsi_scan(block_dev_desc_t **desc, int max_devs,
			 uint32_t disk_flags)
{
	int index, found;

	for (index = found = 0; index < max_devs; index++) {
		block_dev_desc_t *scsi;

		scsi = scsi_get_dev(index);
		if (!scsi)
			continue;

		desc[found++] = scsi;
	}
	return found;
}

CROS_BOOT_DEVICE(scsi_interface) = {
	.name = "scsi",
	.type = IF_TYPE_SCSI,
	.start = boot_device_scsi_start,
	.scan = boot_device_scsi_scan,
};
