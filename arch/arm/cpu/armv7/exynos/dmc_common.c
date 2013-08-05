/*
 * Mem setup common file for different types of DDR present on EXYNOS5 boards.
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
#include <asm/arch/spl.h>
#include <asm/arch/cpu.h>
#include <asm/arch/setup.h>

#include "clock_init.h"

#define ZQ_INIT_TIMEOUT	10000

int dmc_config_zq(struct mem_timings *mem,
		  struct exynos5_phy_control *phy0_ctrl,
		  struct exynos5_phy_control *phy1_ctrl)
{
	unsigned long val = 0;
	int i;

	/*
	 * ZQ Calibration:
	 * Select Driver Strength,
	 * long calibration for manual calibration
	 */
	val = PHY_CON16_RESET_VAL;
	val |= mem->zq_mode_dds << PHY_CON16_ZQ_MODE_DDS_SHIFT;
	val |= mem->zq_mode_term << PHY_CON16_ZQ_MODE_TERM_SHIFT;
	/* Disable termination */
	if (mem->zq_mode_noterm)
		val |= PHY_CON16_ZQ_MODE_NOTERM_MASK;
	writel(val, &phy0_ctrl->phy_con16);
	writel(val, &phy1_ctrl->phy_con16);

	writel(0x7, &phy0_ctrl->phy_con40);
	writel(0x7, &phy1_ctrl->phy_con40);

	setbits_le32(&phy0_ctrl->phy_con16, ZQ_CLK_DIV_EN);
	setbits_le32(&phy1_ctrl->phy_con16, ZQ_CLK_DIV_EN);

	setbits_le32(&phy0_ctrl->phy_con16, LONG_CALIBRATION);
	setbits_le32(&phy1_ctrl->phy_con16, LONG_CALIBRATION);

	/* ZQ_MANUAL_START: Enable */
	setbits_le32(&phy0_ctrl->phy_con16, ZQ_MANUAL_STR);
	setbits_le32(&phy1_ctrl->phy_con16, ZQ_MANUAL_STR);

	/*
	 * Since we are manaully calibrating the ZQ values,
	 * we are looping for the ZQ_init to complete.
	 */
	i = ZQ_INIT_TIMEOUT;
	while ((readl(&phy0_ctrl->phy_con17) & ZQ_DONE) != ZQ_DONE && i > 0) {
		sdelay(100);
		i--;
	}
	if (!i)
		return -1;
	clrbits_le32(&phy0_ctrl->phy_con16, ZQ_MANUAL_STR);

	i = ZQ_INIT_TIMEOUT;
	while ((readl(&phy1_ctrl->phy_con17) & ZQ_DONE) != ZQ_DONE && i > 0) {
		sdelay(100);
		i--;
	}
	if (!i)
		return -1;
	clrbits_le32(&phy1_ctrl->phy_con16, ZQ_MANUAL_STR);

	return 0;
}

void update_reset_dll(struct exynos5_dmc *dmc, enum ddr_mode mode)
{
	unsigned long val;

	if (mode == DDR_MODE_DDR3) {
		val = MEM_TERM_EN | PHY_TERM_EN | DMC_CTRL_SHGATE;
		writel(val, &dmc->phycontrol0);
	}

	/* Update DLL Information: Force DLL Resyncronization */
	val = readl(&dmc->phycontrol0);
	val |= FP_RSYNC;
	writel(val, &dmc->phycontrol0);

	/* Reset Force DLL Resyncronization */
	val = readl(&dmc->phycontrol0);
	val &= ~FP_RSYNC;
	writel(val, &dmc->phycontrol0);
}

void dmc_config_mrs(struct mem_timings *mem, struct exynos5_dmc *dmc)
{
	int channel, chip;

	for (channel = 0; channel < mem->dmc_channels; channel++) {
		unsigned long mask;

		mask = channel << DIRECT_CMD_CHANNEL_SHIFT;
		for (chip = 0; chip < mem->chips_to_configure; chip++) {
			int i;

			mask |= chip << DIRECT_CMD_CHIP_SHIFT;

			/* Sending NOP command */
			writel(DIRECT_CMD_NOP | mask, &dmc->directcmd);

			/*
			 * TODO(alim.akhtar@samsung.com): Do we need these
			 * delays? This one and the next were not there for
			 * DDR3.
			 */
			sdelay(0x10000);

			/* Sending EMRS/MRS commands */
			for (i = 0; i < MEM_TIMINGS_MSR_COUNT; i++) {
				writel(mem->direct_cmd_msr[i] | mask,
				       &dmc->directcmd);
				sdelay(0x10000);
			}

			if (mem->send_zq_init) {
				/* Sending ZQINIT command */
				writel(DIRECT_CMD_ZQINIT | mask,
				       &dmc->directcmd);

				sdelay(10000);
			}
		}
	}
}

void dmc_config_prech(struct mem_timings *mem, struct exynos5_dmc *dmc)
{
	int channel, chip;

	for (channel = 0; channel < mem->dmc_channels; channel++) {
		unsigned long mask;

		mask = channel << DIRECT_CMD_CHANNEL_SHIFT;
		for (chip = 0; chip < mem->chips_per_channel; chip++) {
			mask |= chip << DIRECT_CMD_CHIP_SHIFT;

			/* PALL (all banks precharge) CMD */
			writel(DIRECT_CMD_PALL | mask, &dmc->directcmd);
		}
	}
}

void mem_ctrl_init(int reset)
{
	struct spl_machine_param *param = spl_get_machine_params();
	int ret;

#ifdef CONFIG_EXYNOS5420
	/*
	 * During Suspend-Resume & S/W-Reset, as soon as PMU releases
	 * pad retention, CKE goes high. This causes memory contents
	 * not to be retained during DRAM initialization. Therfore,
	 * there is a new control register(0x100431e8[28]) which lets us
	 * release pad retention and retain the memory content until the
	 * initialization is complete.
	 */
	if (readl(INF_REG_BASE + INF_REG1_OFFSET) == S5P_CHECK_SLEEP) {
		writel(PAD_RETENTION_DRAM_COREBLK_VAL,
			PAD_RETENTION_DRAM_COREBLK_OPTION);
		do {
			ret = readl(PAD_RETENTION_DRAM_STATUS);
		} while (ret != 0x1);
	}
#endif

	/* If there are any other memory variant, add their init call below */
	if (param->mem_type == DDR_MODE_DDR3) {
		ret = ddr3_mem_ctrl_init(reset);
		if (ret) {
			/* will hang if failed to init memory control */
			while (1)
				;
		}
#ifdef CONFIG_EXYNOS5420
	} else if (param->mem_type == DDR_MODE_LPDDR3) {
		ret = lpddr3_mem_ctrl_init(reset);
		if (ret) {
			/* will hang if failed to init memory control */
			while (1)
				;
		}
#endif
	} else {
		/* will hang if unknow memory type  */
		while (1)
			;
	}
}
