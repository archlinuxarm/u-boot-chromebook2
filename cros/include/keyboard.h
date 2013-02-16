/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef CHROMEOS_KEYBOARD_H_
#define CHROMEOS_KEYBOARD_H_

/* Control Sequence Introducer for arrow keys */
#define CSI_0		0x1B	/* Escape */
#define CSI_1		0x5B	/* '[' */

/* Types of keys to be overriden */
#define KEY_TYPE_ASCII      0
#define KEY_TYPE_SPECIAL    1
#define KEY_TYPE_COUNT      2

/* Each fdt key array holds three pairs of keys. */
#define KEY_ARRAY_SIZE  (2 * 3)

/*
 * When the remap_key array is initialized, the pointer is set to the begining
 * of the array. If the structure is not initialized, the ptr is NULL.
 */
struct remap_key {
	uint32_t array[KEY_ARRAY_SIZE];
	uint32_t *array_ptr;
};

/**
 * Initialize the remap_key structs from the mainboard fdt arrays.
 *
 * If the array exist in the fdt, it is copied to a remap_key structure
 * and the pointer set to the begining of the array. If there isn't a
 * fdt key map, the ptr is NULL.
 *
 * @return	-1 on error.
 */
int vboot_keymap_init(void);

#endif /* CHROMEOS_KEYBOARD_H_ */
