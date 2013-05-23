/*
 * Copyright (c) 2013 The Chromium OS Authors.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <initcall.h>

DECLARE_GLOBAL_DATA_PTR;

int initcall_run_list(init_fnc_t init_sequence[])
{
	init_fnc_t *init_fnc_ptr;

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		debug("initcall: %p\n", *init_fnc_ptr);
		if ((*init_fnc_ptr)()) {
			unsigned long reloc_ofs = 0;

			if (gd->flags & GD_FLG_RELOC)
				reloc_ofs = gd->reloc_off;
			printf("initcall sequence %p failed at call %p\n",
			       init_sequence, (char *)*init_fnc_ptr - reloc_ofs);
			return -1;
		}
	}
	return 0;
}
