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
 */

#ifndef _FTHREAD_H
#define _FTHREAD_H

/* thread priority values */
#define FTHREAD_PRIO_MIN		(-5)
#define FTHREAD_PRIO_STD		0
#define FTHREAD_PRIO_MAX		(+5)

/* max name length for a thread */
#define FTHREAD_TCB_NAMELEN		20

/* stack size */
#define FTHREAD_DEFAULT_STACKSIZE	(64 * 1024)

/* forward declaration of basic thread type */
struct fthread;

/**
 * fthread_init() - Initialize the threading library
 *
 * Initilize the threading library.  This function must be called before any
 * other fthread function is called.
 *
 * @return 0 if successful
 */
int fthread_init(void);

/**
 * fthread_shutdown() - Shut down the threading library
 *
 * Shut down the threading library.  Implicitly kill all the running threads and
 * transform the application back into single-threaded execution.  This must be
 * the last API call made by the user application and must be called from the
 * same thread that called fthread_init()
 *
 * @return 0 if successful
 */
int fthread_shutdown(void);

/**
 * fthread_report() - Report statistics
 *
 * Report running statistics on all the threads in the system.  Can be called by
 * any thread.
 *
 * @return 0 on success, -EPERM if fthread is not initialized
 */
int fthread_report(void);

/**
 * fthread_spawn() - Spawn a new thread of execution
 *
 * Spawn a new thread of execution which calls @a func the first time it is
 * run and directly pass @a arg as a parameter to it.
 *
 * @func:	Function called by the newly spawned thread
 * @arg:	Parameter that is directly passed to func() when it is called
 * @pre_start:	Function that will be called every time before the thread is
 *		scheduled to run.  This function should take care of properly
 *		preserving and restoring any relevant global state variables
 *		that may have changed while the thread was sleeping.
 * @post_stop:	Function that will be called every time after this thread
 *		returns control to the scheduler.  It should properly preserve
 *		and restore any global state variables that may have been
 *		changed by the thread.
 * @context:	Argument that is passed to both pre_start() and post_stop().
 *		This will typically be a struct that holds information on any
 *		global state variables that might be modified by the thread.
 * @prio:	Base priority of the newly spawned thread.  Must be between
 *		@a FTHREAD_PRIO_MIN and @a FTHREAD_PRIO_MAX
 * @name:	Name of the newly spawned thread
 * @stacksize:	Size of the stack that the spawned thread should have
 * @threadp:	Pointer to the location where the spawned thread will be stored
 * @return 0 if successful
 */
int fthread_spawn(void *(*func)(void *), void *arg, void (*pre_start)(void *),
		  void (*post_stop)(void *), void *context, int prio,
		  const char *name, size_t stacksize, struct fthread **threadp);

/**
 * fthread_yield() - Explicitly yield control back to the scheduler.
 */
void fthread_yield(void);

/**
 * fthread_usleep() - Sleep for @a waittime microseconds
 *
 * Implicitly hand control back to the scheduler and tell it not to schedule
 * the calling thread again until at least @a waittime microseconds have
 * passed.
 *
 * @waittime:	Number of microseconds to sleep for
 * @return Number of microseconds that this thread was actually sleeping
 */
unsigned long fthread_usleep(unsigned long waittime);

/**
 * fthread_msleep() - Sleep for @a waittime milliseconds
 *
 * Implicitly hand control back to the scheduler and tell it not to schedule
 * the calling thread again until at least @a waittime milliseconds have
 * passed.
 *
 * @waittime:	Number of milliseconds to sleep for
 * @return Number of milliseconds that this thread was actually sleeping
 */
static inline unsigned long fthread_msleep(unsigned long waittime)
{
	return fthread_usleep(waittime * 1000) / 1000;
}

/**
 * fthread_sleep() - Sleep for @a waittime seconds
 *
 * Implicitly hand control back to the scheduler and tell it not to schedule
 * the calling thread again until at least @a waittime seconds have passed.
 *
 * @waittime:	Number of seconds to sleep for
 * @return Number of seconds that this thread was actually sleeping
 */
static inline unsigned long fthread_sleep(unsigned long waittime)
{
	return fthread_usleep(waittime * 1000000) / 1000000;
}

/**
 * fthread_join() - Wait until @a tid has terminated
 *
 * Implicitly hand control back to the scheduler and tell it not to schedule
 * the calling thread until @a tid has terminated.  Once @a tid does terminate,
 * its return value is stored in @a value.  If @a value is NULL, then the return
 * value is discarded.
 *
 * @tid:	Thread that the calling thread should wait on
 * @value:	Pointer to a location where @a tid's return value shold be
 *		stored
 * @return 0 if successful
 */
int fthread_join(struct fthread *tid, void **value);

/**
 * fthread_exit() - Terminate the current thread and return @a value
 *
 * Terminates the currently executing thread and stores @a value as its return
 * value.  This function is called implicitly whenever a thread is created with
 * fthread_spawn() so simply returning from the thread is enough to terminate it
 *
 * @value:	Return value of the current thread
 */
void fthread_exit(void *value);

#endif /* _FTHREAD_H */
