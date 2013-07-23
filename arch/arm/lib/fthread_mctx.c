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
#include <malloc.h>
#include <linux/stddef.h>

#ifdef CONFIG_SYS_THUMB_BUILD

#define ARM(x...)
#define THUMB(x...)	x

#else

#define ARM(x...)	x
#define THUMB(x...)

#endif /* CONFIG_SYS_THUMB_BUILD */

struct fthread_mctx {
	long uregs[10];		/* saved registers */
};

#define MCTX_lr		uregs[9]
#define MCTX_sp		uregs[8]
#define MCTX_fp		uregs[7]
#define MCTX_sl		uregs[6]
#define MCTX_r9		uregs[5]
#define MCTX_r8		uregs[4]
#define MCTX_r7		uregs[3]
#define MCTX_r6		uregs[2]
#define MCTX_r5		uregs[1]
#define MCTX_r4		uregs[0]

#define MCTX_UREG_OFFSET	offsetof(struct fthread_mctx, uregs)

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
	int stacksize = (sk_addr_hi - sk_addr_lo) / sizeof(long);
	long *sp = (long *)sk_addr_lo + stacksize - 1;

	memset(mctx, '\0', sizeof(struct fthread_mctx));

	/* Start new thread at func */
	mctx->MCTX_lr = (long)func;

	/* Give the new thread its own stack */
	mctx->MCTX_sp = (long)sp;

	/* Preserve the U-Boot global data pointer */
	__asm__ __volatile__("str r8, %0\n" : "=m" (mctx->MCTX_r8));

	return 0;
}

void fthread_mctx_switch(struct fthread_mctx *octx,
			 struct fthread_mctx *nctx)
{
	__asm__ __volatile__(
	"add		ip, r0, %0\n"
	ARM("stmia	ip!, {r4 - sl, fp, sp, lr}\n")
	THUMB("stmia	ip!, {r4 - sl, fp}\n")
	THUMB("str	sp, [ip], #4\n")
	THUMB("str	lr, [ip], #4\n")
	"add		ip, r1, %0\n"
	ARM("ldmia	ip!, {r4 - sl, fp, sp, lr}\n")
	THUMB("ldmia	ip!, {r4 - sl, fp}\n")
	THUMB("ldr	sp, [ip], #4\n")
	THUMB("ldr	lr, [ip]\n")
	: /* no outputs */
	: "i" (MCTX_UREG_OFFSET) /* offset to register array */
	: "ip"); /* intra-procedure scratch register is clobbered */
}
