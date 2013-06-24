/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_BOOT_DEVICE_H_
#define CHROMEOS_BOOT_DEVICE_H_

#include <linker_lists.h>

/*
 * This is an interface that we can boot from. It provides a function to start
 * up the peripheral and also one to scan for available devices attached to
 * this peripheral.
 */
struct boot_interface {
	const char *name;
	int type;		/* IF_TYPE... from part.h */

	/**
	 * Start the peripheral interface, so we are ready to scan for
	 * devices. This function can return 0 if it knows that there is
	 * no need to scan. If it returns 1 or more, then a scan will
	 * started.
	 *
	 * @param flags		Flags for this disk: VB_DISK_FLAG_...
	 * @return 0 if no devices, >=1 if we have devices, -1 on error
	 */
	int (*start)(uint32_t flags);

	/**
	 * Scan this peripheral for available devices
	 *
	 * @param desc		Array to put available devices into
	 * @param max_devs	Maximum number of devices to return
	 * @param flags		Flags for this disk: VB_DISK_FLAG_...
	 * @return number of devices found, or -1 on error
	 */
	int (*scan)(block_dev_desc_t **desc, int max_devs, uint32_t flags);
};

/**
 * Register a new boot interface.
 *
 * @param iface		The interface to register
 * @return 0 if ok, -1 on error
 */
int boot_device_register_interface(struct boot_interface *iface);

/**
 * Checks if a given device matches the provided disk_flags.
 *
 * @param dev		Device to check
 * @param disk_flags	Disk flags which must be present for this device
 * @param flags		Returns calculated flags for this device
 * @return 1 if the device matches, 0 if not
 */
int boot_device_matches(const block_dev_desc_t *dev, uint32_t disk_flags,
			uint32_t *flags);

/**
 * Checks if a given device is presented and matches the provided disk_flags.
 *
 * @param dev		Device to check
 * @param disk_flags	Disk flags which must be present for this device
 * @param flags		Returns calculated flags for this device
 * @return 1 if the device matches, 0 if not
 */
int boot_device_present_and_matches(const block_dev_desc_t *dev,
		uint32_t disk_flags, uint32_t *flags);

/* Declare a boot device, capable of loading a kernel */
#define CROS_BOOT_DEVICE(_name) \
	ll_entry_declare(struct boot_interface, _name, boot_interface)

#endif /* CHROMEOS_BOOT_DEVICE_H_ */
