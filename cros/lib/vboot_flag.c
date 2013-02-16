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
#include <fdtdec.h>
#include <asm/global_data.h>
#include <cros/common.h>
#include <cros/cros_fdtdec.h>
#include <cros/vboot_flag.h>

DECLARE_GLOBAL_DATA_PTR;

static struct vboot_flag_context internal_context[VBOOT_FLAG_MAX_FLAGS];

/* The drivers supported are listed here */
static struct vboot_flag_driver vboot_flag_driver_unknown = {
	.type	= COMPAT_UNKNOWN,
	.setup	= NULL,
	.fetch	= NULL,
};

#ifdef CONFIG_CHROMEOS_CONST_FLAG
extern struct vboot_flag_driver vboot_flag_driver_const;
#endif
#ifdef CONFIG_CHROMEOS_GPIO_FLAG
extern struct vboot_flag_driver vboot_flag_driver_gpio;
#endif
#ifdef CONFIG_CHROMEOS_CROS_EC_FLAG
extern struct vboot_flag_driver vboot_flag_driver_cros_ec;
#endif
#ifdef CONFIG_CHROMEOS_SYSINFO_FLAG
extern struct vboot_flag_driver vboot_flag_driver_sysinfo;
#endif

static int vboot_flag_init(void);

static const char *node_name[VBOOT_FLAG_MAX_FLAGS] = {
#define VBF(__e, __s) __s,
	VBOOT_FLAGS
#undef VBF
};

const char *vboot_flag_node_name(enum vboot_flag_id id)
{
	return node_name[id];
}

static struct vboot_flag_context *vboot_flag_get_context(enum vboot_flag_id id)
{
	static int vboot_flag_initted;
	if (!vboot_flag_initted) {
		vboot_flag_initted = 1;
		vboot_flag_init();
	}

	if (id < 0 || id >= VBOOT_FLAG_MAX_FLAGS) {
		VBDEBUG("id out of range: %d\n", id);
		return NULL;
	}
	return &internal_context[id];
}

static int vboot_flag_setup(enum vboot_flag_id id)
{
	struct vboot_flag_context *context = vboot_flag_get_context(id);

	if (context == NULL)
		return -1;

	if (context->driver) {
		if (context->driver->setup)
			return context->driver->setup(id, context);
		else
			return 0; /* setup() is unnecessary */
	}

	VBDEBUG("the driver of %s not assigned\n", node_name[id]);
	return -1;
}

enum fdt_compat_id vboot_flag_type(enum vboot_flag_id id)
{
	struct vboot_flag_context *context = vboot_flag_get_context(id);

	if (!context || !context->driver)
		return COMPAT_UNKNOWN;

#ifdef CONFIG_CHROMEOS_CONST_FLAG
	if (context->driver == &vboot_flag_driver_const)
		return COMPAT_GOOGLE_CONST_FLAG;
#endif
#ifdef CONFIG_CHROMEOS_GPIO_FLAG
	if (context->driver == &vboot_flag_driver_gpio)
		return COMPAT_GOOGLE_GPIO_FLAG;
#endif
#ifdef CONFIG_CHROMEOS_CROS_EC_FLAG
	if (context->driver == &vboot_flag_driver_cros_ec)
		return COMPAT_GOOGLE_CROS_EC_FLAG;
#endif
#ifdef CONFIG_CHROMEOS_SYSINFO_FLAG
	if (context->driver == &vboot_flag_driver_sysinfo)
		return COMPAT_GOOGLE_SYSINFO_FLAG;
#endif

	return COMPAT_UNKNOWN;
}

int vboot_flag_fetch(enum vboot_flag_id id, struct vboot_flag_details *details)
{
	struct vboot_flag_context *context = vboot_flag_get_context(id);

	if (context == NULL)
		return -1;

	if (context->driver && context->driver->fetch) {
		details->prev_value = context->prev_value;
		if (context->driver->fetch(id, context, details))
			return -1;
		context->prev_value = details->value;
		return 0;
	}

	VBDEBUG("the driver of %s not assigned\n", node_name[id]);
	return -1;
}

static int vboot_flag_init(void)
{
	const void *blob = gd->fdt_blob;
	int config, child;
	int i;
	struct vboot_flag_context *context;

	config = cros_fdtdec_config_node(blob);
	if (config < 0)
		return -1;

	for (i = 0; i < VBOOT_FLAG_MAX_FLAGS; i++) {
		context = &internal_context[i];

		/* By default, assign an unknown driver which does nothing */
		child = fdt_subnode_offset(blob, config, node_name[i]);
		if (child < 0) {
			VBDEBUG("can't find the node %s\n", node_name[i]);
			continue;
		}
		context->node = child;
		context->config_node = config;
		context->driver = &vboot_flag_driver_unknown;
		context->prev_value = -1;

		switch (fdtdec_lookup(blob, child)) {
#ifdef CONFIG_CHROMEOS_CONST_FLAG
		case COMPAT_GOOGLE_CONST_FLAG:
			context->driver = &vboot_flag_driver_const;
			break;
#endif
#ifdef CONFIG_CHROMEOS_GPIO_FLAG
		case COMPAT_GOOGLE_GPIO_FLAG:
			context->driver = &vboot_flag_driver_gpio;
			break;
#endif
#ifdef CONFIG_CHROMEOS_CROS_EC_FLAG
		case COMPAT_GOOGLE_CROS_EC_FLAG:
			context->driver = &vboot_flag_driver_cros_ec;
			break;
#endif
#ifdef CONFIG_CHROMEOS_SYSINFO_FLAG
		case COMPAT_GOOGLE_SYSINFO_FLAG:
			context->driver = &vboot_flag_driver_sysinfo;
			break;
#endif
		default:
			VBDEBUG("can't find any compatiable driver %s\n",
				node_name[i]);
		}
	}

	/* Call the setup function for each driver */
	for (i = 0; i < VBOOT_FLAG_MAX_FLAGS; i++)
		if (vboot_flag_setup(i))
			VBDEBUG("driver setup failed %s\n", node_name[i]);

	return 0;
}
