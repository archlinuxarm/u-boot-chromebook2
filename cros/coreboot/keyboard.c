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
#include <config.h>
#include <fdtdec.h>
#include <cros/common.h>
#include <cros/crossystem_data.h>
#include <cros/cros_fdtdec.h>
#include <cros/firmware_storage.h>
#include <cros/power_management.h>

int board_i8042_skip(void)
{
	struct vboot_flag_details devsw;

	vboot_flag_fetch(VBOOT_FLAG_DEVELOPER, &devsw);
	if (devsw.value)
		return 0;

	return fdtdec_get_config_int(gd->fdt_blob, "skip-i8042", 0);
}
