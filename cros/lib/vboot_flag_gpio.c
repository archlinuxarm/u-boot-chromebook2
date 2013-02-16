/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of vboot flag accessor from GPIO hardware */

#include <common.h>
#include <fdtdec.h>
#include <asm-generic/gpio.h>
#include <asm/global_data.h>
#include <cros/common.h>
#include <cros/vboot_flag.h>

DECLARE_GLOBAL_DATA_PTR;

/* Default empty implementation */
static int __vboot_flag_setup_gpio_arch(enum vboot_flag_id id,
					struct vboot_flag_context *context)
{
	return 0;
}

int vboot_flag_setup_gpio_arch(enum vboot_flag_id id,
			       struct vboot_flag_context *context)
	__attribute__((weak, alias("__vboot_flag_setup_gpio_arch")));

static int vboot_flag_setup_gpio(enum vboot_flag_id id,
				 struct vboot_flag_context *context)
{
	const void *blob = gd->fdt_blob;
	unsigned long delay_time;

	if (fdtdec_decode_gpio(blob, context->node, "gpio",
			&context->gpio_state)) {
		VBDEBUG("failed to decode GPIO state: %s\n",
			vboot_flag_node_name(id));
		return -1;
	}
	fdtdec_setup_gpio(&context->gpio_state);
	if (fdt_gpio_isvalid(&context->gpio_state)) {
		if (vboot_flag_setup_gpio_arch(id, context)) {
			VBDEBUG("arch specific setup failed: %s\n",
				vboot_flag_node_name(id));
			return -1;
		}
		gpio_direction_input(context->gpio_state.gpio);
	}

	/*
	 * In theory we have to insert a delay here for charging the input
	 * gate capacitance. Consider a 200K ohms series resister and 10
	 * picofarads gate capacitance.
	 *
	 * RC time constant is
	 *     200 K ohms * 10 picofarads = 2 microseconds
	 *
	 * Then 10-90% rise time is
	 *     2 microseconds * 2.2 = 4.4 microseconds
	 *
	 * Thus, 10 microseconds gives us a 50% margin.
	 */
	delay_time = fdtdec_get_int(blob, context->config_node,
			"cros-gpio-input-charging-delay", 0);
	if (delay_time)
		context->gpio_valid_time = timer_get_us() + delay_time;

	context->initialized = 1;

	return 0;
}

static int vboot_flag_fetch_gpio(enum vboot_flag_id id,
				 struct vboot_flag_context *context,
				 struct vboot_flag_details *details)
{
	int p, valid_time;

	if (!context->initialized) {
		VBDEBUG("gpio state is not initialized\n");
		return -1;
	}
	details->port = context->gpio_state.gpio;
	details->active_high = (context->gpio_state.flags &
			FDT_GPIO_ACTIVE_LOW) ? 0 : 1;
	p = details->active_high ? 0 : 1;

	valid_time = context->gpio_valid_time;
	if (valid_time) {
		/* We can only read GPIO after valid_time */
		while (timer_get_us() < valid_time)
			udelay(10);
	}
	details->value = p ^ gpio_get_value(details->port);

	return 0;
}

struct vboot_flag_driver vboot_flag_driver_gpio = {
	.type	= COMPAT_GOOGLE_GPIO_FLAG,
	.setup	= vboot_flag_setup_gpio,
	.fetch	= vboot_flag_fetch_gpio,
};
