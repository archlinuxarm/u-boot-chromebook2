/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
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

#include <command.h>
#include <common.h>
#include <errno.h>
#include <fthread.h>

#define FAIL_COND(cond, fmt, args...)					\
	do {								\
		if (cond) {						\
			panic("TEST FAILED: " fmt "\nat %s:%d %s()\n",	\
			      ##args, __FILE__, __LINE__, __func__);	\
		}							\
	} while (0)

/**
 * struct thread_data - basic data type that will hold both the arguments passed
 * to a thread and the result of the thread's computation
 *
 * @val	Initial argument passed to thread
 * @result	Result of the thread's computation
 */
struct thread_data {
	int val;
	int result;
};

/**
 * struct global_state - basic struct to keep track of global state data that
 * may be modified by a thread
 *
 * @old:	Old state data
 * @cur	Current state data
 */
struct global_state {
	int old;
	int cur;
};

/* global variable for testing global state preservation*/
static int global_var;

static void *add_one_thousand(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	int i;

	data->result = data->val;
	for (i = 0; i < 100; i++) {
		printf("child thread working, num = %d\n", data->result);
		data->result += 10;
		fthread_yield();
	}

	return arg;
}

static void *nested_threads(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	struct fthread *tid;
	int err;
	int cur = data->val;

	if (cur < 9) {
		data->val++;
		printf("spawning thread %d\n", cur);
		err = fthread_spawn(nested_threads, arg, NULL, NULL, NULL,
				    FTHREAD_PRIO_STD, "",
				    FTHREAD_DEFAULT_STACKSIZE, &tid);
		FAIL_COND(err, "nested thread spawn, errno = %d", err);

		err = fthread_join(tid, NULL);
		printf("joined with thread %d\n", cur);
		FAIL_COND(err, "joining nested child thread, errno = %d", err);
		data->result *= cur;
	} else {
		data->result = cur;
	}

	return arg;
}

static void *test_square(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;
	int i;

	global_var = 0;
	for (i = 0; i < data->val; i++) {
		global_var += data->val;
		fthread_yield();
	}
	data->result = global_var;

	return arg;
}

static void square_pre_start(void *state_arg)
{
	struct global_state *state = (struct global_state *)state_arg;

	state->old = global_var;
	global_var = state->cur;
}

static void square_post_stop(void *state_arg)
{
	struct global_state *state = (struct global_state *)state_arg;

	state->cur = global_var;
	global_var = state->old;
}

void test_basic_operation(void)
{
	int err;

	printf("\n*** TESTING BASIC OPERATION ***\n\n");
	printf("Initializing library: ");
	printf("spawns main thread and scheduler\n");
	err = fthread_init();
	FAIL_COND(err, "initialization error, errno = %d", err);

	printf("Killing threading library\n");
	err = fthread_shutdown();
	FAIL_COND(err, "unable to kill library, errno = %d", err);

	printf("Re-initializing library\n");
	err = fthread_init();
	FAIL_COND(err, "initialization error, errno = %d", err);
}

void test_basic_thread_operation(void)
{
	int err;
	struct fthread *tid;
	void *ret;
	struct thread_data data;
	const unsigned long sleeptime = 700;
	unsigned long actualsleep;

	printf("\n*** TESTING BASIC THREAD OPERATION ***\n\n");
	printf("Spawning thread\n");
	data.val = 123;
	err = fthread_spawn(add_one_thousand, (void *)&data, NULL, NULL, NULL,
			    FTHREAD_PRIO_STD, "123", FTHREAD_DEFAULT_STACKSIZE,
			    &tid);
	FAIL_COND(err, "simple thread spawn, errno = %d", err);

	printf("Sleeping for %lu microseconds\n", sleeptime);
	actualsleep = fthread_usleep(sleeptime);

	printf("Main thread slept for %lu microseconds, yielding\n",
	       actualsleep);
	fthread_yield();

	printf("Main thread returned from yield, joining with child thread\n");
	err = fthread_join(tid, &ret);
	FAIL_COND(err, "joining spawned thread, errno = %d", err);

	FAIL_COND(((struct thread_data *)ret)->result != 1123,
		  "wrong return value: expected %d, got %d", 1123,
		  ((struct thread_data *)ret)->result);
}

void test_nested_thread_operation(void)
{
	int err;
	struct fthread *tid;
	struct thread_data data;
	void *ret;
	int expected = 1 * 2 * 3 * 4 * 5 * 6 * 7 * 8 * 9;

	printf("\n*** TESTING NESTED THREAD OPERATION ***\n");
	printf("spawning thread %d\n", 0);
	data.val = 1;
	err = fthread_spawn(nested_threads, (void *)&data, NULL, NULL, NULL,
			    FTHREAD_PRIO_STD, "", FTHREAD_DEFAULT_STACKSIZE,
			    &tid);
	FAIL_COND(err, "simple thread spawn, errno = %d", err);

	err = fthread_join(tid, &ret);
	printf("joined with thread %d\n", 0);
	FAIL_COND(err, "joining spawned thread, errno = %d", err);
	FAIL_COND(((struct thread_data *)ret)->result != expected,
		  "wrong return value: expected %d, got %d",
		  expected, ((struct thread_data *)ret)->result);
}

void test_global_state_preservation(void)
{
	int err;
	struct fthread *t1, *t2;
	struct thread_data t1_data, t2_data;
	struct global_state t1_state, t2_state;
	void *ret;
	int t1_expected, t2_expected;

	printf("\n*** TESTING GLOBAL STATE PRESERVATION ***\n");
	global_var = 0xdeadbeef; /* should be preserved after threads */
	t1_data.val = 13;
	t2_data.val = 7;
	t1_expected = t1_data.val * t1_data.val;
	t2_expected = t2_data.val * t2_data.val;

	printf("spawning thread to calculate square of %d\n", t1_data.val);
	err = fthread_spawn(test_square, (void *)&t1_data, square_pre_start,
			    square_post_stop, (void *)&t1_state,
			    FTHREAD_PRIO_STD, "square: thread 1",
			    FTHREAD_DEFAULT_STACKSIZE, &t1);
	FAIL_COND(err, "thread spawn error, errno = %d", err);
	printf("spawning thread to calculate square of %d\n", t2_data.val);
	err = fthread_spawn(test_square, (void *)&t2_data, square_pre_start,
			    square_post_stop, (void *)&t2_state,
			    FTHREAD_PRIO_STD, "square: thread 2",
			    FTHREAD_DEFAULT_STACKSIZE, &t2);
	FAIL_COND(err, "thread spawn error, errno = %d", err);

	err = fthread_join(t1, &ret);
	FAIL_COND(err, "joining spawned thread, errno = %d", err);
	FAIL_COND(((struct thread_data *)ret)->result != t1_expected,
		  "wrong return value: expected %d, got %d",
		  t1_expected, ((struct thread_data *)ret)->result);
	FAIL_COND(global_var != 0xdeadbeef,
		  "global_var is not preserved: expected %d, got %d",
		  0xdeadbeef, global_var);

	err = fthread_join(t2, &ret);
	FAIL_COND(err, "joining spawned thread, errno = %d", err);
	FAIL_COND(((struct thread_data *)ret)->result != t2_expected,
		  "wrong return value: expected %d, got %d",
		  t2_expected, ((struct thread_data *)ret)->result);
	FAIL_COND(global_var != 0xdeadbeef,
		  "global_var is not preserved: expected %d, got %d",
		  0xdeadbeef, global_var);
}

static int do_fthread_test(cmd_tbl_t *cmdtp, int flag, int argc,
			   char * const argv[])
{
	test_basic_operation();

	test_basic_thread_operation();

	test_nested_thread_operation();

	test_global_state_preservation();

	fthread_shutdown();
	printf("\nOK: ALL TESTS PASSED :)\n\n");
	return 0;
}

static void *threadmsg(void *arg)
{
	int i;
	char *msg = (char *)arg;

	for (i = 0; i < 10; i++) {
		printf("%d %s\n", i, msg);
		fthread_yield();
	}

	fthread_exit(NULL);

	hang();	/* should never reach here */
}

static int do_fthread_demo(cmd_tbl_t *cmdtp, int flag, int argc,
			   char * const argv[])
{
	struct fthread *p1;
	struct fthread *p2;
	const char *ping = "ping";
	const char *pong = "pong";
	const char *whoo = "whoo-hoo";
	const unsigned long sleeptime = 2000;
	int err;
	unsigned long actualsleep;

	puts("Initializing threading library...\n");
	err = fthread_init();
	if (err)
		printf("Error during initialization: %d\n", err);

	puts("Spawning ping-pong threads...\n");
	fthread_spawn(threadmsg, (void *)ping, NULL, NULL, NULL,
		      FTHREAD_PRIO_STD, "thread 1", FTHREAD_DEFAULT_STACKSIZE,
		      &p1);
	fthread_spawn(threadmsg, (void *)pong, NULL, NULL, NULL,
		      FTHREAD_PRIO_STD, "thread 2", FTHREAD_DEFAULT_STACKSIZE,
		      &p2);

	puts("Waiting to join with spawned threads...\n");
	fthread_join(p1, NULL);
	fthread_join(p2, NULL);

	puts("Spawning whoo-hoo thread...\n");
	fthread_spawn(threadmsg, (void *)whoo, NULL, NULL, NULL,
		      FTHREAD_PRIO_STD, "whoo-hoo", FTHREAD_DEFAULT_STACKSIZE,
		      &p1);

	printf("Main thread sleeping for %lu milliseconds...\n", sleeptime);
	actualsleep = fthread_msleep(sleeptime);

	printf("Main thread slept for %lu milliseconds\n", actualsleep);
	puts("Exiting demo...\n");
	fthread_exit(NULL);

	return 0;
}

#ifdef CONFIG_CMD_FTHREAD_REPORT
static int do_fthread_report(cmd_tbl_t *cmdtp, int flag, int argc,
			      char * const argv[])
{
	fthread_report();
	return 0;
}
#endif

U_BOOT_SUBCMD_START(cmd_fthread_sub)
	U_BOOT_CMD_MKENT(demo, 2, 0, do_fthread_demo, "", "")
	U_BOOT_CMD_MKENT(test, 2, 0, do_fthread_test, "", "")
#ifdef CONFIG_CMD_FTHREAD_REPORT
	U_BOOT_CMD_MKENT(report, 2, 0, do_fthread_report, "", "")
#endif
U_BOOT_SUBCMD_END

/*
 * Process a fthread sub-command
 */
static int do_fthread(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	cmd_tbl_t *c;

	/* Strip off leading 'fthread' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], cmd_fthread_sub,
			 ARRAY_SIZE(cmd_fthread_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
}


U_BOOT_CMD(fthread, 2, 1, do_fthread,
	   "multithreading command",
	   " - run a cooperatively multithreaded process\n"
	   "demo     - Run a simple demo\n"
	   "test     - Test the library"
#ifdef CONFIG_CMD_FTHREAD_REPORT
	   "\nreport   - Report thread runtime statistics"
#endif
);
