/*
 * Copyright (C) 2013 The ChromiumOS Authors.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 */

#ifndef ELOG_INTERNAL_H_
#define ELOG_INTERNAL_H_

#include <elog.h>
#include <linux/compiler.h>

/* SMBIOS Type 15 related constants */
#define ELOG_HEADER_TYPE_OEM		0x88

enum elog_area_state {
	ELOG_AREA_UNDEFINED,		/* Initial boot strap state */
	ELOG_AREA_EMPTY,		/* Entire area is empty */
	ELOG_AREA_HAS_CONTENT,		/* Area has some content */
};

enum elog_header_state {
	ELOG_HEADER_INVALID,
	ELOG_HEADER_VALID,
};

enum elog_event_buffer_state {
	ELOG_EVENT_BUFFER_OK,
	ELOG_EVENT_BUFFER_CORRUPTED,
};

struct elog_area {
	struct elog_header header;
	u8 data[0];
} __packed;

#endif /* ELOG_INTERNAL_H_ */
