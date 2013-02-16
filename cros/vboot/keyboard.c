/*
 * Copyright (c) 2011-2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <fdtdec.h>
#include <cros/gbb.h>
#include <cros/keyboard.h>

/* Import the header files from vboot_reference. */
#include <vboot_api.h>
#include <gbb_header.h>

DECLARE_GLOBAL_DATA_PTR;

static struct remap_key remap_keys[KEY_TYPE_COUNT];

int vboot_keymap_init(void)
{
	int croscfg_node = cros_fdtdec_config_node(gd->fdt_blob);

	if (croscfg_node < 0)
		return -1;

	if (!fdtdec_get_int_array(gd->fdt_blob, croscfg_node,
			"faft-key-remap-special",
			remap_keys[KEY_TYPE_SPECIAL].array,
			KEY_ARRAY_SIZE)) {
		remap_keys[KEY_TYPE_SPECIAL].array_ptr =
		remap_keys[KEY_TYPE_SPECIAL].array;
	}

	if (!fdtdec_get_int_array(gd->fdt_blob, croscfg_node,
			"faft-key-remap-ascii",
			remap_keys[KEY_TYPE_ASCII].array,
			KEY_ARRAY_SIZE)) {
		remap_keys[KEY_TYPE_ASCII].array_ptr =
		remap_keys[KEY_TYPE_ASCII].array;
	}

	return 0;
}

/**
 * Replace normal ascii keys and special keys if the mainboard
 * fdt has either an ascii or a special key remap array.
 *
 * @param *c		pointer to key to be checked and possibly remapped
 * @param keytype  KEY_TYPE_ASCII or KEY_TYPE_SPECIAL
 *
 * @return	-1 on error, 0 on success with update int key in c
 */
static int faft_key_remap(int *c, unsigned char keytype)
{
	int  i;
	uint32_t gbb_flags = gbb_get_flags();

	if ((gbb_flags & GBB_FLAG_FAFT_KEY_OVERIDE) == 0)
		return -1;

	if (remap_keys[keytype].array_ptr) {
		for (i = 0; i < KEY_ARRAY_SIZE; i += 2) {
			if (*c == remap_keys[keytype].array_ptr[i]) {
				*c = remap_keys[keytype].array_ptr[i + 1];
				return 0;
			}
		}
	}

	return -1;
}

uint32_t VbExKeyboardRead(void)
{
	int c = 0;

	/* No input available. */
	if (!tstc())
		goto out;

	/* Read a non-Escape character or a standalone Escape character. */
	c = getc();
	if (c != CSI_0 || !tstc()) {
		/* Handle normal asci keys for FAFT keyboard matrix */
		if (faft_key_remap(&c, KEY_TYPE_ASCII) >= 0)
			goto out;

		/*
		 * Special handle of Ctrl-Enter, which is converted into '\n'
		 * by i8042 driver.
		 */
		if (c == '\n')
			c = VB_KEY_CTRL_ENTER;
		goto out;
	}

	/* Filter out non- Escape-[ sequence. */
	if (getc() != CSI_1) {
		c = 0;
		goto out;
	}

	/* Get special keys */
	c = getc();

	/* Handle special keys for FAFT keyboard matrix */
	if (faft_key_remap(&c, KEY_TYPE_SPECIAL) >= 0)
		goto out;


	/* Special values for arrow up/down/left/right. */
	switch (c) {
	case 'A':
		c = VB_KEY_UP;
		break;
	case 'B':
		c = VB_KEY_DOWN;
		break;
	case 'C':
		c = VB_KEY_RIGHT;
		break;
	case 'D':
		c = VB_KEY_LEFT;
		break;
	default:
		/* Filter out speical keys that we do not recognize. */
		c = 0;
		break;
	}

out:
	return c;
}
