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
#include <cros/boot_device.h>
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <cros/keyboard.h>
#include <cros/nvstorage.h>
#include <cros/vboot_flag.h>

int cros_init(void)
{
	bootstage_set_next_id(BOOTSTAGE_VBOOT_LAST);

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

int defer_display_init(const void *blob)
{
#ifdef CONFIG_OF_CONTROL
#define UNINITTED_VALUE 0xdeadbeef
	static int lazy_init = UNINITTED_VALUE;

	if (lazy_init == UNINITTED_VALUE) {
		int node;

		node = cros_fdtdec_config_node(blob);
		if ((node >= 0) &&
		    (fdtdec_get_int(blob, node, "lazy-init", -1) > 0))
			lazy_init = 1; /* Init should be deferred. */
		else
			lazy_init = 0; /* Init should NOT be deferred. */
	}

	return lazy_init;
#else
	return 0;
#endif
}
