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

#include <common.h>
#include <asm/io.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/clock.h>
#include <asm/arch/pmc.h>
#include <asm/arch/tegra.h>
#include <asm/arch/flow.h>
#include <asm/arch/ahb.h>
#include <asm/arch/warmboot.h>
#include <asm/arch/sysctr.h>
#include "warmboot_avp.h"

void wb_start(void)
{
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;
	struct flow_ctlr *flow = (struct flow_ctlr *)NV_PA_FLOW_BASE;
	struct clk_rst_ctlr *clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;
	struct ahb_ctlr *ahb = (struct ahb_ctlr *)NV_PA_AHB_BASE;
	struct sysctr_ctlr *sysctr = (struct sysctr_ctlr *)NV_PA_TSC_BASE;
	u32 reg;
	u32 reg_1;
	u32 reg_saved;
	u32 divm;
	u32 divn;
	u32 cpcon, lfcon;

	/* Are we running where we're supposed to be? */
	asm volatile (
		"adr	%0, wb_start;"	/* reg: wb_start address */
		: "=r"(reg)		/* output */
					/* no input, no clobber list */
	);

	if (reg != NV_WB_RUN_ADDRESS)
		goto do_reset;

	/* Are we running with AVP? */
	if (readl(NV_PA_PG_UP_BASE + PG_UP_TAG_0) != PG_UP_TAG_AVP)
		goto do_reset;

	/*
	 * Save the current setting for CLK_BURST_POLICY and change clock
	 * source to CLKM
	 */

	/* use reg_saved to save the crc_sclk_brst_pol value */
	reg_saved = readl(&clkrst->crc_sclk_brst_pol);

	reg = SCLK_SWAKE_FIQ_SRC_CLKM | SCLK_SWAKE_IRQ_SRC_CLKM |
		SCLK_SWAKE_RUN_SRC_CLKM | SCLK_SWAKE_IDLE_SRC_CLKM |
		SCLK_SYS_STATE_RUN;
	writel(reg, &clkrst->crc_sclk_brst_pol);

	/* Update PLLP output dividers for 408 MHz operation, assert RST */
	/* Set OUT1 to 9.6MHz and OUT2 to 48MHz (based on 408MHz of PLLP) */
	writel(PLLP_OUTA_RST, &clkrst->crc_pll[CLOCK_ID_PERIPH].pll_out[0]);
	/* Set OUT3 to 9.6MHz and OUT4 to 48MHz (based on 408MHz of PLLP) */
	writel(PLLP_OUTB_RST, &clkrst->crc_pll[CLOCK_ID_PERIPH].pll_out[1]);

	/*
	 * Read oscillator drive strength from OSC_EDPD_OVER.XOFS and copy
	 * to OSC_CTRL.XOFS and set XOE
	 */
	reg = readl(&pmc->pmc_osc_edpd_over);
	reg &= PMC_XOFS_MASK;
	reg >>= PMC_XOFS_SHIFT;

	reg_1 = readl(&clkrst->crc_osc_ctrl);
	reg_1 &= ~(OSC_XOE_MASK | OSC_XOFS_MASK);

	reg <<= OSC_XOFS_SHIFT;
	reg |= OSC_XOE_ENABLE;
	reg |= reg_1;
	writel(reg, &clkrst->crc_osc_ctrl);

	/* Set CPCON and enable PLLP lock bit */
	reg = PLLP_MISC_PLLP_CPCON_8 | PLLP_MISC_PLLP_LOCK_ENABLE;
	writel(reg, &clkrst->crc_pll[CLOCK_ID_PERIPH].pll_misc);

	/* Find out the current osc frequency */
	reg = readl(&clkrst->crc_osc_ctrl);

	/*
	 * Note: can not use switch statement: compiler builds tables on the
	 * local stack. We don't want to use anything on the stack.
	 */
	reg >>= OSC_CTRL_OSC_FREQ_SHIFT;
	if ((reg == OSC_FREQ_OSC12) || (reg == OSC_FREQ_OSC48)) {
		divm = 0x0c;
		divn = 0x198;
	} else if (reg == OSC_FREQ_OSC16P8) {
		divm = 0x0e;
		divn = 0x154;
	} else if ((reg == OSC_FREQ_OSC19P2) || (reg == OSC_FREQ_OSC38P4)) {
		divm = 0x10;
		divn = 0x154;
	} else if (reg == OSC_FREQ_OSC26) {
		divm = 0x1a;
		divn = 0x198;
	} else {
		/*
		 * Unused code in OSC_FREQ is mapped to 13MHz - use 13MHz as
		 * default settings.
		 */
		divm = 0x0d;
		divn = 0x198;
	}

	/* Change PLLP to be 408MHz */
	reg = (divm << PLL_DIVM_SHIFT) |
			(divn << PLL_DIVN_SHIFT) |
			PLL_BASE_OVRRIDE_MASK |
			PLL_ENABLE_MASK;
	writel(reg, &clkrst->crc_pll[CLOCK_ID_PERIPH].pll_base);

	/* Wait till PLLP is lock */
	while ((readl(&clkrst->crc_pll[CLOCK_ID_PERIPH].pll_base) &
		PLL_BASE_LOCK_MASK) == 0)
			;

	/* Update PLLP output dividers for 408 MHz operation, deassert RSTN */
	writel(PLLP_OUTA_NRST, &clkrst->crc_pll[CLOCK_ID_PERIPH].pll_out[0]);
	writel(PLLP_OUTB_NRST, &clkrst->crc_pll[CLOCK_ID_PERIPH].pll_out[1]);

	/*
	 * Wait for 250uS after lock bit is set to make sure pll is stable.
	 * The typical wait time is 300uS. Since we already check the lock
	 * bit, reduce the wait time to 250uS.
	 */
	reg = 250 | EVENT_USEC | EVENT_MODE_STOP;
	writel(reg, &flow->halt_cop_events);

	/* Restore setting for SCLK_BURST_POLICY */
	writel(reg_saved, &clkrst->crc_sclk_brst_pol);

	/*
	 * Enable the PPSB_STOPCLK feature to allow SCLK to be run at
	 * higher frequencies.
	 */
	reg = readl(&clkrst->crc_misc_clk_enb);
	reg |= EN_PPSB_STOPCLK;
	writel(reg, &clkrst->crc_misc_clk_enb);

	reg = readl(&ahb->arbitration_xbar_ctrl);
	reg |= ARBITRATION_XBAR_CTRL_PPSB_ENABLE;
	writel(reg, &ahb->arbitration_xbar_ctrl);

	/*
	 * Lock down the memory aperture configuration since the sticky
	 * secure lock bit was reset because of LP0.
	 */
	reg = readl(&pmc->pmc_sec_disable);
	reg |= SEC_DISABLE_AMAP_WRITE_ON;
	writel(reg, &pmc->pmc_sec_disable);

	/*
	 * Find out which CPU (slow or fast) to wake up. The default setting
	 * in flow controller is to wake up GCPU
	 */
	reg = readl(&pmc->pmc_scratch4);
	if (reg & CPU_WAKEUP_CLUSTER) {
		reg = readl(&flow->cluster_control);
		reg |= ACTIVE_LP;
		writel(reg, &flow->cluster_control);
	}

	/* Program SUPER_CCLK_DIVIDER */
	reg = SUPER_CDIV_ENB_ENABLE;
	writel(reg, &clkrst->crc_super_cclk_div);

	/* Enable the CoreSight clock */
	reg = CLK_ENB_CSITE;
	writel(reg, &clkrst->crc_clk_enb_ex[TEGRA_DEV_U].set);

	/*
	 * De-assert CoreSight reset.
	 * NOTE: We're leaving the CoreSight clock on the oscillator for
	 *       now. It will be restored to its original clock source
	 *       when the CPU-side restoration code runs.
	 */
	reg = SWR_CSITE_RST;
	writel(reg, &clkrst->crc_rst_dev_ex[TEGRA_DEV_U].clr);

	/* Find out the current osc frequency */
	reg = readl(&clkrst->crc_osc_ctrl);
	reg >>= OSC_CTRL_OSC_FREQ_SHIFT;

	/* Find out the PLL-U value to use */
	if ((reg == OSC_FREQ_OSC12) || (reg == OSC_FREQ_OSC48)) {
		divm = 0x0c;
		divn = 0x3c0;
		cpcon = 0x0c;
		lfcon = 0x02;
	} else if (reg == OSC_FREQ_OSC16P8) {
		divm = 0x07;
		divn = 0x190;
		cpcon = 0x05;
		lfcon = 0x02;
	} else if ((reg == OSC_FREQ_OSC19P2) || (reg == OSC_FREQ_OSC38P4)) {
		divm = 0x04;
		divn = 0xc8;
		cpcon = 0x03;
		lfcon = 0x02;
	} else if (reg == OSC_FREQ_OSC26) {
		divm = 0x1a;
		divn = 0x3c0;
		cpcon = 0x0c;
		lfcon = 0x02;
	} else {
		/*
		 * Unused code in OSC_FREQ is mapped to 13MHz - use 13MHz as
		 * default settings.
		 */
		divm = 0x0d;
		divn = 0x3c0;
		cpcon = 0x0c;
		lfcon = 0x02;
	}

	/* Program PLL-U */
	reg = PLLU_BYPASS_ENABLE | PLLU_OVERRIDE_ENABLE | (divn << 8) |
		(divm << 0);
	writel(reg, &clkrst->crc_pll[CLOCK_ID_USB].pll_base);
	reg_1 = (cpcon << 8) | (lfcon << 4);
	writel(reg_1, &clkrst->crc_pll[CLOCK_ID_USB].pll_misc);

	/* Enable PLL-U */
	reg &= ~PLLU_BYPASS_ENABLE;
	reg |= PLLU_ENABLE_ENABLE;
	writel(reg, &clkrst->crc_pll[CLOCK_ID_USB].pll_base);
	reg_1 |= PLLU_LOCK_ENABLE_ENABLE;
	writel(reg_1, &clkrst->crc_pll[CLOCK_ID_USB].pll_misc);

	/*
	 * Set the CPU reset vector. pmc_scratch41 contains the physical
	 * address of the CPU-side restoration code.
	 */
	reg = readl(&pmc->pmc_scratch41);
	writel(reg, EXCEP_VECTOR_CPU_RESET_VECTOR);

	/*
	 * Following builds up instructions for the CPU startup
	 *	MOV	R0, #LSB16(ResetVector);
	 *	MOVT	R0, #MSB16(ResetVector);
	 *	BX	R0;
	 *	B	-12;
	 */
	reg_1 = ((reg & 0xF000) << 4) | (reg & 0x0FFF);
	reg_1 |= 0xE3000000;	/* MOV LSB(Rn) */
	writel(reg_1, 0x4003FFF0);

	reg >>= 16;
	reg_1 = ((reg & 0xF000) << 4) | (reg & 0x0FFF);
	reg_1 |= 0xE3400000;	/* MOV MSB(Rn) */
	writel(reg_1, 0x4003FFF4);
	/* BX R0 */
	writel(0xE12FFF10, 0x4003FFF8);
	/* B -12 */
	writel(0xEAFFFFFB, 0x4003FFFC);

	/* Select CPU complex clock source. */
	writel(CCLK_PLLP_BURST_POLICY, &clkrst->crc_cclk_brst_pol);

	/* Set MSELECT clock source to PLL_P with 1:4 divider */
	reg = MSELECT_CLK_SRC_PLLP_OUT0 | MSELECT_CLK_DIVISOR_6;
	writel(reg,
		&clkrst->crc_clk_src_vw[PERIPHC_MSELECT - PERIPHC_VW_FIRST]);

	/* Enable clock to MSELECT */
	reg = SET_CLK_ENB_MSELECT_ENABLE;
	writel(reg, &clkrst->crc_clk_enb_ex_vw[TEGRA_DEV_V].set);

	/* Bring MSELECT out of reset, after 2 microsecond wait */
	reg = readl(NV_PA_TMRUS_BASE);
	while (1) {
		if (readl(NV_PA_TMRUS_BASE) > (reg + 2))
			break;
	}

	reg = SET_CLK_ENB_MSELECT_ENABLE;
	writel(reg, &clkrst->crc_rst_dev_ex_vw[TEGRA_DEV_V].clr);

	/* Disable PLLX, since it is not used as CPU clock source */
	reg = readl(&clkrst->crc_pll_simple[SIMPLE_PLLX].pll_base);
	reg &= ~(PLLX_BASE_PLLX_ENABLE);
	writel(reg, &clkrst->crc_pll_simple[SIMPLE_PLLX].pll_base);

	/* Set CAR2PMC_CPU_ACK_WIDTH to 408 */
	reg = readl(&clkrst->crc_cpu_softrst_ctrl2);
	reg |= CAR2PMC_CPU_ACK_WIDTH_408;
	writel(reg, &clkrst->crc_cpu_softrst_ctrl2);

	/* Enable the CPU complex clock */
	reg = CLK_ENB_CPU;
	writel(reg, &clkrst->crc_clk_enb_ex[TEGRA_DEV_L].set);

	reg = SET_CLK_ENB_CPUG_ENABLE | SET_CLK_ENB_CPULP_ENABLE;
	writel(reg, &clkrst->crc_clk_enb_ex_vw[TEGRA_DEV_V].set);

	/* Take non-cpu of G and LP cluster OUT of reset */
	reg = CLR_NONCPURESET;
	writel(reg, &clkrst->crc_rst_cpulp_cmplx_clr);
	writel(reg, &clkrst->crc_rst_cpug_cmplx_clr);

	/* Clear software controlled reset of slow cluster */
	reg = CLR_CPURESET0 | CLR_DBGRESET0 | CLR_CORERESET0 | CLR_CXRESET0;
	writel(reg, &clkrst->crc_rst_cpulp_cmplx_clr);

	/* Clear software controlled reset of fast cluster */
	reg = CLR_CPURESET0 | CLR_DBGRESET0 | CLR_CORERESET0 | CLR_CXRESET0 |
		CLR_CPURESET1 | CLR_DBGRESET1 | CLR_CORERESET1 | CLR_CXRESET1 |
		CLR_CPURESET2 | CLR_DBGRESET2 | CLR_CORERESET2 | CLR_CXRESET2 |
		CLR_CPURESET3 | CLR_DBGRESET3 | CLR_CORERESET3 | CLR_CXRESET3;
	writel(reg, &clkrst->crc_rst_cpug_cmplx_clr);

	/* Initialize System Counter (TSC) with osc frequency */
	/* Find out the current osc frequency */
	reg = readl(&clkrst->crc_osc_ctrl);
	reg >>= OSC_CTRL_OSC_FREQ_SHIFT;

	if (reg == OSC_FREQ_OSC12)
		reg_1 = 12000000;
	else if (reg == OSC_FREQ_OSC48)
		reg_1 = 48000000;
	else if (reg == OSC_FREQ_OSC16P8)
		reg_1 = 16800000;
	else if (reg == OSC_FREQ_OSC19P2)
		reg_1 = 19200000;
	else if (reg == OSC_FREQ_OSC38P4)
		reg_1 = 38400000;
	else if (reg == OSC_FREQ_OSC26)
		reg_1 = 26000000;
	else
		reg_1 = 13000000;

	/* write frequency (in reg_1) to SYSCTR0_CNTFID0 */
	writel(reg_1, &sysctr->cntfid0);
	/* enable the TSC */
	reg = readl(&sysctr->cntcr);
	reg |= TSC_CNTCR_ENABLE | TSC_CNTCR_HDBG;
	writel(reg, &sysctr->cntcr);

	/*
	 * Reprogram PMC_CPUPWRGOOD_TIMER register:
	 *
	 * Kernel prepares PMC_CPUPWRGOOD_TIMER based on 32768Hz clock.
	 * Note that PMC_CPUPWRGOOD_TIMER is running at pclk.
	 *
	 * Need to reprogram PMC_CPUPWRGOOD_TIMER based on the current pclk,
	 * which is @408Mhz (pclk = sclk = pllp_out0) after reset.
	 *
	 * So, just multiply a factor = (408M/32K) to PMC_CPUPWRGOOD_TIMER.
	 *
	 * Save the original PMC_CPUPWRGOOD_TIMER register, need to restore
	 * it after CPU is powered up.
	 */
	reg = readl(&pmc->pmc_cpupwrgood_timer);
	reg_saved = reg;

	reg *= (408000000 / 32768);
	writel(reg, &pmc->pmc_cpupwrgood_timer);

	/* Find out which cluster (slow or fast) to power on */
	reg = readl(&pmc->pmc_scratch4);
	if (reg & CPU_WAKEUP_CLUSTER) {
		/* Power up the slow cluster non-CPU partition. */
		reg = PWRGATE_STATUS_C1NC_ENABLE;
		reg_1 = PWRGATE_TOGGLE_PARTID_C1NC | PWRGATE_TOGGLE_START;
		if (!(readl(&pmc->pmc_pwrgate_status) & reg)) {
			/* partition is not on, turn it on */
			writel(reg_1, &pmc->pmc_pwrgate_toggle);

			/* wait until the partitions is powered on */
			while (!(readl(&pmc->pmc_pwrgate_status) & reg))
				;

			/* wait until clamp is off */
			while (readl(&pmc->pmc_clamp_status) & reg)
				;
		}

		reg = PWRGATE_STATUS_CELP_ENABLE;
		reg_1 = PWRGATE_TOGGLE_PARTID_CELP | PWRGATE_TOGGLE_START;
		if (!(readl(&pmc->pmc_pwrgate_status) & reg)) {
			/* partition is not on, turn it on */
			writel(reg_1, &pmc->pmc_pwrgate_toggle);

			/* wait until the partitions is powered on */
			while (!(readl(&pmc->pmc_pwrgate_status) & reg))
				;

			/* wait until clamp is off */
			while (readl(&pmc->pmc_clamp_status) & reg)
				;
		}
	} else {
		/* FastCPU */
		reg = PWRGATE_STATUS_CRAIL_ENABLE;
		reg_1 = PWRGATE_TOGGLE_PARTID_CRAIL | PWRGATE_TOGGLE_START;
		if (!(readl(&pmc->pmc_pwrgate_status) & reg)) {
			/* partition is not on, turn it on */
			writel(reg_1, &pmc->pmc_pwrgate_toggle);

			/* wait until the partitions is powered on */
			while (!(readl(&pmc->pmc_pwrgate_status) & reg))
				;

			/* wait until clamp is off */
			while (readl(&pmc->pmc_clamp_status) & reg)
				;
		}

		reg = PWRGATE_STATUS_C0NC_ENABLE;
		reg_1 = PWRGATE_TOGGLE_PARTID_C0NC | PWRGATE_TOGGLE_START;
		if (!(readl(&pmc->pmc_pwrgate_status) & reg)) {
			/* partition is not on, turn it on */
			writel(reg_1, &pmc->pmc_pwrgate_toggle);

			/* wait until the partitions is powered on */
			while (!(readl(&pmc->pmc_pwrgate_status) & reg))
				;

			/* wait until clamp is off */
			while (readl(&pmc->pmc_clamp_status) & reg)
				;
		}

		reg = PWRGATE_STATUS_CE0_ENABLE;
		reg_1 = PWRGATE_TOGGLE_PARTID_CE0 | PWRGATE_TOGGLE_START;
		if (!(readl(&pmc->pmc_pwrgate_status) & reg)) {
			/* partition is not on, turn it on */
			writel(reg_1, &pmc->pmc_pwrgate_toggle);

			/* wait until the partitions is powered on */
			while (!(readl(&pmc->pmc_pwrgate_status) & reg))
				;

			/* wait until clamp is off */
			while (readl(&pmc->pmc_clamp_status) & reg)
				;
		}
	}

	/* restore the original PMC_CPUPWRGOOD_TIMER register */
	writel(reg_saved, &pmc->pmc_cpupwrgood_timer);

avp_halt:
	reg = EVENT_MODE_STOP | EVENT_JTAG;
	writel(reg, &flow->halt_cop_events);
	goto avp_halt;

do_reset:
	writel(SWR_TRIG_SYS_RST, &clkrst->crc_rst_dev[TEGRA_DEV_L]);
	while (1)
		;
}

/*
 * wb_end() is a dummy function, and must be directly following wb_start(),
 * and is used to calculate the size of wb_start().
 */
void wb_end(void)
{
}
