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
#include <errno.h>
#include <malloc.h>

#include <fthread.h>
#include "fthread_priv.h"

/* thread library should only be initialized once */
static bool fthread_initialized;

int fthread_init(void)
{
	int err;

	if (fthread_initialized)
		return -EBUSY;

	debug("%s: enter\n", __func__);

	/* initilize the scheduler */
	fthread_scheduler_init();

	/* spawn the scheduler thread */
	err = fthread_spawn(fthread_scheduler, NULL, FTHREAD_PRIO_MAX,
			    "**SCHEDULER**", FTHREAD_DEFAULT_STACKSIZE,
			    &fthread_sched);
	if (err) {
		fthread_scheduler_kill();
		return err;
	}

	/* spawn the main thread, which should have a stack size of 0 */
	err = fthread_spawn(FTHREAD_MAIN_FUNC, NULL, FTHREAD_PRIO_STD,
			    "**main**", 0, &fthread_main);
	if (err) {
		fthread_scheduler_kill();
		return err;
	}

	fthread_initialized = true;

	/*
	 * We need to manually switch to the scheduler thread the first time to
	 * start threading.  Since the main thread is currently the only thread
	 * in the scheduler, we should come back immediately.  We also need to
	 * initialize fthread_current here so that the scheduler function is
	 * spawned correctly.
	 */
	fthread_current = fthread_sched;
	fthread_mctx_switch(fthread_main->mctx, fthread_sched->mctx);

	/* we will come right back to here after the scheduler initializes */
	debug("%s: exit\n", __func__);
	return 0;
}

int fthread_shutdown(void)
{
	if (!fthread_initialized)
		return -EPERM;
	if (fthread_current != fthread_main)
		return -EPERM;

	debug("%s: enter\n", __func__);
	fthread_scheduler_kill();
	fthread_initialized = false;
	fthread_tcb_free(fthread_sched);
	fthread_tcb_free(fthread_main);

	debug("%s: exit\n", __func__);
	return 0;
}

int fthread_tcb_alloc(unsigned int stacksize, struct fthread **threadp)
{
	struct fthread *t = malloc(sizeof(struct fthread));

	if (t == NULL)
		return -ENOMEM;

	t->stacksize = stacksize;
	t->stack = NULL;

	if (stacksize > 0) { /* stacksize == 0 => main thread */
		t->stack = malloc(stacksize);
		if (t->stack == NULL) {
			free(t);
			return -ENOMEM;
		}
	}

	t->mctx = fthread_mctx_alloc();
	if (t->mctx == NULL) {
		free(t->stack);
		free(t);
		return -ENOMEM;
	}

	*threadp = t;

	return 0;
}

void fthread_tcb_free(struct fthread *t)
{
	fthread_mctx_free(t->mctx);
	free(t->stack);
	free(t);
}

/**
 * fthread_spawn_helper() - Jump start a new thread of execution.
 */
static void fthread_spawn_helper(void)
{
	void *data;

	data = (*fthread_current->start_func)(fthread_current->start_arg);

	/* do an implicit exit with the return value */
	fthread_exit(data);
}

int fthread_spawn(void *(*func)(void *), void *arg, int prio, const char *name,
		  size_t stacksize, struct fthread **threadp)
{
	unsigned long time;
	char *sk_addr_hi;
	struct fthread *t;
	int err;

	debug("%s: spawning thread \"%s\"\n", __func__, name);
	if (func == NULL)
		return -EINVAL;

	/* special case of the main thread */
	if (func == FTHREAD_MAIN_FUNC)
		func = NULL;

	if (fthread_tcb_alloc(stacksize, &t))
		return -ENOMEM;

	t->prio = prio;
	strncpy(t->name, name, FTHREAD_TCB_NAMELEN);

	/* initialize time points */
	time = fthread_get_current_time_us();
	t->spawned_us = time;
	t->lastran_us = time;
	t->running_us = 0;

	/* initialize starting and ending values */
	t->start_func = func;
	t->start_arg = arg;
	t->join_arg = NULL;

	/* initialize machine state */
	if (stacksize > 0) {	/* The main thread has stacksize == 0 */
		sk_addr_hi = t->stack + t->stacksize;
		err = fthread_mctx_set(t->mctx, fthread_spawn_helper, t->stack,
				       sk_addr_hi);
		if (err) {
			fthread_tcb_free(t);
			return err;
		}
	}

	/* Add the thread to the new queue so the scheduler will pick it up */
	if (func != fthread_scheduler) {
		t->state = FTHREAD_STATE_NEW;
		fthread_pqueue_insert(&fthread_nq, t->prio, t);
	}

	*threadp = t;
	debug("%s: exit\n", __func__);
	return 0;
}

inline void fthread_yield(void)
{
	debug("%s: thread \"%s\" giving up control to scheduler\n",
	      __func__, fthread_current->name);
	fthread_mctx_switch(fthread_current->mctx, fthread_sched->mctx);
	debug("%s: returning to thread \"%s\"\n",  __func__,
	      fthread_current->name);
}

void fthread_usleep(unsigned long waittime)
{
	if (waittime != 0) {
		fthread_current->state = FTHREAD_STATE_WAITING;
		fthread_current->waitevent = FTHREAD_EVENT_SLEEP;
		fthread_current->ev_time = waittime;
		fthread_current->ev_tid = NULL;
		fthread_current->ev_func = NULL;
		fthread_yield();
	}
}

int fthread_join(struct fthread *tid, void **value)
{
	debug("%s: joining thread \"%s\"\n", __func__, tid->name);
	if (tid == NULL)
		return -EINVAL;
	if (tid == fthread_current)
		return -EDEADLK;

	if (tid->state != FTHREAD_STATE_DEAD) {
		fthread_current->state = FTHREAD_STATE_WAITING;
		fthread_current->waitevent = FTHREAD_EVENT_JOIN;
		fthread_current->ev_tid = tid;
		fthread_current->ev_time = 0;
		fthread_current->ev_func = NULL;
		fthread_yield();
	}

	if (value != NULL)
		*value = tid->join_arg;
	fthread_pqueue_delete(&fthread_dq, tid);
	fthread_tcb_free(tid);

	return 0;
}

/**
 * fthread_exit_main_cb() - Callback to check when main thread should exit
 *
 * Special predicate function to wait until @a fthread_main is the only thread
 * left in the system.  This function must be called from _within_ the scheduler
 * so that fthread_scheduler_eventmanager() can figure out whether or not to
 * wake up the main thread, which will then terminate the application.
 *
 * @return true if there is only one thread left in the system, false otherwise
 */
static int fthread_exit_main_cb(void)
{
	int count = 0;

	count += fthread_pqueue_length(&fthread_nq);
	count += fthread_pqueue_length(&fthread_rq);
	count += fthread_pqueue_length(&fthread_wq);

	if (count == 1) /* only the main thread is left */
		return true;
	else
		return false;
}

void fthread_exit(void *value)
{
	debug("%s: thread \"%s\" has terminated\n", __func__,
	      fthread_current->name);
	/*
	 * The main thread is special, because its termination would terminate
	 * the whole program, so we use a special predicate function event to
	 * wait until the main thread is the only thread left in the program,
	 * then cleanup and exit.
	 */
	if (fthread_current != fthread_main) {
		/*
		 * Mark the current thread as dead and switch to the scheduler,
		 * which will clean up after us.  Control should never return
		 */
		fthread_current->join_arg = value;
		fthread_current->state = FTHREAD_STATE_DEAD;
		debug("%s: switching from \"%s\" to scheduler\n",
		      __func__, fthread_current->name);
		fthread_yield();
	} else {
		if (!fthread_exit_main_cb()) {
			fthread_current->state = FTHREAD_STATE_WAITING;
			fthread_current->waitevent = FTHREAD_EVENT_FUNC;
			fthread_current->ev_func = fthread_exit_main_cb;
			fthread_current->ev_time = 0;
			fthread_current->ev_tid = NULL;
			fthread_yield();
		}

		fthread_shutdown();
	}
}
