/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of vboot flag accessor from FDT constant value */

#include <common.h>
#include <fdtdec.h>
#include <cros/common.h>
#include <cros/vboot_flag.h>

DECLARE_GLOBAL_DATA_PTR;

static int vboot_flag_fetch_const(enum vboot_flag_id id,
				  struct vboot_flag_context *context,
				  struct vboot_flag_details *details)
{
	int node;

	node = context->node;
	if (node == 0) {
		VBDEBUG("the vboot flag node is not initialized\n");
		return -1;
	}
	details->port = 0;
	details->active_high = 0;
	details->value = fdtdec_get_int(gd->fdt_blob, node, "value", 0);
	return 0;
}

CROS_VBOOT_FLAG_DRIVER(const) = {
	.name	= "const",
	.compat	= COMPAT_GOOGLE_CONST_FLAG,
	.setup	= NULL,
	.fetch	= vboot_flag_fetch_const,
};
