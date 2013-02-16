/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/*
 * Memory wipe library for easily set and exclude memory regions that need
 * to be cleared.
 *
 * The following methods must be called in order:
 *   memory_wipe_init
 *   memory_wipe_exclude
 *   memory_wipe_execute
 */

#ifndef CHROMEOS_MEMORY_WIPE_H_
#define CHROMEOS_MEMORY_WIPE_H_

#include <asm/types.h>
#include <linux/types.h>

/* A node in a linked list of edges, each at position "pos". */
typedef struct memory_wipe_edge_t {
	struct memory_wipe_edge_t *next;
	phys_addr_t pos;
} memory_wipe_edge_t;

/*
 * Data describing memory to wipe. Contains a linked list of edges between the
 * regions of memory to wipe and not wipe.
 */
typedef struct memory_wipe_t {
	memory_wipe_edge_t head;
} memory_wipe_t;

/*
 * Initializes the memory region that needs to be cleared.
 *
 * @param wipe		Wipe structure to initialize.
 */
void memory_wipe_init(memory_wipe_t *wipe);

/*
 * Adds a memory region to be cleared.
 *
 * @param wipe		Wipe structure to add the region to.
 * @param start		The start of the region.
 * @param end		The end of the region.
 */
void memory_wipe_add(memory_wipe_t *wipe, phys_addr_t start, phys_addr_t end);

/*
 * Subtracts a memory region.
 *
 * @param wipe		Wipe structure to subtract the region from.
 * @param start		The start of the region.
 * @param end		The end of the region.
 */
void memory_wipe_sub(memory_wipe_t *wipe, phys_addr_t start, phys_addr_t end);

/*
 * Executes the memory wipe.
 *
 * @param wipe		Wipe structure to execute.
 */
void memory_wipe_execute(memory_wipe_t *wipe);

#endif /* CHROMEOS_MEMORY_WIPE_H */
