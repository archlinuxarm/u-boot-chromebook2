/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_NVSTORAGE_H_
#define CHROMEOS_NVSTORAGE_H_

#include <vboot_api.h>
#include <crossystem_data.h>

/*
 * The VbNvContext is stored in block 0, which is also the MBR on x86
 * platforms but generally unused on ARM platforms.  Given this, it is not a
 * perfect place for storing stuff, but since there are no fixed blocks that we
 * may use reliably, block 0 is our only option left.
 */
#define CHROMEOS_VBNVCONTEXT_LBA	0

int nvstorage_init(void);

/**
 * Get current type of non-volatile storage, which is a 8-bit enum value
 * defined in crossystem.h.
 *
 * @return	Current type of non-volatile storage.
 */
uint8_t nvstorage_get_type(void);

/**
 * Set new storage type; if new type is different from old, driver is reloaded.
 *
 * @param type	New storage type, which is a 8-bit enum value defined in
 *		crossystem.h.
 * @return	0 on success, non-0 on error.
 */
int nvstorage_set_type(uint8_t type);

struct nvstorage_method {
	const char *name;
	enum vboot_nvstorage_type type;
	VbError_t (*read)(uint8_t *buf);
	VbError_t (*write)(const uint8_t *buf);
};

/* Declare a non-volatile storage method, capable of accessing vb context */
#define CROS_NVSTORAGE_METHOD(_name) \
	ll_entry_declare(struct nvstorage_method, _name, nvstorage_method)

#endif /* CHROMEOS_NVSTORAGE_H_ */
