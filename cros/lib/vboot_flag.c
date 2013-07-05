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

int vboot_flag_init(void)
{
	const void *blob = gd->fdt_blob;
	int config, child;
	struct vboot_flag_context *context;
	struct vboot_flag_driver *start, *drv;
	int i, count, compat, upto;

	config = cros_fdtdec_config_node(blob);
	if (config < 0)
		return -1;

	start = ll_entry_start(struct vboot_flag_driver, vboot_flag_driver);
	count = ll_entry_count(struct vboot_flag_driver, vboot_flag_driver);
	for (i = 0; i < VBOOT_FLAG_MAX_FLAGS; i++) {
		context = &internal_context[i];

		child = fdt_subnode_offset(blob, config, node_name[i]);
		if (child < 0) {
			VBDEBUG("can't find the node %s\n", node_name[i]);
			continue;
		}
		context->node = child;
		context->config_node = config;
		context->driver = NULL;
		context->prev_value = -1;

		compat = fdtdec_lookup(blob, child);
		for (drv = start, upto = 0; upto < count; drv++, upto++) {
			if (drv->compat == compat)
				context->driver = drv;
		}
		if (!context->driver) {
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
