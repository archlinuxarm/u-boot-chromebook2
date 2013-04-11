/*
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Tegra cache routines */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch/gp_padctrl.h>

void config_cache(void)
{
	struct apb_misc_gp_ctlr *gp =
		(struct apb_misc_gp_ctlr *)NV_PA_APB_MISC_GP_BASE;
	u32 reg = 0;

	/* Get the number of architectural cache levels */
	asm volatile(
		"mrc p15, 1, r0, c0, c0, 0\n"	/* get CLIDR */
		"lsr r0, r0, #24\n"		/* levels = CLIDR bits 26:24 */
		"and r1, r0, #0x7\n"		/* save to r1 */

		/* enable SMP mode in Aux Ctl reg for all chips */
		"mrc p15, 0, r0, c1, c0, 1\n"
		"orr r0, r0, #0x40\n"		/* ACTLR.SMP, bit 6 */
		"cmp r1, #0x1\n"		/* check number of levels */

		/*
		 * if levels == 1, enable maintenance operation
		 * broadcast (FW) on systems with only a single level
		 * of architectural cache.
		 */
		"orreq r0, r0, #1\n"		/* ACTLR.FW, bit 0 */
		"mcr p15, 0, r0, c1, c0, 1\n");

	/* Currently, only T114 needs this L2 cache change to boot Linux */
	reg = (readl(&gp->hidrev) & HIDREV_CHIPID_MASK);
	if (reg != (CHIPID_TEGRA114 << HIDREV_CHIPID_SHIFT))
		return;
	/*
	 * Systems with an architectural L2 cache must not use the PL310.
	 * Config L2CTLR here for a data RAM latency of 3 cycles.
	 */
	asm volatile(
		"mrc p15, 1, r1, c9, c0, 2\n"
		"and r1, r1, #0xFFFFFFF8\n"
		"orr r1, r1, #2\n"
		"mcr p15, 1, r1, c9, c0, 2\n");
}
