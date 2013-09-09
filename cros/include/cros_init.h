/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_CROS_INIT_
#define CHROMEOS_CROS_INIT_

/**
 * Initialize the cros library; it must be called before calling to the rest of
 * cros library.
 *
 * @return zero on success and non-zero on error.
 */
int cros_init(void);

#endif /* CHROMEOS_CROS_INIT_ */
