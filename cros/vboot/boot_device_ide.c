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
#include <ide.h>
#include <cros/boot_device.h>

static int boot_device_ide_start(uint32_t disk_flags)
{
	/* We expect to have at least one IDE device */
	return 1;
}

static int boot_device_ide_scan(block_dev_desc_t **desc, int max_devs,
			 uint32_t disk_flags)
{
	int index, found;

	for (index = found = 0; index < max_devs; index++) {
		block_dev_desc_t *ide;

		ide = ide_get_dev(index);
		if (!ide)
			continue;

		desc[found++] = ide;
	}
	return found;
}

CROS_BOOT_DEVICE(ide_interface) = {
	.name = "ide",
	.type = IF_TYPE_IDE,
	.start = boot_device_ide_start,
	.scan = boot_device_ide_scan,
};
