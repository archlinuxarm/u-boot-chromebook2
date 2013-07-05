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
#include <bootstage.h>
#include <cros/common.h>
#include <cros/boot_device.h>
#include <cros/nvstorage.h>
#include <cros/vboot_flag.h>
#include <cros/keyboard.h>

int cros_init(void)
{
	bootstage_set_next_id(BOOTSTAGE_VBOOT_LAST);
	if (boot_device_init()) {
		VBDEBUG("boot_device_init failed\n");
		return -1;
	}

	if (nvstorage_init()) {
		VBDEBUG("nvstorage_init failed\n");
		return -1;
	}

	if (vboot_keymap_init()) {
		VBDEBUG(" vboot_keyboard_init failed\n");
		return -1;
	}

	if (vboot_flag_init()) {
		VBDEBUG(" vboot_flag_init() failed\n");
		return -1;
	}

	return 0;
}
