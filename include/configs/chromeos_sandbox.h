/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __configs_chromeos_sandbox_h__
#define __configs_chromeos_sandbox_h__

#include <configs/sandbox.h>

#include <configs/chromeos.h>

/* Unfortunately we don't support everything yet */
#undef CONFIG_CHROMEOS_USB
#undef CONFIG_CHROMEOS_DISPLAY

#define CONFIG_CHROMEOS_GPIO_FLAG
#define CONFIG_CHROMEOS_CROS_EC_FLAG

#endif /* __configs_chromeos_sandbox_h__ */
