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

#ifndef __SANDBOX_STATE_H
#define __SANDBOX_STATE_H

#include <config.h>
#include <stdbool.h>

/* How we exited U-Boot */
enum exit_type_id {
	STATE_EXIT_NORMAL,
	STATE_EXIT_COLD_REBOOT,
	STATE_EXIT_POWER_OFF,
};

struct sandbox_spi_info {
	const char *spec;
	const struct sandbox_spi_emu_ops *ops;
};

/* The complete state of the test system */
struct sandbox_state {
	const char *cmd;		/* Command to execute */
	bool interactive;		/* Enable cmdline after execute */
	const char *fdt_fname;		/* Filename of FDT binary */
	enum exit_type_id exit_type;	/* How we exited U-Boot */
	const char *parse_err;		/* Error to report from parsing */
	int argc;			/* Program arguments */
	char **argv;
	uint8_t *ram_buf;		/* Emulated RAM buffer */
	unsigned int ram_size;		/* Size of RAM buffer */
	const char *ram_buf_fname;	/* Filename to use for RAM buffer */
	bool write_ram_buf;		/* Write RAM buffer on exit */

	/* Pointer to information for each SPI bus/cs */
	struct sandbox_spi_info spi[CONFIG_SANDBOX_SPI_MAX_BUS]
					[CONFIG_SANDBOX_SPI_MAX_CS];
};

/**
 * Record the exit type to be reported by the test program.
 *
 * @param exit_type	Exit type to record
 */
void state_record_exit(enum exit_type_id exit_type);

/**
 * Gets a pointer to the current state.
 *
 * @return pointer to state
 */
struct sandbox_state *state_get_current(void);

/**
 * Initialize the test system state
 */
int state_init(void);

#endif
