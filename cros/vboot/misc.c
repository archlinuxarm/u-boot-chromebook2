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
#include <cros/vboot_flag.h>
#include <vboot_api.h>

uint32_t VbExIsShutdownRequested(void)
{
	struct vboot_flag_details lidsw, pwrsw;

	/* if lid is NOT OPEN */
	if (!vboot_flag_fetch(VBOOT_FLAG_LID_OPEN, &lidsw) && !lidsw.value) {
		VBDEBUG("Lid-closed is detected.\n");
		return 1;
	}
	/*
	 * If power switch is pressed (but previously was known to be not
	 * pressed), we power off.
	 */
	if (!vboot_flag_fetch(VBOOT_FLAG_POWER_OFF, &pwrsw) &&
			!pwrsw.prev_value && pwrsw.value) {
		VBDEBUG("Power-key-pressed is detected.\n");
		return 1;
	}
	/*
	 * Either the gpios don't exist, or the lid is up and and power button
	 * is not pressed. No-Shutdown-Requested.
	 */
	return 0;
}
