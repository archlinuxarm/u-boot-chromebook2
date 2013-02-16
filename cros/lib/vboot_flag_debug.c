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
#include <cros/common.h>
#include <cros/vboot_flag.h>

int vboot_flag_dump(enum vboot_flag_id id, struct vboot_flag_details *details)
{
#ifdef VBOOT_DEBUG
	struct vboot_flag_details local_details;

	if (id < 0 || id >= VBOOT_FLAG_MAX_FLAGS) {
		VBDEBUG("id out of range: %d\n", id);
		return -1;
	}

	if (details == NULL) {
		if (vboot_flag_fetch(id, &local_details)) {
			VBDEBUG("failed to get vboot_flag details of %s\n",
				 vboot_flag_node_name(id));
			return -1;
		}
		details = &local_details;
	}

	VBDEBUG("%-24s: port=%3d, active_high=%d, value=%d\n",
			vboot_flag_node_name(id),
			details->port, details->active_high, details->value);
#endif
	return 0;
}
