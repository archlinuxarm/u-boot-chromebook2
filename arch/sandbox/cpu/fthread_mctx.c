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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

struct fthread_mctx {
	ucontext_t uc;
};

struct fthread_mctx *fthread_mctx_alloc(void)
{
	return malloc(sizeof(struct fthread_mctx));
}

void fthread_mctx_free(struct fthread_mctx *mctx)
{
	free(mctx);
}

int fthread_mctx_set(struct fthread_mctx *mctx, void (*func)(void),
		     char *sk_addr_lo, char *sk_addr_hi)
{
	/* fetch current context */
	if (getcontext(&mctx->uc) != 0) /* getcontext will set errno for us */
		return -errno;

	/* remove parent link */
	mctx->uc.uc_link = NULL;

	/* configure new stack */
	mctx->uc.uc_stack.ss_sp = sk_addr_lo;
	mctx->uc.uc_stack.ss_size = (sk_addr_hi - sk_addr_lo);
	mctx->uc.uc_stack.ss_flags = 0;

	/* configure new startup function with no arguments */
	makecontext(&mctx->uc, func, 0);

	return 0;
}

void fthread_mctx_switch(struct fthread_mctx *octx,
			 struct fthread_mctx *nctx)
{
	if (swapcontext(&octx->uc, &nctx->uc) != 0) {
		printf("FTHREAD ERROR: bad context switch\n");
		exit(EXIT_FAILURE);
	}
}
