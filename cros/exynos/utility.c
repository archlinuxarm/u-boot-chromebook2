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
#include <cros/common.h>

#include <vboot_api.h>

DECLARE_GLOBAL_DATA_PTR;

const char *cros_fdt_get_mem_type(void)
{
	const void *blob = gd->fdt_blob;
	int nodeoffset;

	nodeoffset = fdt_path_offset(blob, "/dmc");
	if (nodeoffset > 0)
		return fdt_getprop(blob, nodeoffset, "mem-type", NULL);

	return NULL;
}
