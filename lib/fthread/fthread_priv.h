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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Private header for fthread library internals.
 */

#ifndef _FTHREAD_PRIV_H
#define _FTHREAD_PRIV_H

#include <stdlib.h>

#include <fthread.h>

/**
 * Unique signature for the main function so that fthread_spawn() will recognize
 * it and not treat it like a normal thread
 */
#define FTHREAD_MAIN_FUNC	(void *(*)(void *))(-1)

/* Thread state information */
enum fthread_state {
	FTHREAD_STATE_SCHEDULER = 0,
	FTHREAD_STATE_NEW,
	FTHREAD_STATE_READY,
	FTHREAD_STATE_WAITING,
	FTHREAD_STATE_DEAD
};

/* Thread event information */
enum fthread_event {
	FTHREAD_EVENT_NONE = 0,
	FTHREAD_EVENT_SLEEP,
	FTHREAD_EVENT_JOIN,
	FTHREAD_EVENT_FUNC
};

/**
 * struct fthread_mctx - Holds all the information necessary to save and load
 * contexts.  This will be different for different architectures and so must be
 * provided by any architecture that wants to use fthreads.
 */
struct fthread_mctx;

/**
 * struct fthread - Holds the information related to a single thread
 *
 * @q_next:	Next thread in the priority queue
 * @q_prev:	Previous thread in the priority queue
 * @q_prio:	Relative priority of this thread
 * @prio:	Base priority of this thread
 * @name:	Name of this thread (mainly for debugging)
 * @dispatches:	Total number of dispatches
 * @state:	Current state of this thread
 * @spawned_us:	Time at which this thread was spawned (in microseconds)
 * @lastran_us:	Time at which this thread was last running (in microseconds)
 * @running_us:	Number of microseconds that this thread has been running
 * @waitevent:	The type of the event this thread is waiting for, if any
 * @ev_time_us:	Number of microseconds that this thread is sleeping for
 * @ev_tid:	Thread whose termination this thread is waiting for
 * @ev_func:	Predicate function which will return !0 when this thread should
 *		wake up
 * @mctx:	Pointer to the machine-dependent context information
 * @stack	Pointer to the lowest memory location of the thread stack
 * @stacksize:	Size of the current thread's stack
 * @start_func:	Function to be called when this thread is first run
 * @start_arg:	Argument that is directly passed to @a start_func
 * @join_arg:	Value that is returned when this thread joins with another
 *		thread
 */
struct fthread {
	/* priority queue handling */
	struct fthread		*q_next;
	struct fthread		*q_prev;
	int			q_prio;

	/* thread control block information */
	int			prio;
	char			name[FTHREAD_TCB_NAMELEN];
	int			dispatches;
	enum fthread_state	state;

	/* timing */
	unsigned long		spawned_us;
	unsigned long		lastran_us;
	unsigned long		running_us;

	/* event handling */
	enum fthread_event	waitevent;
	unsigned long		ev_time;
	struct fthread		*ev_tid;
	int			(*ev_func)(void);

	/* machine context */
	struct fthread_mctx	*mctx;
	char			*stack;
	size_t			stacksize;
	void			*(*start_func)(void *);
	void			*start_arg;
	void			*join_arg;
};

/**
 * struct fthread_pqueue - Holds a list of threads sorted by priority
 *
 * @q_head:	The thread in the current queue with the highest priority
 * @q_num:	The number of threads currently in this queue
 */
struct fthread_pqueue {
	struct fthread	*q_head;
	int		q_num;
};

/* walk direction when stepping through a priority queue */
#define FTHREAD_WALK_NEXT	(1 << 0)
#define FTHREAD_WALK_PREV	(1 << 1)

/* Scheduler variables */
extern struct fthread		*fthread_main;		/* Main thread      */
extern struct fthread		*fthread_sched;		/* Scheduler thread */
extern struct fthread		*fthread_current;	/* Current thread   */
extern struct fthread_pqueue	fthread_nq;		/* New queue        */
extern struct fthread_pqueue	fthread_rq;		/* Ready queue      */
extern struct fthread_pqueue	fthread_wq;		/* Waiting queue    */
extern struct fthread_pqueue	fthread_dq;		/* Dead queue       */

/**
 * fthread_scheduler_init() - Initialize the scheduler
 *
 * Initialize the scheduler and all the priority queues.
 */
void fthread_scheduler_init(void);

/**
 * fthread_scheduler_kill() - Kill the scheduler
 *
 * Kill the scheduler and clean up the memory used by all threads except the
 * main thread.
 */
void fthread_scheduler_kill(void);

/**
 * fthread_scheduler_eventmanager() - Handle thread wait events
 *
 * Handle all wait events and wake threads up as necessary.  If @a block is true
 * then block execution until there is at least one thread ready to be woken up.
 *
 * @now:	The current time (in microseconds)
 * @block:	Whether the event manager should block until at least one thread
 *		can be woken up
 */
void fthread_scheduler_eventmanager(unsigned long now_us, bool block);

/**
 * fthread_scheduler() - The main scheduler algorithm
 *
 * The scheduler is responsible for managing all the threads in the system and
 * keeps several different priority queues.  When threads are initially spawned,
 * they are placed on the new queue.
 *
 * The scheduler works inside an infinite loop.  It first moves all the threads
 * from the new queue onto the ready queue.  It then removes the thread with the
 * highest priority from the ready queue and hands control off to that thread.
 * The next time control returns to the scheduler, it moves the thread onto the
 * dead queue if it terminated or the waiting queue if it wants to wait for some
 * event.  To ensure that none of the threads in the ready queue starve, the
 * scheduler increases the priority of all the threads in the ready queue by
 * one.  If the thread that returned did not terminate and is not waiting for an
 * event, then the scheduler puts it back into the ready queue.  Finally, it
 * calls fthread_scheduler_eventmanager() to see if any of the threads on the
 * waiting queue should be woken up and then repeats the loop.
 *
 * @unused:	Unused parameter necessary so the scheduler's signature
 *			matches the signature expected by fthread_spawn()
 */
void *fthread_scheduler(void *unused) __attribute__((noreturn));

/**
 * fthread_pqueue_init() - Initialize the priority queue @q
 *
 * Initialize the priority queue @a q.  Set @a q->q_head to NULL and @a q->q_num
 * to 0.
 *
 * @q::	Pointer to the priority queue to be initialized
 */
void fthread_pqueue_init(struct fthread_pqueue *q);

/**
 * fthread_pqueue_insert() - Insert thread @t into priority queue @q
 *
 * Insert thread @a t with priority @a prio into the priority queue @q.
 *
 * @q:		Priority queue into which @a t should be inserted
 * @priority:	Initial priority that @a t should be inserted with
 * @t:		Thread that will be inserted into @a q
 */
void fthread_pqueue_insert(struct fthread_pqueue *q, int priority,
			   struct fthread *t);

/**
 * fthread_pqueue_delete() - Remove thread @a t from priority queue @a q.
 *
 * @q:	Queue from which @a t should be removed
 * @t:	Thread that will be removed from @a q
 */
void fthread_pqueue_delete(struct fthread_pqueue *q, struct fthread *t);

/**
 * fthread_pqueue_pop() - Remove the thread with the highest priority from @a q
 *
 * @q:	Queue from which we should remove the thread
 * @return Pointer to the thread with the highest priority in @a q or NULL if
 *	   @a q is empty */
struct fthread *fthread_pqueue_pop(struct fthread_pqueue *q);

/**
 * fthread_pqueue_inc_prio() - Increase the priority of all the threads in @a q
 *
 * @q:	Queue whose threads' priorities we should increase
 */
void fthread_pqueue_inc_prio(struct fthread_pqueue *q);

/**
 * fthread_pqueue_length() - Return the number of threads in @a q.
 *
 * @q:	Queue whose size we want to find out
 * @return Number of threads in @a q
 */
int fthread_pqueue_length(struct fthread_pqueue *q);

/**
 * fthread_pqueue_contains() - Check if @a q contains @a t.
 *
 * @q:	Queue that we wish to check
 * @t:	Thread that we wish to check for
 * @return 1 if @a q contains @a t, 0 otherwise
 */
bool fthread_pqueue_contains(struct fthread_pqueue *q, struct fthread *t);

/**
 * fthread_pqueue_head() -  Get the thread with the highest priority in @a q
 *
 * Get the thread with the highest priority in @a q.  The thread is NOT removed
 * from @a q.
 *
 * @q:	Queue whose head we want
 * @return Pointer to the thread with the highest priority in @a q, NULL if @a q
 *	   is empty or uninitialized
 */
struct fthread *fthread_pqueue_head(struct fthread_pqueue *q);

/**
 * fthread_pqueue_tail() - Get the thread with the lowest priority in @a q
 *
 * Get the thread with the lowest priority in @a q.  The thread is NOT removed
 * from @a q.
 *
 * @q:	Queue whose tail we want
 * @return Pointer to the thread with the lowest priority in @a q, NULL if @a q
 *	   is empty or uninitialized
 */
struct fthread *fthread_pqueue_tail(struct fthread_pqueue *q);

/**
 * fthread_pqueue_walk() - Traverse the priority queue @a q in the direction @a
 *			   dir from thread @a t.
 *
 * @q:		The priority queue we want to traverse
 * @t:		The thread from which we should start traversing
 * @dir:	The direction in which we should traverse @a q.  Must be either
 *		FTHREAD_WALK_NEXT or FTHREAD_WALK_PREV.
 * @return Pointer to the next thread in @a q in the direction specified or NULL
 *	   if we have reached the end of the queue
 */
struct fthread *fthread_pqueue_walk(struct fthread_pqueue *q, struct fthread *t,
				    int dir);

/**
 * fthread_get_current_time_us() - Return the current time in microseconds
 *
 * @return Current time in microseconds
 */
static inline unsigned long fthread_get_current_time_us(void)
{
	return timer_get_us();
}

/**
 * fthread_tcb_alloc() - Allocate memory for a thread control block
 *
 * Allocate memory for a thread control block with a stack of @a stacksize bytes
 *
 * @stacksize:	The size of the stack that the thread should have
 * @threadp:	Pointer to memory location where the allocated thread
 *		control block address should be saved
 * @return 0 if successful
 */
int fthread_tcb_alloc(unsigned int stacksize, struct fthread **threadp);

/**
 * fthread_tcb_free() - Free an allocated thread control block
 *
 * Free the memory allocated for the thread control block @a t
 *
 * @t:	Pointer to the thread control block that should be freed
 */
void fthread_tcb_free(struct fthread *t);

/**
 * fthread_mctx_alloc() - Allocate memory for the machine-dependent context
 *
 * Allocate memory for the machine-dependent context.  This must be provided by
 * every architexture that wants to use fthreads.
 *
 * @return Pointer to allocated machine context or NULL if there was an error
 */
struct fthread_mctx *fthread_mctx_alloc(void);

/**
 * fthread_mctx_free() - Free an allocated machine-dependent context
 *
 * Free the memory allocated for the machine-dependent context @a mctx.  This
 * must be provided by every architecture that wants to use fthreads.
 *
 * @mctx:	Pointer to the machine-dependent context to be freed
 */
void fthread_mctx_free(struct fthread_mctx *mctx);

/**
 * fthread_mctx_set() - Store context information in @a mctx
 *
 * Store context information in @a mctx such that when it is restored, execution
 * will start from the beginning of @a func.  This must be provided by every
 * architecture that wants to use fthreads.
 *
 * @mctx:		Pointer to the location where the machine-dependent
 *			context will be stored
 * @func:		Function that should start executing when @a mctx is
 *			restored
 * @sk_addr_lo:		Pointer to the lowest memory address that will be used
 *			as the stack for @a mctx
 * @sk_addr_hi:		Pointer to the highest memory address that will be used
 *			as the stack for @a mctx
 * @return 0 if successful
 */
int fthread_mctx_set(struct fthread_mctx *mctx, void (*func)(void),
		     char *sk_addr_lo, char *sk_addr_hi);

/**
 * fthread_mctx_switch() - Switch machine contexts
 *
 * Save the current context in @a octx and load the context from @a nctx.  This
 * must be provided by every architecture that wants to use fthreads.
 *
 * @octx:	Pointer to memory location where the current context will be
 *		stored
 * @nctx:	Pointer to memory location from which the new context will be
 *		loaded
 */
void fthread_mctx_switch(struct fthread_mctx *octx, struct fthread_mctx *nctx);
#endif /* _FTHREAD_PRIV_H */
