/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of vboot flag accessor from communicating with EC */

#include <common.h>
#include <cros_ec.h>
#include <cros/common.h>
#include <cros/vboot_flag.h>

static int vboot_flag_fetch_cros_ec(enum vboot_flag_id id,
				 struct vboot_flag_context *context,
				 struct vboot_flag_details *details)
{
	struct cros_ec_dev *dev;
	struct ec_response_mkbp_info info;

	dev = board_get_cros_ec_dev();
	if (!dev) {
		VBDEBUG("%s: no cros_ec device\n", __func__);
		return -1;
	}

	if (cros_ec_info(dev, &info)) {
		VBDEBUG("Could not read KBC info\n");
		return -1;
	}

	/* TODO(waihong): Could check things like EC_SWITCH_LID_OPEN here */
	switch (id) {
	default:
		VBDEBUG("the flag is not supported reading from ec: %s\n",
			vboot_flag_node_name(id));
		return -1;
	}
	details->port = 0;
	details->active_high = 0;

	return 0;
}

CROS_VBOOT_FLAG_DRIVER(cros_ec) = {
	.name	= "cros-ec",
	.compat	= COMPAT_GOOGLE_CROS_EC_FLAG,
	.setup	= NULL,
	.fetch	= vboot_flag_fetch_cros_ec,
};
