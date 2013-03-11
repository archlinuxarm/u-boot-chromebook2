/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __configs_chromeos_coreboot_h__
#define __configs_chromeos_coreboot_h__

/* So far all our x86-based boards share the coreboot config. */
#include <configs/coreboot.h>

/* Support USB booting */
#define CONFIG_CHROMEOS_USB

/* Support vboot flag reading from sysinfo struct and hardware pin */
#define CONFIG_CHROMEOS_SYSINFO_FLAG
#define CONFIG_CHROMEOS_GPIO_FLAG

#define CONFIG_INITRD_ADDRESS 0x12008000

#include "chromeos.h"

#endif /* __configs_chromeos_coreboot_h__ */
