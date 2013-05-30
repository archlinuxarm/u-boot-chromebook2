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
#include <usb.h>
#include <cros/boot_device.h>
#include <cros/crossystem_data.h>
#include <vboot_api.h>

static int is_enumerated;

#ifdef CONFIG_USB_KEYBOARD
int board_use_usb_keyboard(int boot_mode)
{
	struct vboot_flag_details devsw;

	/* the keyboard is needed only in developer mode and recovery mode */
	vboot_flag_fetch(VBOOT_FLAG_DEVELOPER, &devsw);
	if (!devsw.value && (boot_mode != FIRMWARE_TYPE_RECOVERY))
		return 0;

	/* does this machine have a USB keyboard as primary input ? */
	if (fdtdec_get_config_bool(gd->fdt_blob, "usb-keyboard"))
		return 1;

	return 0;
}
#endif

static int boot_device_usb_start(uint32_t disk_flags)
{
	static char has_been_powered_on;
	int enumerate = 1;

	/* If we aren't looking for removable disks, skip USB */
	if (!(disk_flags & VB_DISK_FLAG_REMOVABLE))
		return 0;

	/*
	 * if the USB devices have already been enumerated, redo it
	 * only if something has been plugged on unplugged.
	 */
	if (is_enumerated)
		enumerate = usb_detect_change();

	if (enumerate) {
		/*
		 * TODO(crosbug/11523): USB flash drive responds to probing
		 * only after it is powered on; so start USB device if this is
		 * the first time, and wait 3 seconds for them to power up.
		 * From experiments a 2 seconds delay is sufficient; choose 3
		 * seconds to have more margin of safety.
		 *
		 * However, this should be a temporary workaround, as USB driver
		 * should deal with this situation.
		 */
		if (!has_been_powered_on) {
			usb_stop();
			usb_init();
			mdelay(3000);
			has_been_powered_on = 1;
		}

		/*
		 * We should stop all USB devices first. Otherwise we can't
		 * detect any new devices.
		 */
		usb_stop();

		if (usb_init() >= 0) {
#ifdef CONFIG_USB_KEYBOARD
			if (board_use_usb_keyboard(FIRMWARE_TYPE_RECOVERY))
				drv_usb_kbd_init();
#endif
			usb_stor_scan(/*mode=*/1);
			is_enumerated = 1;
		}
	}
	return 1;
}

static int boot_device_usb_scan(block_dev_desc_t **desc, int max_devs,
			 uint32_t disk_flags)
{
	int index;

	max_devs = min(max_devs, USB_MAX_STOR_DEV);

	for (index = 0; index < max_devs; index++) {
		desc[index] = usb_stor_get_dev(index);
		if (!desc[index])
			break;
	}
	return index;
}

static struct boot_interface usb_interface = {
	.name = "usb",
	.type = IF_TYPE_USB,
	.start = boot_device_usb_start,
	.scan = boot_device_usb_scan,
};

int boot_device_usb_probe(void)
{
	return boot_device_register_interface(&usb_interface);
}
