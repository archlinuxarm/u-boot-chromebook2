/*
 * Copyright (c) 2010 Samsung Electronics.
 * Minkyu Kang <mk7.kang@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/arch/system.h>

enum l2_cache_params {
	CACHE_TAG_RAM_SETUP = (1 << 9),
	CACHE_DATA_RAM_SETUP = (1 << 5),
	CACHE_TAG_RAM_LATENCY = (2 << 6),
	CACHE_DATA_RAM_LATENCY = (2 << 0)
};

void reset_cpu(ulong addr)
{
	writel(0x1, samsung_get_base_swreset());
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#ifndef CONFIG_SYS_L2CACHE_OFF
/*
 * Set L2 cache parameters
 */
static void exynos5_set_l2cache_params(void)
{
	unsigned int val = 0;

	asm volatile("mrc p15, 1, %0, c9, c0, 2\n" : "=r"(val));

	val |= CACHE_TAG_RAM_SETUP |
		CACHE_DATA_RAM_SETUP |
		CACHE_TAG_RAM_LATENCY |
		CACHE_DATA_RAM_LATENCY;

	asm volatile("mcr p15, 1, %0, c9, c0, 2\n" : : "r"(val));

#ifdef CONFIG_EXYNOS5420
	mrc_l2_aux_ctlr(val);

	/* L2ACTLR[3]: Disable clean/evict push to external */
	val |= (1 << 3);

	/* L2ACTLR[7]: Enable hazard detect timeout for A15 */
	val |= (1 << 7);

	/* L2ACTLR[27]: Prevents stopping the L2 logic clock */
	val |= (1 << 27);

	mcr_l2_aux_ctlr(val);

	/* Read the l2 control register to force things to take effect? */
	mrc_l2_ctlr(val);
#endif
}

/*
 * Sets L2 cache related parameters before enabling data cache
 */
void v7_outer_cache_enable(void)
{
	if (cpu_is_exynos5())
		exynos5_set_l2cache_params();
}
#endif
