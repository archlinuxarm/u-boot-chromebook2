/*
 * Copyright (c) 2013, Henrik Nordstrom <henrik@henriknordstrom.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SANDBOX_BLOCK_DEV__
#define __SANDBOX_BLOCK_DEV__

struct host_block_dev {
	block_dev_desc_t blk_dev;
	char *filename;
	int fd;
};

int host_dev_bind(int dev, char *filename);

#endif
