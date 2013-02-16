/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#include <common.h>
#include <cros/common.h>
#include <cros/memory_wipe.h>
#include <malloc.h>
#include <physmem.h>

#include <vboot_api.h>

/*
 * This implementation tracks regions of memory that need to be wiped by
 * filling them with zeroes. It does that by keeping a linked list of the
 * edges between regions where memory should be wiped and not wiped. New
 * regions take precedence over older regions they overlap with. With
 * increasing addresses, the regions of memory alternate between needing to be
 * wiped and needing to be left alone. Edges similarly alternate between
 * starting a wipe region and starting a not wiped region.
 */

static void memory_wipe_insert_between(memory_wipe_edge_t *before,
	memory_wipe_edge_t *after, phys_addr_t pos)
{
	memory_wipe_edge_t *new_edge =
		(memory_wipe_edge_t *)malloc(sizeof(*new_edge));

	assert(new_edge);
	assert(before != after);

	new_edge->next = after;
	new_edge->pos = pos;
	before->next = new_edge;
}

void memory_wipe_init(memory_wipe_t *wipe)
{
	wipe->head.next = NULL;
	wipe->head.pos = 0;
}

static void memory_wipe_set_region_to(memory_wipe_t *wipe_info,
	phys_addr_t start, phys_addr_t end, int new_wiped)
{
	/* whether the current region was originally going to be wiped. */
	int wipe = 0;

	assert(start != end);

	/* prev is never NULL, but cur might be. */
	memory_wipe_edge_t *prev = &wipe_info->head;
	memory_wipe_edge_t *cur = prev->next;

	/*
	 * Find the start of the new region. After this loop, prev will be
	 * before the start of the new region, and cur will be after it or
	 * overlapping start. If they overlap, this ensures that the existing
	 * edge is deleted and we don't end up with two edges in the same spot.
	 */
	while (cur && cur->pos < start) {
		prev = cur;
		cur = cur->next;
		wipe = !wipe;
	}

	/* Add the "start" edge between prev and cur, if needed. */
	if (new_wiped != wipe) {
		memory_wipe_insert_between(prev, cur, start);
		prev = prev->next;
	}

	/*
	 * Delete any edges obscured by the new region. After this loop, prev
	 * will be before the end of the new region or overlapping it, and cur
	 * will be after if, if there is a edge after it. For the same
	 * reason as above, we want to ensure that we end up with one edge if
	 * there's an overlap.
	 */
	while (cur && cur->pos <= end) {
		cur = cur->next;
		free(prev->next);
		prev->next = cur;
		wipe = !wipe;
	}

	/* Add the "end" edge between prev and cur, if needed. */
	if (wipe != new_wiped)
		memory_wipe_insert_between(prev, cur, end);
}

/* Set a region to "wiped". */
void memory_wipe_add(memory_wipe_t *wipe, phys_addr_t start, phys_addr_t end)
{
	memory_wipe_set_region_to(wipe, start, end, 1);
}

/* Set a region to "not wiped". */
void memory_wipe_sub(memory_wipe_t *wipe, phys_addr_t start, phys_addr_t end)
{
	memory_wipe_set_region_to(wipe, start, end, 0);
}

/* Actually wipe memory. */
void memory_wipe_execute(memory_wipe_t *wipe)
{
	memory_wipe_edge_t *cur;

	VBDEBUG("Wipe memory regions:\n");
	for (cur = wipe->head.next; cur; cur = cur->next->next) {
		phys_addr_t start, end;

		if (!cur->next) {
			VBDEBUG("Odd number of region edges!\n");
			return;
		}

		start = cur->pos;
		end = cur->next->pos;

		VBDEBUG("\t[%#016llx, %#016llx)\n",
			(uint64_t)start, (uint64_t)end);
		arch_phys_memset(start, 0, end - start);
	}
}
