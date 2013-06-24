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
#include <bootstage.h>
#include <part.h>
#include <cros/common.h>
#include <cros/boot_device.h>
#include <linux/list.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>

/* Maximum number of devices we can support */
enum {
	MAX_DISK_INFO	= 10,
};

/* TODO Move these definitions to vboot_wrapper.h or somewhere like that. */
enum {
	VBERROR_DISK_NO_DEVICE = 1,
	VBERROR_DISK_OUT_OF_RANGE,
	VBERROR_DISK_READ_ERROR,
	VBERROR_DISK_WRITE_ERROR,
};

int boot_device_matches(const block_dev_desc_t *dev,
				    uint32_t disk_flags, uint32_t *flags)
{
	*flags = dev->removable ? VB_DISK_FLAG_REMOVABLE :
			VB_DISK_FLAG_FIXED;
	return (*flags & disk_flags) == disk_flags;
}

int boot_device_present_and_matches(const block_dev_desc_t *dev,
		uint32_t disk_flags, uint32_t *flags)
{
	return dev->lba && boot_device_matches(dev, disk_flags, flags);
}

/**
 * Scan a list of devices and add those which match the supplied disk_flags
 * to the info array.
 *
 * @param name		Peripheral name
 * @param devs		List of devices to check
 * @param count		Number of devices
 * @param disk_flags	Disk flags which must be present for each device added
 * @param info		Output array of matching devices
 * @return number of devices added (0 if none)
 */
static int add_matching_devices(const char *name, block_dev_desc_t **devs,
		int count, uint32_t disk_flags, VbDiskInfo *info)
{
	int added = 0;
	int i;

	for (i = 0; i < count; i++) {
		block_dev_desc_t *dev = devs[i];
		uint32_t flags;

		/*
		* Only add this storage device if the properties of
		* disk_flags is a subset of the properties of flags.
		*/
		if (boot_device_present_and_matches(dev, disk_flags, &flags)) {
			info->handle = (VbExDiskHandle_t)dev;
			info->bytes_per_lba = dev->blksz;
			info->lba_count = dev->lba;
			info->flags = flags;
			info->name = name;
			info++, added++;
		}
	}
	return added;
}

VbError_t VbExDiskGetInfo(VbDiskInfo **infos_ptr, uint32_t *count_ptr,
			  uint32_t disk_flags)
{
	VbDiskInfo *infos;
	uint32_t max_count;	/* maximum devices to scan for */
	uint32_t count = 0;	/* number of matching devices found */
	block_dev_desc_t *dev[MAX_DISK_INFO];
	struct boot_interface *start, *iface;
	int interface_count;
	int upto;

	/* We return as many disk infos as possible. */
	max_count = MAX_DISK_INFO;

	infos = (VbDiskInfo *)VbExMalloc(sizeof(VbDiskInfo) * max_count);

	bootstage_start(BOOTSTAGE_ACCUM_VBOOT_BOOT_DEVICE_INFO,
			"boot_device_info");
	start = ll_entry_start(struct boot_interface, boot_interface);
	interface_count = ll_entry_count(struct boot_interface, boot_interface);

	/* Scan through all the interfaces looking for devices */
	for (upto = 0, iface = start;
	     upto < interface_count && count < max_count;
	     iface++, upto++) {
		int found;

		found = iface->start(disk_flags);
		if (found < 0) {
			VBDEBUG("%s: start() failed\n", iface->name);
			continue;
		}
		if (!found) {
			VBDEBUG("%s - start() returned 0\n",
				iface->name);
			continue;
		}

		found = iface->scan(dev, max_count - count, disk_flags);
		if (found < 0) {
			VBDEBUG("%s: scan() failed\n", iface->name);
			continue;
		}
		assert(found <= max_count - count);

		/* Now record the devices that have the required flags */
		count += add_matching_devices(iface->name, dev, found,
					      disk_flags, infos + count);
	}

	if (count) {
		*infos_ptr = infos;
		*count_ptr = count;
	} else {
		*infos_ptr = NULL;
		*count_ptr = 0;
		VbExFree(infos);
	}
	bootstage_accum(BOOTSTAGE_ACCUM_VBOOT_BOOT_DEVICE_INFO);

	/* The operation itself succeeds, despite scan failure all about */
	return VBERROR_SUCCESS;
}

VbError_t VbExDiskFreeInfo(VbDiskInfo *infos, VbExDiskHandle_t preserve_handle)
{
	/* We do nothing for preserve_handle as we keep all the devices on. */
	VbExFree(infos);
	return VBERROR_SUCCESS;
}

VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
                       uint64_t lba_count, void *buffer)
{
	block_dev_desc_t *dev = (block_dev_desc_t *)handle;

	bootstage_start(BOOTSTAGE_ACCUM_VBOOT_BOOT_DEVICE_READ,
			"boot_device_read");
	if (dev == NULL)
		return VBERROR_DISK_NO_DEVICE;

	if (lba_start >= dev->lba || lba_start + lba_count > dev->lba)
		return VBERROR_DISK_OUT_OF_RANGE;

	if (dev->block_read(dev->dev, lba_start, lba_count, buffer)
			!= lba_count)
		return VBERROR_DISK_READ_ERROR;
	bootstage_accum(BOOTSTAGE_ACCUM_VBOOT_BOOT_DEVICE_READ);

	return VBERROR_SUCCESS;
}

VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
                        uint64_t lba_count, const void *buffer)
{
	block_dev_desc_t *dev = (block_dev_desc_t *)handle;

	/* TODO Replace the error codes with pre-defined macro. */
	if (dev == NULL)
		return VBERROR_DISK_NO_DEVICE;

	if (lba_start >= dev->lba || lba_start + lba_count > dev->lba)
		return VBERROR_DISK_OUT_OF_RANGE;

	if (!dev->block_write) {
		VBDEBUG("interface (%d) does not support writing.\n",
			(int)dev->if_type);
		return VBERROR_DISK_WRITE_ERROR;
	}
	if (dev->block_write(dev->dev, lba_start, lba_count, buffer)
			!= lba_count)
		return VBERROR_DISK_WRITE_ERROR;

	return VBERROR_SUCCESS;
}
