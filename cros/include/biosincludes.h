/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef BIOSINCLUDES_H
#define BIOSINCLUDES_H

#define UINT64_C(v) ((uint64_t) v)

#include <compiler.h>
#include <inttypes.h>

#define INLINE inline
#define POSSIBLY_UNUSED

#define UINT64_RSHIFT(v, shiftby) (((uint64_t)(v)) >> (shiftby))
#define UINT64_MULT32(v, multby)  (((uint64_t)(v)) * ((uint32_t)(multby)))

#endif /* BIOSINCLUDES_H */
