/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board power management function */

#include <common.h>
#include <asm/arch/board.h>
#include <asm/arch/power.h>
#include <cros/common.h>
#include <cros/power_management.h>

int is_processor_reset(void)
{
	return board_is_processor_reset();
}

/* This function never returns */
void cold_reboot(void)
{
	VBDEBUG("Reboot\n");

	/* Add a delay to allow serial output to drain */
	mdelay(100);
	power_reset();
}

/* This function never returns */
void power_off(void)
{
	VBDEBUG("Power off\n");

	/* Add a delay to allow serial output to drain */
	mdelay(100);
	power_shutdown();		/* never returns */
}
