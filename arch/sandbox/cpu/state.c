/*
 * Copyright (c) 2011-2012 The Chromium OS Authors.
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
#include <os.h>
#include <asm/state.h>

/* Main state record for the sandbox */
static struct sandbox_state main_state;
static struct sandbox_state *state;	/* Pointer to current state record */

void state_record_exit(enum exit_type_id exit_type)
{
	state->exit_type = exit_type;
}

struct sandbox_state *state_get_current(void)
{
	assert(state);
	return state;
}

int state_init(void)
{
	state = &main_state;

	state->ram_size = CONFIG_SYS_SDRAM_SIZE;
	state->ram_buf = os_malloc(state->ram_size);
	assert(state->ram_buf);

	/*
	 * Example of how to use GPIOs:
	 *
	 * sandbox_gpio_set_direction(170, 0);
	 * sandbox_gpio_set_value(170, 0);
	 */
	return 0;
}

int state_uninit(void)
{
	int err;

	state = &main_state;

	if (state->write_ram_buf) {
		err = os_write_ram_buf(state->ram_buf_fname);
		if (err) {
			printf("Failed to write RAM buffer\n");
			return err;
		}
	}

	return 0;
}
