/*
 * Memory setup for SMDK5420 board based on EXYNOS5
 *
 * Copyright (C) 2012 Samsung Electronics
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
#include <asm/arch/clock.h>
#include <asm/arch/dmc.h>
#include <asm/arch/setup.h>

#define NUM_CHIP	1
#define ZQ_MODE_DDS	7
#define BANK_INTERLEAVING 1

void Low_frequency_init_lpddr3(void)
{
	u32 chip;
	struct exynos5_phy_control *phy0_ctrl, *phy1_ctrl;
	struct exynos5_dmc *drex0, *drex1;

	phy0_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY0_BASE;
	phy1_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY1_BASE;
	drex0 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_0;
	drex1 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_1;

	writel(PHY_CON0_VAL, &phy0_ctrl->phy_con0);
	writel(PHY_CON0_VAL, &phy1_ctrl->phy_con0);

	writel(PHY_CON12_VAL, &phy0_ctrl->phy_con12);
	writel(PHY_CON12_VAL, &phy1_ctrl->phy_con12);

	writel(CTRL_OFFSETD_VAL, &phy0_ctrl->phy_con10);
	writel(CTRL_OFFSETD_VAL, &phy1_ctrl->phy_con10);

	writel(PHY_CON4_VAL, &phy0_ctrl->phy_con4);
	writel(PHY_CON4_VAL, &phy1_ctrl->phy_con4);

	writel(PHY_CON6_VAL, &phy0_ctrl->phy_con6);
	writel(PHY_CON6_VAL, &phy1_ctrl->phy_con6);

	/* PHY_CON31 DeSkew Code for CA */
	writel(PHY_CON31_VAL, &phy0_ctrl->phy_con31);
	writel(PHY_CON31_VAL, &phy1_ctrl->phy_con31);

	/* PHY_CON32 DeSkew Code for CA */
	writel(PHY_CON32_VAL, &phy0_ctrl->phy_con32);
	writel(PHY_CON32_VAL, &phy1_ctrl->phy_con32);

	/* PHY_CON33 DeSkew Code for CA */
	writel(PHY_CON33_VAL, &phy0_ctrl->phy_con33);
	writel(PHY_CON33_VAL, &phy1_ctrl->phy_con33);

	setbits_le32(&drex0->phycontrol0, FP_RSYNC);
	clrbits_le32(&drex0->phycontrol0, FP_RSYNC);

	setbits_le32(&drex1->phycontrol0, FP_RSYNC);
	clrbits_le32(&drex1->phycontrol0, FP_RSYNC);

	writel(DMC_MEMCONTROL_VAL, &drex0->memcontrol);
	writel(DMC_MEMCONTROL_VAL, &drex1->memcontrol);

	writel(DMC_CONCONTROL_VAL, &drex0->concontrol);
	writel(DMC_CONCONTROL_VAL, &drex1->concontrol);

	clrbits_le32(&phy0_ctrl->phy_con12, CTRL_START);
	clrbits_le32(&phy1_ctrl->phy_con12, CTRL_START);

	/* Direct Command P0 CH0 */
	writel(DIRECT_CMD_NOP, &drex0->directcmd);
	writel(DIRECT_CMD_MRS1, &drex0->directcmd);
	writel(DIRECT_CMD_MRS2, &drex0->directcmd);
	writel(DIRECT_CMD_MRS3, &drex0->directcmd);
	writel(DIRECT_CMD_MRS4, &drex0->directcmd);
	writel(DIRECT_CMD_MRS5, &drex0->directcmd);

	writel(DIRECT_CMD_NOP, &drex1->directcmd);
	writel(DIRECT_CMD_MRS1, &drex1->directcmd);
	writel(DIRECT_CMD_MRS2, &drex1->directcmd);
	writel(DIRECT_CMD_MRS3, &drex1->directcmd);
	writel(DIRECT_CMD_MRS4, &drex1->directcmd);
	writel(DIRECT_CMD_MRS5, &drex1->directcmd);

	/* Initialization of second DRAM */
	if (NUM_CHIP == 1) {
		chip = (NUM_CHIP << 20);

		writel((DIRECT_CMD_NOP | chip), &drex0->directcmd);
		writel((DIRECT_CMD_MRS1 | chip), &drex0->directcmd);
		writel((DIRECT_CMD_MRS2 | chip), &drex0->directcmd);
		writel((DIRECT_CMD_MRS3 | chip), &drex0->directcmd);
		writel((DIRECT_CMD_MRS4 | chip), &drex0->directcmd);
		writel((DIRECT_CMD_MRS5 | chip), &drex0->directcmd);

		writel((DIRECT_CMD_NOP | chip), &drex1->directcmd);
		writel((DIRECT_CMD_MRS1 | chip), &drex1->directcmd);
		writel((DIRECT_CMD_MRS2 | chip), &drex1->directcmd);
		writel((DIRECT_CMD_MRS3 | chip), &drex1->directcmd);
		writel((DIRECT_CMD_MRS4 | chip), &drex1->directcmd);
		writel((DIRECT_CMD_MRS5 | chip), &drex1->directcmd);
	}

	writel(CTRL_OFFSETD_RESET_VAL, &phy0_ctrl->phy_con10);
	writel(CTRL_OFFSETD_RESET_VAL, &phy1_ctrl->phy_con10);

	writel(PHY_CON4_RESET_VAL, &phy0_ctrl->phy_con4);
	writel(PHY_CON4_RESET_VAL, &phy1_ctrl->phy_con4);

	writel(PHY_CON6_RESET_VAL, &phy0_ctrl->phy_con6);
	writel(PHY_CON6_RESET_VAL, &phy1_ctrl->phy_con6);

	writel(PHY_CON31_RESET_VAL, &phy0_ctrl->phy_con31);
	writel(PHY_CON31_RESET_VAL, &phy1_ctrl->phy_con31);

	writel(PHY_CON32_RESET_VAL, &phy0_ctrl->phy_con32);
	writel(PHY_CON32_RESET_VAL, &phy1_ctrl->phy_con32);

	writel(PHY_CON33_RESET_VAL, &phy0_ctrl->phy_con33);
	writel(PHY_CON33_RESET_VAL, &phy1_ctrl->phy_con33);

	return;
}

void High_frequency_init_lpddr3(void)
{
	u32 val;
	struct exynos5_phy_control *phy0_ctrl, *phy1_ctrl;
	struct exynos5_dmc *drex0, *drex1;

	phy0_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY0_BASE;
	phy1_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY1_BASE;
	drex0 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_0;
	drex1 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_1;

	val = PHY_CON14_RESET_VAL | (CTRL_PULLD_DQS << CTRL_PULLD_DQS_OFFSET);
	writel(val, &phy0_ctrl->phy_con14);
	writel(val, &phy1_ctrl->phy_con14);

	/* ZQ calibration */
	val =  PHY_CON16_RESET_VAL | ZQ_CLK_DIV_EN | ZQ_MANUAL_STR;
	if (ZQ_MODE_DDS == 7) {
		writel(val | (IMPEDANCE_30_OHM << ZQ_MODE_DDS_OFFSET),
			&phy0_ctrl->phy_con16);
		writel(val | (IMPEDANCE_30_OHM << ZQ_MODE_DDS_OFFSET),
			&phy1_ctrl->phy_con16);
		writel(PHY_CON39_VAL_30_OHM,  &phy0_ctrl->phy_con39);
		writel(PHY_CON39_VAL_30_OHM,  &phy1_ctrl->phy_con39);
	} else if (ZQ_MODE_DDS == 6) {
		writel(val | (IMPEDANCE_34_OHM << ZQ_MODE_DDS_OFFSET),
			&phy0_ctrl->phy_con16);
		writel(val | (IMPEDANCE_34_OHM << ZQ_MODE_DDS_OFFSET),
			&phy1_ctrl->phy_con16);
		writel(PHY_CON39_VAL_34_OHM,  &phy0_ctrl->phy_con39);
		writel(PHY_CON39_VAL_34_OHM,  &phy1_ctrl->phy_con39);
	} else if (ZQ_MODE_DDS == 5) {
		writel(val | (IMPEDANCE_40_OHM << ZQ_MODE_DDS_OFFSET),
			&phy0_ctrl->phy_con16);
		writel(val | (IMPEDANCE_40_OHM << ZQ_MODE_DDS_OFFSET),
			&phy1_ctrl->phy_con16);
		writel(PHY_CON39_VAL_40_OHM,  &phy0_ctrl->phy_con39);
		writel(PHY_CON39_VAL_40_OHM,  &phy1_ctrl->phy_con39);
	} else if  (ZQ_MODE_DDS == 4) {
		writel(val | (IMPEDANCE_48_OHM << ZQ_MODE_DDS_OFFSET),
			&phy0_ctrl->phy_con16);
		writel(val | (IMPEDANCE_48_OHM << ZQ_MODE_DDS_OFFSET),
			&phy1_ctrl->phy_con16);
		writel(PHY_CON39_VAL_48_OHM,  &phy0_ctrl->phy_con39);
		writel(PHY_CON39_VAL_48_OHM,  &phy1_ctrl->phy_con39);
	} else {
		writel(val | (IMPEDANCE_30_OHM << ZQ_MODE_DDS_OFFSET),
			&phy0_ctrl->phy_con16);
		writel(val | (IMPEDANCE_30_OHM << ZQ_MODE_DDS_OFFSET),
			&phy1_ctrl->phy_con16);
		writel(PHY_CON39_VAL_30_OHM,  &phy0_ctrl->phy_con39);
		writel(PHY_CON39_VAL_30_OHM,  &phy1_ctrl->phy_con39);
	}

	/* Checking the completion of ZQ calibration */
	do {
		val = readl(&phy0_ctrl->phy_con17);
	} while ((val & ZQ_DONE) != ZQ_DONE);

	do {
		val = readl(&phy1_ctrl->phy_con17);
	} while ((val & ZQ_DONE) != ZQ_DONE);

	/* ZQ calibration exit */
	/* hatim: to beautify the below magic numbers */
	val =  readl(&phy0_ctrl->phy_con16);
	val &= ~(DATA_MASK);
	val |= 0x40304;
	writel(val, &phy0_ctrl->phy_con16);
	val =  readl(&phy1_ctrl->phy_con16);
	val &= ~(DATA_MASK);
	val |= 0x40304;
	writel(val, &phy1_ctrl->phy_con16);

	/* Set DRAM burst length to 8 and READ latency to 12 */
	val = (8 << CTRL_BSTLEN_OFFSET) | (0xC << CTRL_RDLAT_OFFSET);
	writel(val, &phy0_ctrl->phy_con42);
	writel(val, &phy1_ctrl->phy_con42);

	/* Set DRAM write latency */
	val = (CMD_DEFAULT_LPDDR3 << CMD_DEFUALT_OFFSET) |
		(T_WRDATA_EN << T_WRDATA_EN_OFFSET);
	writel(val, &phy0_ctrl->phy_con26);
	writel(val, &phy1_ctrl->phy_con26);

	val = PHY_CON12_RESET_VAL & ~CTRL_START;
	writel(val, &phy0_ctrl->phy_con12);
	writel(val, &phy1_ctrl->phy_con12);
	val = PHY_CON12_RESET_VAL | CTRL_START;
	writel(val, &phy0_ctrl->phy_con12);
	writel(val, &phy1_ctrl->phy_con12);

	clrbits_le32(&drex0->concontrol, DMC_CONCONTROL_EMPTY);
	clrbits_le32(&drex1->concontrol, DMC_CONCONTROL_EMPTY);

	/*
	 * TODO (Hatim.rv@samsung.com):To get more info on the following
	 * registers
	 */
	writel(0x001007E0, 0x10d40f00);
	writel(0x003007E0, 0x10d40f04);
	writel(0x001007E0, 0x10d50f00);
	writel(0x003007E0, 0x10d50f04);

	if (BANK_INTERLEAVING == 0) {
		writel(0x00000323, 0x10d40f10);
		writel(0x00000323, 0x10d40f14);
		writel(0x00000323, 0x10d50f10);
		writel(0x00000323, 0x10d50f14);
	} else {
		writel(0x00002323, 0x10d40f10);
		writel(0x00002323, 0x10d40f14);
		writel(0x00002323, 0x10d50f10);
		writel(0x00002323, 0x10d50f14);
	}

	writel(PRECHCONFIG_DEFAULT_VAL, &drex0->prechconfig0);
	writel(PRECHCONFIG_DEFAULT_VAL, &drex1->prechconfig0);

	writel(PWRDNCONFIG_DEFAULT_VAL, &drex0->pwrdnconfig);
	writel(PWRDNCONFIG_DEFAULT_VAL, &drex1->pwrdnconfig);

	/* Set the TimingAref, TimingRow, TimingData and TimingPower regs */
	writel(TIMINGAREF_VAL, &drex0->timingref);
	writel(TIMINGAREF_VAL, &drex1->timingref);

	writel(TIMINGROW_VAL, &drex0->timingrow);
	writel(TIMINGROW_VAL, &drex1->timingrow);

	writel(TIMINGDATA_VAL, &drex0->timingdata);
	writel(TIMINGDATA_VAL, &drex1->timingdata);

	writel(TIMINGPOWER_VAL, &drex0->timingpower);
	writel(TIMINGPOWER_VAL, &drex1->timingpower);

	do {
		val = readl(&drex0->phystatus);
	} while ((val & DFI_INIT_COMPLETE) != DFI_INIT_COMPLETE);

	val = DMC_CONCONTROL_VAL & ~DFI_INIT_START;
	val &= ~DMC_CONCONTROL_EMPTY;
	writel(val, &drex0->concontrol);
	writel(val, &drex1->concontrol);

	setbits_le32(&drex1->phycontrol0, FP_RSYNC);
	clrbits_le32(&drex1->phycontrol0, FP_RSYNC);

	writel(PHY_CON0_VAL, &phy0_ctrl->phy_con0);
	writel(PHY_CON0_VAL, &phy1_ctrl->phy_con0);

	val = DMC_MEMCONTROL_VAL | CLK_STOP_EN | DPWRDN_EN | DSREF_EN;
	writel(val, &drex0->memcontrol);
	writel(val, &drex1->memcontrol);

	return;
}

int lpddr3_mem_ctrl_init(int reset)
{
	struct exynos5420_clock *clk =
		(struct exynos5420_clock *)EXYNOS5_CLOCK_BASE;
	struct exynos5_phy_control *phy0_ctrl, *phy1_ctrl;
	struct exynos5_dmc *drex0, *drex1;
	u32 val, nLockR, nLockW;

	phy0_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY0_BASE;
	phy1_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY1_BASE;
	drex0 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_0;
	drex1 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_1;

	writel(ENABLE_BIT, &clk->lpddr3phy_ctrl);	/* LPDDR3PHY_CTRL */
	writel(DISABLE_BIT, &clk->lpddr3phy_ctrl);	/* LPDDR3PHY_CTRL */

	setbits_le32(&drex0->concontrol, CA_SWAP_EN);
	setbits_le32(&drex1->concontrol, CA_SWAP_EN);

	/* Disable PAUSE for DREX */
	val = readl(&clk->pause);
	val &= ~ENABLE_BIT;
	writel(val, &clk->pause);

	setbits_le32(&clk->bpll_con1, BYPASS_EN);

	writel(MUX_BPLL_SEL_FOUTBPLL, &clk->clk_src_cdrex);
	do {
		val = readl(&clk->clk_mux_stat_cdrex);
		val &= BPLL_SEL_MASK;
	} while (val != FOUTBPLL);

	clrbits_le32(&clk->bpll_con1, BYPASS_EN);

	writel(CLK_DIV_CDREX_VAL, &clk->clk_div_cdrex0); /* CLK_DIV_CDREX0 */

	Low_frequency_init_lpddr3();

	writel(CLK_DIV_CDREX0_VAL, &clk->clk_div_cdrex0); /* CLK_DIV_CDREX0 */


	High_frequency_init_lpddr3();

	writel(DREX_CONCONTROL_VAL, &drex0->concontrol);
	writel(DREX_CONCONTROL_VAL, &drex1->concontrol);

	writel(BRBRSVCONTROL_VAL, &drex0->brbrsvcontrol);
	writel(BRBRSVCONTROL_VAL, &drex1->brbrsvcontrol);

	writel(BRBRSVCONFIG_VAL, &drex0->brbrsvconfig);
	writel(BRBRSVCONFIG_VAL, &drex1->brbrsvconfig);

	nLockR = readl(&phy0_ctrl->phy_con13);
	nLockW = (nLockR & CTRL_LOCK_COARSE_MASK) >> 2;
	nLockR = readl(&phy0_ctrl->phy_con12);
	nLockR &= ~CTRL_FORCE_MASK;
	nLockW |= nLockR;
	writel(nLockW, &phy0_ctrl->phy_con12);

	nLockR = readl(&phy1_ctrl->phy_con13);
	nLockW = (nLockR & CTRL_LOCK_COARSE_MASK) >> 2;
	nLockR = readl(&phy1_ctrl->phy_con12);
	nLockR &= ~CTRL_FORCE_MASK;
	nLockW |= nLockR;
	writel(nLockW, &phy1_ctrl->phy_con12);

	setbits_le32(&drex0->phycontrol0, SL_DLL_DYN_CON_EN);
	setbits_le32(&drex0->phycontrol0, SL_DLL_DYN_CON_EN);

	return 0;
}
