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

/* scheduler variables */
struct fthread		*fthread_main;
struct fthread		*fthread_sched;
struct fthread		*fthread_current;
struct fthread_pqueue	fthread_nq;
struct fthread_pqueue	fthread_rq;
struct fthread_pqueue	fthread_wq;
struct fthread_pqueue	fthread_dq;

void fthread_scheduler_init(void)
{
	/* initialize important threads */
	fthread_sched = NULL;
	fthread_current = NULL;

	/* initialize the thread queues */
	fthread_pqueue_init(&fthread_nq);
	fthread_pqueue_init(&fthread_rq);
	fthread_pqueue_init(&fthread_wq);
	fthread_pqueue_init(&fthread_dq);
}

void fthread_scheduler_kill(void)
{
	struct fthread *t;

	while ((t = fthread_pqueue_pop(&fthread_nq)) != NULL)
		fthread_tcb_free(t);
	fthread_pqueue_init(&fthread_nq);

	while ((t = fthread_pqueue_pop(&fthread_rq)) != NULL)
		fthread_tcb_free(t);
	fthread_pqueue_init(&fthread_rq);

	while ((t = fthread_pqueue_pop(&fthread_wq)) != NULL)
		fthread_tcb_free(t);
	fthread_pqueue_init(&fthread_wq);

	while ((t = fthread_pqueue_pop(&fthread_dq)) != NULL)
		fthread_tcb_free(t);

	fthread_pqueue_init(&fthread_dq);
}

void fthread_scheduler_eventmanager(unsigned long now, bool block)
{
	struct fthread *t;
	struct fthread *tlast;
	bool wake;
	unsigned long waittime;
	unsigned long minwait = -1;	/* initialize to max long */
	struct fthread *mintid = NULL;

	debug("%s: entering in %s mode\n", __func__, block ?
	      "blocking" : "non-blocking");
	t = fthread_pqueue_head(&fthread_wq);
	while (t != NULL) {
		wake = false;
		switch (t->waitevent) {
		case FTHREAD_EVENT_SLEEP:
			waittime = now - t->lastran_us;
			if (waittime > t->ev_time) {
				wake = true;
			} else if (t->ev_time - waittime < minwait) {
				minwait = t->ev_time - waittime;
				mintid = t;
			}
			break;
		case FTHREAD_EVENT_JOIN:
			if (t->ev_tid->state == FTHREAD_STATE_DEAD)
				wake = true;
			break;
		case FTHREAD_EVENT_FUNC:
			if ((*t->ev_func)())
				wake = true;
			break;
		default:
			break;
		}
		tlast = t;
		t = fthread_pqueue_walk(&fthread_wq, t, FTHREAD_WALK_NEXT);
		if (wake) {
			debug("%s: thread \"%s\" will be woken up\n",
			      __func__, tlast->name);
			tlast->state = FTHREAD_STATE_READY;
			tlast->waitevent = FTHREAD_EVENT_NONE;
			fthread_pqueue_delete(&fthread_wq, tlast);
			fthread_pqueue_insert(&fthread_rq, tlast->prio, tlast);
		}
	}

	if (block && fthread_pqueue_length(&fthread_rq) == 0) {
		/* sleep until at least one thread can wake up */
		debug("%s: sleeping for %lu microseconds\n",
		      __func__, minwait);
		__udelay(minwait);
		mintid->state = FTHREAD_STATE_READY;
		mintid->waitevent = FTHREAD_EVENT_NONE;
		fthread_pqueue_delete(&fthread_wq, mintid);
		fthread_pqueue_insert(&fthread_rq, mintid->prio, mintid);
	}
}

void *fthread_scheduler(void *unused)
{
	unsigned long snapshot;
	struct fthread *t;

	debug("%s: bootstrapping\n", __func__);
	fthread_sched->state = FTHREAD_STATE_SCHEDULER;
	snapshot = fthread_get_current_time_us();

	/* endless scheduler loop */
	while (1) {
		/* Move all threads off the new queue */
		t = fthread_pqueue_pop(&fthread_nq);
		while (t != NULL) {
			debug("%s: thread \"%s\" moved to ready queue\n",
			      __func__, t->name);
			t->state = FTHREAD_STATE_READY;
			fthread_pqueue_insert(&fthread_rq, t->prio, t);
			t = fthread_pqueue_pop(&fthread_nq);
		}

		/* Get the next thread from the ready queue */
		fthread_current = fthread_pqueue_pop(&fthread_rq);
		if (fthread_current == NULL) {
			/*
			 * This should never happen because it would mean that
			 * the ready queue was empty.  The ready queue cannot be
			 * empty during startup because the main thread should
			 * be in the queue.  If the ready queue was empty when
			 * control returned to the scheduler, then the event
			 * manager should block until some thread is ready to
			 * wake up.  If we get here, something has gone very,
			 * very wrong.
			 */
			panic("FTHREAD ERROR: no more threads to schedule\n");
		}

		debug("%s: thread \"%s\" selected (prio=%d, qprio=%d)\n",
		      __func__, fthread_current->name,
		      fthread_current->prio, fthread_current->q_prio);

		/* Update thread state */
		fthread_current->state = FTHREAD_STATE_RUNNING;

		/* Update scheduler times */
		fthread_sched->lastran_us = fthread_get_current_time_us();
		fthread_sched->running_us += (fthread_sched->lastran_us -
					      snapshot);
		debug("%s: running for %lu microseconds\n",
		      __func__, fthread_sched->running_us);

		/* ***CONTEXT SWITCH*** */
		fthread_current->dispatches++;
		fthread_mctx_switch(fthread_sched->mctx, fthread_current->mctx);

		/* Update thread runtime */
		snapshot = fthread_get_current_time_us();
		fthread_current->lastran_us = snapshot;
		fthread_current->running_us += (snapshot -
						fthread_sched->lastran_us);
		debug("%s: thread \"%s\" running for %lu microseconds\n",
		      __func__, fthread_current->name,
		      fthread_current->running_us);

		/* If terminated, move to dead queue */
		if (fthread_current->state == FTHREAD_STATE_DEAD) {
			debug("%s: marking thread \"%s\" as dead\n",
			      __func__, fthread_current->name);
			fthread_pqueue_insert(&fthread_dq, FTHREAD_PRIO_STD,
					      fthread_current);
			fthread_current = NULL;
		}

		/* If waiting, move to waiting queue */
		if (fthread_current != NULL &&
		    fthread_current->state == FTHREAD_STATE_WAITING) {
			debug("%s: moving thread \"%s\" to wait queue\n",
			      __func__, fthread_current->name);
			fthread_pqueue_insert(&fthread_wq, FTHREAD_PRIO_STD,
					      fthread_current);
			fthread_current = NULL;
		}

		/*
		 * Increase the priority of all the threads in the ready queue
		 * to ensure that no thread starves.  Then re-insert the current
		 * thread if it wasn't put on the waiting or dead queues
		 */
		fthread_pqueue_inc_prio(&fthread_rq);
		if (fthread_current != NULL) {
			fthread_current->state = FTHREAD_STATE_READY;
			fthread_pqueue_insert(&fthread_rq,
					      fthread_current->prio,
					      fthread_current);
		}

		if (fthread_pqueue_length(&fthread_rq) == 0 &&
		    fthread_pqueue_length(&fthread_nq) == 0) {
			/*
			 * The event manager should block until some thread is
			 * ready to wake up.
			 */
			fthread_scheduler_eventmanager(snapshot, true);
		} else {
			/*
			 * There are already threads on the ready queue, no need
			 * to block
			 */
			fthread_scheduler_eventmanager(snapshot, false);
		}
	}
}
