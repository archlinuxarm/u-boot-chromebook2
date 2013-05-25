/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board GPIO setup function */

#include <common.h>
#include <asm/arch/gpio.h>
#include <cros/common.h>
#include <cros/vboot_flag.h>

int vboot_flag_setup_gpio_arch(enum vboot_flag_id id,
			       struct vboot_flag_context *context)
{
	gpio_set_pull(context->gpio_state.gpio, S5P_GPIO_PULL_NONE);
	return 0;
}
