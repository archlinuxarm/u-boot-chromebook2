/*
 * Copyright (c) 2013 The Chromium OS Authors.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#include <fthread.h>
#include "fthread_priv.h"

void fthread_pqueue_init(struct fthread_pqueue *q)
{
	if (q != NULL) {
		q->q_head = NULL;
		q->q_num = 0;
	}
}

void fthread_pqueue_insert(struct fthread_pqueue *q, int priority,
			   struct fthread *t)
{
	struct fthread *cur;
	int rel_prio;

	assert(q);
	assert(t);

	if (q->q_head == NULL || q->q_num == 0) {
		/* the queue is empty, insert as first element */
		t->q_prev = t;
		t->q_next = t;
		t->q_prio = priority;
		q->q_head = t;
	} else if (q->q_head->q_prio < priority) {
		/* new thread has the highest priority, add as new head */
		t->q_prev = q->q_head->q_prev;
		t->q_next = q->q_head;
		t->q_prev->q_next = t;
		t->q_next->q_prev = t;
		t->q_prio = priority;

		/* update the old head's relative priority */
		t->q_next->q_prio = priority - t->q_next->q_prio;
		q->q_head = t;
	} else {
		/* insert after threads with greater or equal priority */
		cur = q->q_head;
		rel_prio = cur->q_prio;
		while ((rel_prio - cur->q_next->q_prio) >= priority &&
		       cur->q_next != q->q_head) {
			cur = cur->q_next;
			rel_prio -= cur->q_prio;
		}
		t->q_prev = cur;
		t->q_next = cur->q_next;
		t->q_next->q_prev = t;
		t->q_prev->q_next = t;
		t->q_prio = rel_prio - priority;

		/* update the next thread's relative priority if necessary */
		if (t->q_next != q->q_head)
			t->q_next->q_prio -= t->q_prio;
	}

	/* increment the number of threads in the queue */
	q->q_num++;
}

void fthread_pqueue_delete(struct fthread_pqueue *q, struct fthread *t)
{
	assert(q);
	assert(q->q_head);
	assert(t);

	if (q->q_head == t) {
		if (t->q_next == t) {
			/* t is the only thread in the queue */
			t->q_next = NULL;
			t->q_prev = NULL;
			t->q_prio = 0;
			q->q_head = NULL;
		} else {
			/* t is the head of the queue */
			t->q_prev->q_next = t->q_next;
			t->q_next->q_prev = t->q_prev;

			/* make the new head's priority absolute */
			t->q_next->q_prio = t->q_prio - t->q_next->q_prio;
			t->q_prio = 0;
			q->q_head = t->q_next;
		}
	} else {
		/* t is somewhere in the middle of the queue */
		t->q_prev->q_next = t->q_next;
		t->q_next->q_prev = t->q_prev;

		/* make the new head's priority absolute */
		t->q_next->q_prio = t->q_prio - t->q_next->q_prio;
		t->q_prio = 0;
	}
	q->q_num--;
}

struct fthread *fthread_pqueue_pop(struct fthread_pqueue *q)
{
	struct fthread *t;

	assert(q);
	if (q->q_head == NULL) {
		return NULL;
	} else if (q->q_head->q_next == q->q_head) {
		/* there is only one thread in the queue */
		t = q->q_head;
		t->q_next = NULL;
		t->q_prev = NULL;
		t->q_prio = 0;
		q->q_head = NULL;
		q->q_num  = 0;
	} else {
		/* remove the first thread from the queue */
		t = q->q_head;
		t->q_prev->q_next = t->q_next;
		t->q_next->q_prev = t->q_prev;

		/* make the new head's priority absolute */
		t->q_next->q_prio = t->q_prio - t->q_next->q_prio;
		t->q_prio = 0;
		q->q_head = t->q_next;
		q->q_num--;
	}

	return t;
}

void fthread_pqueue_inc_prio(struct fthread_pqueue *q)
{
	assert(q);
	if (q->q_head != NULL) {
		/*
		 * The priority of each of the threads in the queue is relative
		 * to the priority of the head of the queue, which has an
		 * absolute priority. Simply incrementing the absolute priority
		 * of the head of the queue automatically increases the priority
		 * of each of the threads in the queue.
		 */
		q->q_head->q_prio += 1;
	}
}

int fthread_pqueue_length(struct fthread_pqueue *q)
{
	assert(q);
	return q->q_num;
}

struct fthread *fthread_pqueue_head(struct fthread_pqueue *q)
{
	assert(q);
	return q->q_head;
}

struct fthread *fthread_pqueue_tail(struct fthread_pqueue *q)
{
	assert(q);
	if (q->q_head == NULL)
		return NULL;
	return q->q_head->q_prev;
}

struct fthread *fthread_pqueue_walk(struct fthread_pqueue *q,
				    struct fthread *t, int dir)
{
	struct fthread *tn;

	if (q == NULL || t == NULL)
		return NULL;

	tn = NULL;
	if (dir == FTHREAD_WALK_PREV) {
		if (t != q->q_head)
			tn = t->q_prev;
	} else if (dir == FTHREAD_WALK_NEXT) {
		tn = t->q_next;
		if (tn == q->q_head)
			tn = NULL;
	}
	return tn;
}

bool fthread_pqueue_contains(struct fthread_pqueue *q, struct fthread *t)
{
	struct fthread *tc;

	for (tc = fthread_pqueue_head(q); tc != NULL;
	     tc = fthread_pqueue_walk(q, tc, FTHREAD_WALK_NEXT)) {
		if (tc == t)
			return true;
	}
	return false;
}
