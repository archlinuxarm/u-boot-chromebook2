/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of vboot flag accessor from sysinfo */

#include <common.h>
#include <asm/arch-coreboot/ipchecksum.h>
#include <asm/arch-coreboot/sysinfo.h>
#include <asm/arch-coreboot/tables.h>
#include <cros/common.h>
#include <cros/vboot_flag.h>

DECLARE_GLOBAL_DATA_PTR;

static int vboot_flag_fetch_sysinfo(enum vboot_flag_id id,
				    struct vboot_flag_context *context,
				    struct vboot_flag_details *details)
{
	const void *blob = gd->fdt_blob;
	const struct fdt_property *prop;
	int len, i, p;

	prop = fdt_get_property(blob, context->node, "google,name", &len);
	if (!prop) {
		VBDEBUG("failed to read name of %s\n",
			vboot_flag_node_name(id));
		return -1;
	}

	for (i = 0; i < lib_sysinfo.num_gpios; i++) {
		if (strncmp((char *)lib_sysinfo.gpios[i].name, prop->data,
						GPIO_MAX_NAME_LENGTH))
			continue;

		/* Entry found */
		details->port = lib_sysinfo.gpios[i].port;
		details->active_high = lib_sysinfo.gpios[i].polarity;
		p = details->active_high ? 0 : 1;
		details->value = p ^ lib_sysinfo.gpios[i].value;

		return 0;
	}

	VBDEBUG("failed to find sysinfo flag for %s\n",
		vboot_flag_node_name(id));
	return -1;
}

CROS_VBOOT_FLAG_DRIVER(sysinfo) = {
	.name	= "sysinfo",
	.compat   = COMPAT_GOOGLE_SYSINFO_FLAG,
	.fetch  = vboot_flag_fetch_sysinfo,
};
