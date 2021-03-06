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
#include <asm/gpio.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dmc.h>
#include <asm/arch/mct.h>
#include <asm/arch/periph.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/power.h>
#include <asm/arch/setup.h>
#include <asm/arch/spl.h>
#include <asm/arch/system.h>
#include <asm/arch/tzpc.h>

/* These are the things we can do during low-level init */
enum {
	DO_WAKEUP	= 1 << 0,
	DO_CLOCKS	= 1 << 1,
	DO_MEM_RESET	= 1 << 2,
	DO_UART		= 1 << 3,
	DO_POWER	= 1 << 4,
	DO_TIMER	= 1 << 5,
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
 * Enable ECC by setting L2CTLR[21].
 * Set L2CTLR[7] to make tag ram latency 3 cycles and
 * set L2CTLR[1] to make data ram latency 3 cycles.
 * We need to make RAM latency of 3 cycles here because cores
 * power ON and OFF while switching. And everytime a core powers
 * ON, iROM provides it a default L2CTLR value 0x400 which stands
 * for TAG RAM setup of 1 cycle. Hence, we face a need of
 * restoring data and tag latency values.
 */
static void configure_l2_ctlr(void)
{
	uint32_t val;

	mrc_l2_ctlr(val);
	val |= (1 << 21);
	val |= (1 << 7);
	val |= (1 << 1);
	mcr_l2_ctlr(val);
}

/*
 * Set L2ACTLR[7] to reissue any memory transaction in the L2 that has been
 * stalled for 1024 cycles to verify that its hazard condition still exists.
 * Disable clean/evict push to external by setting L2ACTLR[3].
 */
static void configure_l2_actlr(void)
{
	uint32_t val;

	mrc_l2_aux_ctlr(val);
	val |= (1 << 27);
	val |= (1 << 7);
	val |= (1 << 3);
	mcr_l2_aux_ctlr(val);
}

/*
 * Power up secondary CPUs.
 */
static void secondary_cpu_start(void)
{
	enable_smp();
	svc32_mode_en();
	branch_bx(CONFIG_EXYNOS_RELOCATE_CODE_BASE);
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
		branch_bx(0x0);
	}

	reg_val = readl(CONFIG_PHY_IRAM_BASE + 0x4);
	if (reg_val != (uint32_t)&low_power_start) {
		/* Store jump address as low_power_start if not present */
		writel((uint32_t)&low_power_start, CONFIG_PHY_IRAM_BASE + 0x4);
		dsb();
		sev();
	}

	/* Set the CPU to SVC32 mode */
	svc32_mode_en();

	/* Read MIDR for Primary Part Number*/
	mrc_midr(val);
	val = (val >> 4);
	val &= 0xf;

	if (val == 0xf) {
		configure_l2_ctlr();
		configure_l2_actlr();
	}

	/* Invalidate L1 & TLB */
	val = 0x0;
	mcr_tlb(val);
	mcr_icache(val);

	/* Disable MMU stuff and caches */
	mrc_sctlr(val);

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
	configure_l2_ctlr();

	/* Clear secondary boot iRAM base */
	writel(0x0, (CONFIG_EXYNOS_RELOCATE_CODE_BASE + 0x1C));

	/* set lowpower flag and address */
	writel(RST_FLAG_VAL, CONFIG_LOWPOWER_FLAG);
	writel((uint32_t)&low_power_start, CONFIG_LOWPOWER_ADDR);
	writel(RST_FLAG_VAL, RST_FLAG_REG);
	/* Store jump address for power down */
	writel((uint32_t)&power_down_core, CONFIG_PHY_IRAM_BASE + 0x4);

	/* Need all core power down check */
	dsb();
	sev();
}
#endif

static void mct_init(void)
{
	struct exynos5_mct *mct = (struct exynos5_mct *)samsung_get_base_mct();

	writel(0, &mct->mct_cfg);
	writel(0, &mct->g_cnt_l);
	writel(0, &mct->g_cnt_u);
	writel(MCT_G_TCON_TIMER_ENABLE, &mct->g_tcon);
}

/**
 * Reset the CPU if the wakeup was not permitted.
 *
 * On some boards we need to look at a special GPIO to ensure that the wakeup
 * from sleep was valid.  If the wakeup is not valid we need to go through a
 * full reset.
 */
static void reset_if_invalid_wakeup(void)
{
	struct spl_machine_param *param = spl_get_machine_params();
	const u32 gpio = param->bad_wake_gpio;
	int is_bad_wake;

	/* We're a bad wakeup if the gpio was defined and was high */
	is_bad_wake = ((gpio != 0xffffffff) && gpio_get_value(gpio));

	if (is_bad_wake) {
		power_reset();

		/*
		 * We don't expect to get here, but it's better to loop
		 * if some bug in U-Boot makes the reset not happen.
		 */
		while (1)
			;
	}
}

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
		actions = DO_CLOCKS | DO_MEM_RESET | DO_POWER |
			DO_TIMER | DO_UART;
	}

	if (actions & DO_POWER)
		power_init();
		/* TODO: Also call board_power_init()? */

	if (actions & DO_CLOCKS) {
		system_clock_init();
		mct_init();
	}
#ifdef CONFIG_SPL_SERIAL_SUPPORT
	if (actions & DO_UART) {
		/* Set up serial UART so we can print. */
		exynos_pinmux_config(PERIPH_ID_UART3, PINMUX_FLAG_NONE);
		serial_init();
	}
#endif
	if (actions & DO_TIMER)
		timer_init();

	if (actions & DO_WAKEUP)
		reset_if_invalid_wakeup();

	if (actions & DO_CLOCKS) {
		/*
		 * Always init the memory controller if we're at reset time.
		 *
		 * If we're not at reset time we'll check for a kernel patch
		 * that might be in iRAM.  The kernel patch can return true
		 * to indicate that it wants to skip the normal memory init.
		 */
		if ((actions & DO_MEM_RESET) || !call_memctrl_patch())
			mem_ctrl_init(actions & DO_MEM_RESET);

		tzpc_init();
	}

	return actions & DO_WAKEUP;
}
