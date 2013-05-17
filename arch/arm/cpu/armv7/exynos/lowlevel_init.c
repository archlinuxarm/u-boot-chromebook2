/*
 * Lowlevel setup for EXYNOS5 based board
 *
 * Copyright (C) 2013 Samsung Electronics
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

#include <common.h>
#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dmc.h>
#include <asm/arch/power.h>
#include <asm/arch/tzpc.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
#include <asm/arch/system.h>
#include "setup.h"

/* These are the things we can do during low-level init */
enum {
	DO_WAKEUP	= 1 << 0,
	DO_CLOCKS	= 1 << 1,
	DO_MEM_RESET	= 1 << 2,
	DO_UART		= 1 << 3,
	DO_POWER	= 1 << 4,
};

#ifdef CONFIG_EXYNOS5420
/*
 * Ensure that the L2 logic has been used within the previous 256 cycles
 * before modifying the ACTLR.SMP bit. This is required during boot before
 * MMU has been enabled, or during a specified reset or power down sequence.
 */
static void enable_smp(void)
{
	uint32_t temp, val;

	/* Enable SMP mode */
	mrc_auxr(temp);
	temp |= (1 << 6);

	/* Dummy read to assure L2 access */
	val = readl(INF_REG_BASE);
	val &= 0;
	temp |= val;
	mcr_auxr(temp);
	dsb();
	isb();
}

/*
 * Set L2ACTLR[7] to reissue any memory transaction in the L2 that has been
 * stalled for 1024 cycles to verify that its hazard condition still exists.
 */
static void set_l2cache(void)
{
	uint32_t val;

	/* Read MIDR for Primary Part Number*/
	mrc_midr(val);
	val = (val >> 4);
	val &= 0xf;

	/* L2ACTLR[7]: Enable hazard detect timeout for A15 */
	if (val == 0xf) {
		mrc_l2_aux_ctlr(val);
		val |= (1 << 7);
		mcr_l2_aux_ctlr(val);
		mrc_l2_ctlr(val);
	}
}

/*
 * Start cluster switching.
 */
static void switch_cluster(void)
{
	uint32_t val, cpu_id, cluster_id;

	enable_smp();
	svc32_mode_en();

	/* Get cluster and CPU id */
	mrc_mpafr(cpu_id);
	cluster_id = (cpu_id >> 8);
	cluster_id &= 0xf;
	cpu_id &= 0xf;

	/*
	 * While carrying out a switch, outbound cluster should disable
	 * interrupts till the time inbound cluster sets the GICD_IGROUPR0
	 * to '1', hence, memory flag for waiting and moving on to the
	 * next step.
	 */
	writel(0x1, CONFIG_GIC_STATE + (cpu_id << 2));

	/* All cores enter WFE and wait for primary core */
	while (readl(CONFIG_CPU_STATE + (cpu_id << 2)) & (1 << 5)) {
		if (cluster_id == 1 && cpu_id != 0) {
			wfe();
		} else {
			val = 0;
			while (val != 100)
				val++;
		}
	}

	/* Primary core send event */
	if  (cluster_id == 1 && cpu_id == 0) {
		sev();
	}

	svc32_mode_en();
	set_pc(CONFIG_IROM_WORKAROUND_BASE);
	while (1);
}

/*
 * Power up secondary CPUs.
 */
static void secondary_cpu_start(void)
{
	uint32_t temp, val;

	svc32_mode_en();
	mrc_mpafr(temp);

	/* No need to reset if current cluster is Eagle */
	if (!(temp & 0xf00))
		goto no_reset;

	/* Check reset status and jump if not reset */
	val = readl(PMU_SPARE_2);
	if (val == 0)
		goto reset;

	/* Clear reset flag */
	writel(0x0, PMU_SPARE_2);
no_reset:
	enable_smp();
	svc32_mode_en();
	set_pc(CONFIG_IROM_WORKAROUND_BASE);
reset:
	/* Set reset flag */
	writel(0x1, PMU_SPARE_2);

	/* Clear secondary boot iRAM base */
	writel(0x0, (CONFIG_IROM_WORKAROUND_BASE + 0x1C));
}

/*
 * This is the entry point of hotplug-in and
 * cluster switching.
 */
static void low_power_start(void)
{
	uint32_t val, reg_val;

	reg_val = readl(RST_FLAG_REG);
	if (reg_val != RST_FLAG_VAL) {
		writel(0x0, CONFIG_LOWPOWER_FLAG);
		set_pc(0x0);
	}

	/* Set the CPU to SVC32 mode */
	svc32_mode_en();
	set_l2cache();

	/* Invalidate L1 & TLB */
	val = 0x0;
	mcr_tlb(val);
	mcr_icache(val);

	/* Disable MMU stuff and caches */
	mrc_sctlr(val);
	mrc_mpafr(reg_val);
	reg_val &= 0xf;

	/* If CPU State if Switch, Start cluster switching */
	if ((readl(CONFIG_CPU_STATE + (reg_val << 2)) & (1 << 4)))
		switch_cluster();

	val &= ~((0x2 << 12) | 0x7);
	val |= ((0x1 << 12) | (0x8 << 8) | 0x2);
	mcr_sctlr(val);

	/* CPU state is hotplug or reset */
	secondary_cpu_start();

	/* Core should not enter into WFI here */
	wfi();

}

/*
 * Pointer to this function is stored in iRam which is used
 * for jump and power down of a specific core.
 */
static void power_down_core(void)
{
	uint32_t tmp, core_id, core_config;

	/* Get the core id */
	mrc_mpafr(core_id);
	tmp = core_id & 0x3;
	core_id = (core_id >> 6) & ~3;
	core_id |= tmp;

	/* Set the status of the core to low */
	core_config = (core_id * CORE_CONFIG_OFFSET);
	core_config += ARM_CORE0_CONFIG;
	writel(0x0, core_config);

	/* Core enter WFI */
	wfi();
}

/*
 * Configurations for secondary cores are inapt at this stage.
 * Reconfigure secondary cores. Shutdown and change the status
 * of all cores except the primary core.
 */
static void secondary_cores_configure(void)
{
	uint32_t i, core_id;

	/* Store jump address for power down of secondary cores */
	writel((uint32_t)&power_down_core, CONFIG_PHY_IRAM_BASE + 0x4);

	/* Need all core power down check */
	dsb();
	sev();

	/*
	 * Power down all cores(secondary) while primary core must
	 * wait for all cores to go down.
	 */
	for (core_id = 1; core_id != CORE_COUNT; core_id++) {
		while ((readl(ARM_CORE0_STATUS
			+ (core_id * CORE_CONFIG_OFFSET))
			& 0xff) != 0x0) {
			isb();
			sev();
		}
		isb();
	}

	/* set lowpower flag and address */
	writel(CONFIG_LOWPOWER_EN, CONFIG_LOWPOWER_FLAG);
	for (i = 0; i < 0x20; i += 0x4)
		writel((uint32_t)&low_power_start, CONFIG_PHY_IRAM_BASE + i);

	/* Setup L2 cache */
	set_l2cache();

	/* Clear secondary boot iRAM base */
	writel(0x0, (CONFIG_IROM_WORKAROUND_BASE + 0x1C));

	/* Set reset flag */
	writel(RST_FLAG_VAL, RST_FLAG_REG);
}
#endif

int do_lowlevel_init(void)
{
	uint32_t reset_status;
	int actions = 0;

	arch_cpu_init();

#ifdef CONFIG_EXYNOS5420
	/* Reconfigure secondary cores */
	secondary_cores_configure();
#endif

	reset_status = get_reset_status();

	switch (reset_status) {
	case S5P_CHECK_SLEEP:
		actions = DO_CLOCKS | DO_WAKEUP;
		break;
	case S5P_CHECK_DIDLE:
	case S5P_CHECK_LPA:
		actions = DO_WAKEUP;
		break;
	default:
		/* This is a normal boot (not a wake from sleep) */
		actions = DO_CLOCKS | DO_MEM_RESET | DO_POWER;
	}

	if (actions & DO_POWER)
		power_init();
		/* TODO: Also call board_power_init()? */
	if (actions & DO_CLOCKS) {
		system_clock_init();
		mem_ctrl_init(actions & DO_MEM_RESET);
		tzpc_init();
	}

#ifdef CONFIG_EXYNOS_SPL_UART
	if (actions & DO_UART) {
		exynos_pinmux_config(PERIPH_ID_UART3, PINMUX_FLAG_NONE);
		serial_init();
		timer_init();
	}
#endif

	return actions & DO_WAKEUP;
}
