/*
 * DDR3 mem setup file for EXYNOS5 based board
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
#include <asm/io.h>
#include <asm/arch/board.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dmc.h>
#include <asm/arch/power.h>
#include <asm/arch/setup.h>
#include "clock_init.h"

#define TIMEOUT			10000
#define NUM_BYTE_LANES		4
#define DEFAULT_DQS		8
#define DEFAULT_DQS_X4		0x08080808
#define FALSE 0
#define TRUE 1

/*
 * RAM address to use in the test.
 *
 * We'll use 4 words at this address and 4 at this address + 0x80 (Ares
 * interleaves channels every 128 bytes).  This will allow us to evaluate all of
 * the chips in a 1 chip per channel (2GB) system and half the chips in a 2
 * chip per channel (4GB) system.  We can't test the 2nd chip since we need to
 * do tests before the 2nd chip is enabled.  Looking at the 2nd chip isn't
 * critical because the 1st and 2nd chip have very similar timings (they'd
 * better have similar timings, since there's only a single adjustment that is
 * shared by both chips).
 */
const unsigned int test_addr = 0x20000000;

/* Test pattern with which RAM will be tested */
static const unsigned int test_pattern[] = {
	0x5A5A5A5A,
	0xA5A5A5A5,
	0xF0F0F0F0,
	0x0F0F0F0F,
};

static struct mem_timings ares_ddr3_timings = {
		.mem_manuf = MEM_MANUF_SAMSUNG,
		.mem_type = DDR_MODE_DDR3,
		.frequency_mhz = 800,
		.direct_cmd_msr = {
			0x00020018, 0x00030000, 0x00010046, 0x00000d70,
			0x00000c70
		},
		.timing_ref = 0x000000bb,
		.timing_row = 0x6836650f,
		.timing_data = 0x3630580b,
		.timing_power = 0x41000a26,
		.phy0_dqs = 0x08080808,
		.phy1_dqs = 0x08080808,
		.phy0_dq = 0x08080808,
		.phy1_dq = 0x08080808,
		.phy0_tFS = 0x8,
		.phy1_tFS = 0x8,
		.phy0_pulld_dqs = 0xf,
		.phy1_pulld_dqs = 0xf,

		.lpddr3_ctrl_phy_reset = 0x1,
		.ctrl_start_point = 0x10,
		.ctrl_inc = 0x10,
		.ctrl_start = 0x1,
		.ctrl_dll_on = 0x1,
		.ctrl_ref = 0x8,

		.ctrl_force = 0x1a,
		.ctrl_rdlat = 0x0b,
		.ctrl_bstlen = 0x08,

		.fp_resync = 0x8,
		.iv_size = 0x7,
		.dfi_init_start = 1,
		.aref_en = 1,

		.rd_fetch = 0x3,

		.zq_mode_dds = 0x7,
		.zq_mode_term = 0x1,
		.zq_mode_noterm = 1,

		/*
		* Dynamic Clock: Always Running
		* Memory Burst length: 8
		* Number of chips: 1
		* Memory Bus width: 32 bit
		* Memory Type: DDR3
		* Additional Latancy for PLL: 0 Cycle
		*/
		.memcontrol = DMC_MEMCONTROL_CLK_STOP_DISABLE |
			DMC_MEMCONTROL_DPWRDN_DISABLE |
			DMC_MEMCONTROL_DPWRDN_ACTIVE_PRECHARGE |
			DMC_MEMCONTROL_TP_DISABLE |
			DMC_MEMCONTROL_DSREF_DISABLE |
			DMC_MEMCONTROL_ADD_LAT_PALL_CYCLE(0) |
			DMC_MEMCONTROL_MEM_TYPE_DDR3 |
			DMC_MEMCONTROL_MEM_WIDTH_32BIT |
			/* DMC_MEMCONTROL_NUM_CHIP_N set below */
			DMC_MEMCONTROL_BL_8 |
			DMC_MEMCONTROL_PZQ_DISABLE |
			DMC_MEMCONTROL_MRR_BYTE_7_0,
		.memconfig = DMC_MEMCONFIG_CHIP_MAP_SPLIT |
			DMC_MEMCONFIGx_CHIP_COL_10 |
			DMC_MEMCONFIGx_CHIP_ROW_15 |
			DMC_MEMCONFIGx_CHIP_BANK_8,
		.prechconfig_tp_cnt = 0xff,
		.dpwrdn_cyc = 0xff,
		.dsref_cyc = 0xffff,
		.concontrol = DMC_CONCONTROL_DFI_INIT_START_DISABLE |
			DMC_CONCONTROL_TIMEOUT_LEVEL0 |
			DMC_CONCONTROL_RD_FETCH_DISABLE |
			DMC_CONCONTROL_EMPTY_DISABLE |
			DMC_CONCONTROL_AREF_EN_DISABLE |
			DMC_CONCONTROL_IO_PD_CON_DISABLE,
		.dmc_channels = 1,
		.chips_per_channel = 1,		/* Modified below */
		.chips_to_configure = 1,	/* Modified below */
		.send_zq_init = 1,
		.gate_leveling_enable = 1,
};


/*
 * This function is a test vector for sw read leveling,
 * it compares the read data with the written data.
 *
 * @param ch			DMC channel number
 * @param byte_lane		which DQS byte offset,
 * 				possible values are 0,1,2,3
 * @returns			TRUE if memory was good, FALSE if not.
 */
static int dmc_valid_window_test_vector(int ch, int byte_lane)
{
	unsigned int read_data;
	unsigned int mask;
	int i;

	mask = 0xFF << (8 * byte_lane);

	for (i = 0; i < ARRAY_SIZE(test_pattern); i++) {
		read_data = readl(test_addr + i*4 + ch*0x80);
		if ((read_data & mask) != (test_pattern[i] & mask))
			return FALSE;
	}

	return TRUE;
}

/*
 * This function returns current read offset value.
 *
 * @param phy_ctrl	pointer to the current phy controller
 */
static unsigned int dmc_get_read_offset_value(
					struct exynos5_phy_control *phy_ctrl)
{
	return readl(&phy_ctrl->phy_con4);
}

/*
 * This function performs resync, so that slave DLL is updated.
 *
 * @param phy_ctrl	pointer to the current phy controller
 */
static void ddr_phy_set_do_resync(struct exynos5_phy_control *phy_ctrl)
{
	setbits_le32(&phy_ctrl->phy_con10, PHY_CON10_CTRL_OFFSETR3);
	clrbits_le32(&phy_ctrl->phy_con10, PHY_CON10_CTRL_OFFSETR3);
}

/*
 * This function sets read offset value register with 'offset'.
 *
 * ...we also call call ddr_phy_set_do_resync().
 *
 * @param phy_ctrl	pointer to the current phy controller
 * @param offset	offset to read DQS
 */
static void dmc_set_read_offset_value(struct exynos5_phy_control *phy_ctrl,
							unsigned int offset)
{
	writel(offset, &phy_ctrl->phy_con4);
	ddr_phy_set_do_resync(phy_ctrl);
}

/*
 * Convert a 2s complement byte to a byte with a sign bit.
 *
 * NOTE: you shouldn't use normal math on the number returned by this function.
 *   As an example, -10 = 0xf6.  After this function -10 = 0x8a.  If you wanted
 *   to do math and get the average of 10 and -10 (should be 0):
 *     0x8a + 0xa = 0x94 (-108)
 *     0x94 / 2   = 0xca (-54)
 *   ...and 0xca = sign bit plus 0x4a, or -74
 *
 * Also note that you lose the ability to represent -128 since there are two
 * representations of 0.
 *
 * @param b	The byte to convert in two's complement.
 * @returns	The 7-bit value + sign bit.
 */

unsigned char make_signed_byte(signed char b)
{
	if (b < 0)
		return 0x80 | -b;
	else
		return b;
}

/*
 * Test various shifts starting at 'start' and going to 'end'.
 *
 * For each byte lane, we'll walk through shift starting at 'start' and going
 * to 'end' (inclusive).  When we are finally able to read the test pattern
 * we'll store the value in the results array.
 *
 * @param phy_ctrl		pointer to the current phy controller
 * @param ch			channel number
 * @param start			the start shift.  -127 to 127
 * @param end			the end shift.  -127 to 127
 * @param results		we'll store results for each byte lane.
 */

void test_shifts(struct exynos5_phy_control *phy_ctrl, int ch,
		 int start, int end, int results[NUM_BYTE_LANES])
{
	int incr = (start < end) ? 1 : -1;
	int byte_lane;

	for (byte_lane = 0; byte_lane < NUM_BYTE_LANES; byte_lane++) {
		int shift;

		dmc_set_read_offset_value(phy_ctrl, DEFAULT_DQS_X4);
		results[byte_lane] = DEFAULT_DQS;

		for (shift = start; shift != (end + incr); shift += incr) {
			unsigned int byte_offsetr;
			unsigned int offsetr;

			byte_offsetr = make_signed_byte(shift);

			offsetr = dmc_get_read_offset_value(phy_ctrl);
			offsetr &= ~(0xFF << (8 * byte_lane));
			offsetr |= (byte_offsetr << (8 * byte_lane));
			dmc_set_read_offset_value(phy_ctrl, offsetr);

			if (dmc_valid_window_test_vector(ch, byte_lane)) {
				results[byte_lane] = shift;
				break;
			}
		}
	}
}

/*
 * This function performs SW read leveling to compensate DQ-DQS skew at
 * receiver it first finds the optimal read offset value on each DQS
 * then applies the value to PHY.
 *
 * Read offset value has its min margin and max margin. If read offset
 * value exceeds its min or max margin, read data will have corruption.
 * To avoid this we are doing sw read leveling.
 *
 * SW read leveling is:
 * 1> Finding offset value's left_limit and right_limit
 * 2> and calculate its center value
 * 3> finally programs that center value to PHY
 * 4> then PHY gets its optimal offset value.
 *
 * @param phy_ctrl		pointer to the current phy controller
 * @param ch			channel number
 * @param coarse_lock_val	The coarse lock value read from PHY_CON13.
 * 				(0 - 0x7f)
 */
static void software_find_read_offset(struct exynos5_phy_control *phy_ctrl,
				      int ch, unsigned int coarse_lock_val)
{
	unsigned int offsetr_cent;
	int byte_lane;
	int left_limit;
	int right_limit;
	int left[NUM_BYTE_LANES];
	int right[NUM_BYTE_LANES];
	int i;

	/* Fill the memory with test patterns */
	for (i = 0; i < ARRAY_SIZE(test_pattern); i++)
		writel(test_pattern[i], test_addr + i*4 + ch*0x80);

	/* Figure out the limits we'll test with; keep -127 < limit < 127 */
	left_limit = DEFAULT_DQS - coarse_lock_val;
	right_limit = DEFAULT_DQS + coarse_lock_val;
	if (right_limit > 127)
		right_limit = 127;

	/* Fill in the location where reads were OK from left and right */
	test_shifts(phy_ctrl, ch, left_limit, right_limit, left);
	test_shifts(phy_ctrl, ch, right_limit, left_limit, right);

	/* Make a final value by taking the center between the left and right */
	offsetr_cent = 0;
	for (byte_lane = 0; byte_lane < NUM_BYTE_LANES; byte_lane++) {
		int temp_center;
		unsigned int vmwc;

		temp_center = (left[byte_lane] + right[byte_lane]) / 2;
		vmwc = make_signed_byte(temp_center);
		offsetr_cent |= vmwc << (8 * byte_lane);
	}
	dmc_set_read_offset_value(phy_ctrl, offsetr_cent);
}

int ddr3_mem_ctrl_init(int reset)
{
	struct exynos5420_clock *clk =
		(struct exynos5420_clock *)EXYNOS5_CLOCK_BASE;
	struct exynos5_phy_control *phy0_ctrl, *phy1_ctrl;
	struct exynos5_dmc *drex0, *drex1;
	struct exynos5_tzasc *tzasc0, *tzasc1;
	struct exynos5_power *pmu;
	struct mem_timings *mem = &ares_ddr3_timings;
	u32 val, nLockR, nLockW_phy0, nLockW_phy1;
	u32 lock0_info, lock1_info;
	int chip;
	int i;

	if (board_get_memory_size() > SZ_2G) {
		/* Need both controllers. */
		mem->memcontrol |= DMC_MEMCONTROL_NUM_CHIP_2;
		mem->chips_per_channel = 2;
		mem->chips_to_configure = 2;
	} else {
		/* 2GB requires a single controller */
		mem->memcontrol |= DMC_MEMCONTROL_NUM_CHIP_1;
	}

	phy0_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY0_BASE;
	phy1_ctrl = (struct exynos5_phy_control *)EXYNOS5_DMC_PHY1_BASE;
	drex0 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_0;
	drex1 = (struct exynos5_dmc *)EXYNOS5420_DMC_DREXI_1;
	tzasc0 = (struct exynos5_tzasc *)EXYNOS5420_DMC_TZASC_0;
	tzasc1 = (struct exynos5_tzasc *)EXYNOS5420_DMC_TZASC_1;
	pmu = (struct exynos5_power *)EXYNOS5420_POWER_BASE;

	/* Enable PAUSE for DREX */
	setbits_le32(&clk->pause, ENABLE_BIT);

	/* Enable BYPASS mode */
	setbits_le32(&clk->bpll_con1, BYPASS_EN);

	writel(MUX_BPLL_SEL_FOUTBPLL, &clk->clk_src_cdrex);
	do {
		val = readl(&clk->clk_mux_stat_cdrex);
		val &= BPLL_SEL_MASK;
	} while (val != FOUTBPLL);

	clrbits_le32(&clk->bpll_con1, BYPASS_EN);

	/* Specify the DDR memory type as DDR3 */
	val = readl(&phy0_ctrl->phy_con0);
	val &= ~(PHY_CON0_CTRL_DDR_MODE_MASK << PHY_CON0_CTRL_DDR_MODE_SHIFT);
	val |= (DDR_MODE_DDR3 << PHY_CON0_CTRL_DDR_MODE_SHIFT);
	writel(val, &phy0_ctrl->phy_con0);

	val = readl(&phy1_ctrl->phy_con0);
	val &= ~(PHY_CON0_CTRL_DDR_MODE_MASK << PHY_CON0_CTRL_DDR_MODE_SHIFT);
	val |= (DDR_MODE_DDR3 << PHY_CON0_CTRL_DDR_MODE_SHIFT);
	writel(val, &phy1_ctrl->phy_con0);

	/* Set Read Latency and Burst Length for PHY0 and PHY1 */
	val = (mem->ctrl_bstlen << PHY_CON42_CTRL_BSTLEN_SHIFT) |
		(mem->ctrl_rdlat << PHY_CON42_CTRL_RDLAT_SHIFT);
	writel(val, &phy0_ctrl->phy_con42);
	writel(val, &phy1_ctrl->phy_con42);

	val = readl(&phy0_ctrl->phy_con26);
	val &= ~(T_WRDATA_EN_MASK << T_WRDATA_EN_OFFSET);
	val |= (T_WRDATA_EN_DDR3 << T_WRDATA_EN_OFFSET);
	writel(val, &phy0_ctrl->phy_con26);

	val = readl(&phy1_ctrl->phy_con26);
	val &= ~(T_WRDATA_EN_MASK << T_WRDATA_EN_OFFSET);
	val |= (T_WRDATA_EN_DDR3 << T_WRDATA_EN_OFFSET);
	writel(val, &phy1_ctrl->phy_con26);

	/* Set Driver strength for CK, CKE, CS & CA to 0x7
	 * Set Driver strength for Data Slice 0~3 to 0x7
	 */
	val = (0x7 << CA_CK_DRVR_DS_OFFSET) | (0x7 << CA_CKE_DRVR_DS_OFFSET) |
		(0x7 << CA_CS_DRVR_DS_OFFSET) | (0x7 << CA_ADR_DRVR_DS_OFFSET);
	val |= (0x7 << DA_3_DS_OFFSET) | (0x7 << DA_2_DS_OFFSET) |
		(0x7 << DA_1_DS_OFFSET) | (0x7 << DA_0_DS_OFFSET);
	writel(val, &phy0_ctrl->phy_con39);
	writel(val, &phy1_ctrl->phy_con39);

	/* ZQ Calibration */
	if (dmc_config_zq(mem, phy0_ctrl, phy1_ctrl))
		return SETUP_ERR_ZQ_CALIBRATION_FAILURE;

	clrbits_le32(&phy0_ctrl->phy_con16, ZQ_CLK_DIV_EN);
	clrbits_le32(&phy1_ctrl->phy_con16, ZQ_CLK_DIV_EN);

	/* DQ Signal */
	val = readl(&phy0_ctrl->phy_con14);
	val |= mem->phy0_pulld_dqs;
	writel(val, &phy0_ctrl->phy_con14);
	val = readl(&phy1_ctrl->phy_con14);
	val |= mem->phy1_pulld_dqs;
	writel(val, &phy1_ctrl->phy_con14);

	val = MEM_TERM_EN | PHY_TERM_EN;
	writel(val, &drex0->phycontrol0);
	writel(val, &drex1->phycontrol0);

	writel(mem->concontrol |
		(mem->dfi_init_start << CONCONTROL_DFI_INIT_START_SHIFT) |
		(mem->rd_fetch << CONCONTROL_RD_FETCH_SHIFT),
		&drex0->concontrol);
	writel(mem->concontrol |
		(mem->dfi_init_start << CONCONTROL_DFI_INIT_START_SHIFT) |
		(mem->rd_fetch << CONCONTROL_RD_FETCH_SHIFT),
		&drex1->concontrol);

	do {
		val = readl(&drex0->phystatus);
	} while ((val & DFI_INIT_COMPLETE) != DFI_INIT_COMPLETE);
	do {
		val = readl(&drex1->phystatus);
	} while ((val & DFI_INIT_COMPLETE) != DFI_INIT_COMPLETE);

	clrbits_le32(&drex0->concontrol, DFI_INIT_START);
	clrbits_le32(&drex1->concontrol, DFI_INIT_START);

	update_reset_dll(drex0, DDR_MODE_DDR3);
	update_reset_dll(drex1, DDR_MODE_DDR3);

	/* Set Base Address:
	 * 0x2000_0000 ~ 0x5FFF_FFFF
	 * 0x6000_0000 ~ 0x9FFF_FFFF
	 */
	/* MEMBASECONFIG0 */
	val = DMC_MEMBASECONFIGx_CHIP_BASE(DMC_CHIP_BASE_0) |
		DMC_MEMBASECONFIGx_CHIP_MASK(DMC_CHIP_MASK);
	writel(val, &tzasc0->membaseconfig0);
	writel(val, &tzasc1->membaseconfig0);

	/* MEMBASECONFIG1 */
	val = DMC_MEMBASECONFIGx_CHIP_BASE(DMC_CHIP_BASE_1) |
		DMC_MEMBASECONFIGx_CHIP_MASK(DMC_CHIP_MASK);
	writel(val, &tzasc0->membaseconfig1);
	writel(val, &tzasc1->membaseconfig1);

	/* Memory Channel Inteleaving Size
	 * Ares Channel interleaving = 128 bytes
	 */
	/* MEMCONFIG0/1 */
	writel(mem->memconfig, &tzasc0->memconfig0);
	writel(mem->memconfig, &tzasc1->memconfig0);
	writel(mem->memconfig, &tzasc0->memconfig1);
	writel(mem->memconfig, &tzasc1->memconfig1);

	/* Precharge Configuration */
	writel(mem->prechconfig_tp_cnt << PRECHCONFIG_TP_CNT_SHIFT,
		&drex0->prechconfig0);
	writel(mem->prechconfig_tp_cnt << PRECHCONFIG_TP_CNT_SHIFT,
		&drex1->prechconfig0);

	/* TimingRow, TimingData, TimingPower and Timingaref
	 * values as per Memory AC parameters
	 */
	writel(mem->timing_ref, &drex0->timingref);
	writel(mem->timing_ref, &drex1->timingref);
	writel(mem->timing_row, &drex0->timingrow);
	writel(mem->timing_row, &drex1->timingrow);
	writel(mem->timing_data, &drex0->timingdata);
	writel(mem->timing_data, &drex1->timingdata);
	writel(mem->timing_power, &drex0->timingpower);
	writel(mem->timing_power, &drex1->timingpower);

	if (reset) {
		/* Send NOP, MRS and ZQINIT commands
		 * Sending MRS command will reset the DRAM. We should not be
		 * reseting the DRAM after resume, this will lead to memory
		 * corruption as DRAM content is lost after DRAM reset
		 */
		dmc_config_mrs(mem, drex0);
		dmc_config_mrs(mem, drex1);
	}

	/*
	 * Get PHY_CON13 from both phys.  Gate CLKM around reading since
	 * PHY_CON13 is glitchy when CLKM is running.  We're paranoid and
	 * wait until we get a "fine lock", though a coarse lock is probably
	 * OK (we only use the coarse numbers below).  We try to gate the
	 * clock for as short a time as possible in case SDRAM is somehow
	 * sensitive.  sdelay(10) in the loop is arbitrary to make sure
	 * there is some time for PHY_CON13 to get updated.  In practice
	 * no delay appears to be needed.
	 */
	val = readl(&clk->clk_gate_bus_cdrex);
	while (true) {
		writel(val & ~0x1, &clk->clk_gate_bus_cdrex);
		lock0_info = readl(&phy0_ctrl->phy_con13);
		writel(val, &clk->clk_gate_bus_cdrex);

		if ((lock0_info & CTRL_FINE_LOCKED) == CTRL_FINE_LOCKED)
			break;

		sdelay(10);
	}
	while (true) {
		writel(val & ~0x2, &clk->clk_gate_bus_cdrex);
		lock1_info = readl(&phy1_ctrl->phy_con13);
		writel(val, &clk->clk_gate_bus_cdrex);

		if ((lock1_info & CTRL_FINE_LOCKED) == CTRL_FINE_LOCKED)
			break;

		sdelay(10);
	}

	if (!reset) {
		/*
		 * During Suspend-Resume & S/W-Reset, as soon as PMU releases
		 * pad retention, CKE goes high. This causes memory contents
		 * not to be retained during DRAM initialization. Therfore,
		 * there is a new control register(0x100431e8[28]) which lets us
		 * release pad retention and retain the memory content until the
		 * initialization is complete.
		 */
		writel(PAD_RETENTION_DRAM_COREBLK_VAL,
		       PAD_RETENTION_DRAM_COREBLK_OPTION);
		do {
			val = readl(PAD_RETENTION_DRAM_STATUS);
		} while (val != 0x1);

		/*
		 * CKE PAD retention disables DRAM self-refresh mode.
		 * Send auto refresh command for DRAM refresh.
		 */
		for (i = 0; i < 128; i++) {
			for (chip = 0; chip < mem->chips_to_configure; chip++) {
				writel(DIRECT_CMD_REFA |
				       (chip << DIRECT_CMD_CHIP_SHIFT),
				       &drex0->directcmd);
				writel(DIRECT_CMD_REFA |
				       (chip << DIRECT_CMD_CHIP_SHIFT),
				       &drex1->directcmd);
			}
		}
	}

	if (mem->gate_leveling_enable) {

		writel(PHY_CON0_RESET_VAL, &phy0_ctrl->phy_con0);
		writel(PHY_CON0_RESET_VAL, &phy1_ctrl->phy_con0);

		setbits_le32(&phy0_ctrl->phy_con0, P0_CMD_EN);
		setbits_le32(&phy1_ctrl->phy_con0, P0_CMD_EN);

		val = PHY_CON2_RESET_VAL;
		val |= INIT_DESKEW_EN;
		writel(val, &phy0_ctrl->phy_con2);
		writel(val, &phy1_ctrl->phy_con2);

		val =  readl(&phy0_ctrl->phy_con1);
		val |= (RDLVL_PASS_ADJ_VAL << RDLVL_PASS_ADJ_OFFSET);
		writel(val, &phy0_ctrl->phy_con1);

		val =  readl(&phy1_ctrl->phy_con1);
		val |= (RDLVL_PASS_ADJ_VAL << RDLVL_PASS_ADJ_OFFSET);
		writel(val, &phy1_ctrl->phy_con1);

		nLockW_phy0 = (lock0_info & CTRL_LOCK_COARSE_MASK) >> 2;
		nLockR = readl(&phy0_ctrl->phy_con12);
		nLockR &= ~CTRL_DLL_ON;
		nLockR |= nLockW_phy0;
		writel(nLockR, &phy0_ctrl->phy_con12);

		nLockW_phy1 = (lock1_info & CTRL_LOCK_COARSE_MASK) >> 2;
		nLockR = readl(&phy1_ctrl->phy_con12);
		nLockR &= ~CTRL_DLL_ON;
		nLockR |= nLockW_phy1;
		writel(nLockR, &phy1_ctrl->phy_con12);

		val = (0x3 << DIRECT_CMD_BANK_SHIFT) | 0x4;
		for (chip = 0; chip < mem->chips_to_configure; chip++) {
			writel(val | (chip << DIRECT_CMD_CHIP_SHIFT),
			       &drex0->directcmd);
			writel(val | (chip << DIRECT_CMD_CHIP_SHIFT),
			       &drex1->directcmd);
		}

		setbits_le32(&phy0_ctrl->phy_con2, RDLVL_GATE_EN);
		setbits_le32(&phy1_ctrl->phy_con2, RDLVL_GATE_EN);

		setbits_le32(&phy0_ctrl->phy_con0, CTRL_SHGATE);
		setbits_le32(&phy1_ctrl->phy_con0, CTRL_SHGATE);

		val = readl(&phy0_ctrl->phy_con1);
		val &= ~(CTRL_GATEDURADJ_MASK);
		writel(val, &phy0_ctrl->phy_con1);

		val = readl(&phy1_ctrl->phy_con1);
		val &= ~(CTRL_GATEDURADJ_MASK);
		writel(val, &phy1_ctrl->phy_con1);

		writel(CTRL_RDLVL_GATE_ENABLE, &drex0->rdlvl_config);
		i = TIMEOUT;
		while (((readl(&drex0->phystatus) & RDLVL_COMPLETE_CHO) !=
			RDLVL_COMPLETE_CHO) && (i > 0)) {
			/*
			 * TODO(waihong): Comment on how long this take to
			 * timeout
			 */
			sdelay(100);
			i--;
		}
		if (!i)
			return SETUP_ERR_RDLV_COMPLETE_TIMEOUT;
		writel(CTRL_RDLVL_GATE_DISABLE, &drex0->rdlvl_config);

		writel(CTRL_RDLVL_GATE_ENABLE, &drex1->rdlvl_config);
		i = TIMEOUT;
		while (((readl(&drex1->phystatus) & RDLVL_COMPLETE_CHO) !=
			RDLVL_COMPLETE_CHO) && (i > 0)) {
			/*
			 * TODO(waihong): Comment on how long this take to
			 * timeout
			 */
			sdelay(100);
			i--;
		}
		if (!i)
			return SETUP_ERR_RDLV_COMPLETE_TIMEOUT;
		writel(CTRL_RDLVL_GATE_DISABLE, &drex1->rdlvl_config);

		writel(0, &phy0_ctrl->phy_con14);
		writel(0, &phy1_ctrl->phy_con14);

		val = (0x3 << DIRECT_CMD_BANK_SHIFT);
		for (chip = 0; chip < mem->chips_to_configure; chip++) {
			writel(val | (chip << DIRECT_CMD_CHIP_SHIFT),
			       &drex0->directcmd);
			writel(val | (chip << DIRECT_CMD_CHIP_SHIFT),
			       &drex1->directcmd);
		}

		/* Common Settings for Leveling */
		val = PHY_CON12_RESET_VAL;
		writel((val + nLockW_phy0), &phy0_ctrl->phy_con12);
		writel((val + nLockW_phy1), &phy1_ctrl->phy_con12);

		setbits_le32(&phy0_ctrl->phy_con2, DLL_DESKEW_EN);
		setbits_le32(&phy1_ctrl->phy_con2, DLL_DESKEW_EN);
	}

	/*
	 * Do software read leveling
	 *
	 * Do this before we turn on auto refresh since the auto refresh can
	 * be in conflict with the resync operation that's part of setting
	 * read leveling.
	 */
	if (!reset) {
		/* restore calibrated value after resume */
		dmc_set_read_offset_value(phy0_ctrl, readl(&pmu->pmu_spare1));
		dmc_set_read_offset_value(phy1_ctrl, readl(&pmu->pmu_spare2));
	} else {
		software_find_read_offset(phy0_ctrl, 0,
					  CTRL_LOCK_COARSE(lock0_info));
		software_find_read_offset(phy1_ctrl, 1,
					  CTRL_LOCK_COARSE(lock1_info));
		/* save calibrated value to restore after resume */
		writel(dmc_get_read_offset_value(phy0_ctrl), &pmu->pmu_spare1);
		writel(dmc_get_read_offset_value(phy1_ctrl), &pmu->pmu_spare2);
	}

	/* Send PALL command */
	dmc_config_prech(mem, drex0);
	dmc_config_prech(mem, drex1);

	writel(mem->memcontrol, &drex0->memcontrol);
	writel(mem->memcontrol, &drex1->memcontrol);

	/*
	 * Set DMC Concontrol: Enable auto-refresh counter, provide
	 * read data fetch cycles and enable DREX auto set powerdown
	 * for input buffer of I/O in none read memory state.
	 */
	writel(mem->concontrol | (mem->aref_en << CONCONTROL_AREF_EN_SHIFT) |
		(mem->rd_fetch << CONCONTROL_RD_FETCH_SHIFT)|
		DMC_CONCONTROL_IO_PD_CON(0x2),
		&drex0->concontrol);
	writel(mem->concontrol | (mem->aref_en << CONCONTROL_AREF_EN_SHIFT) |
		(mem->rd_fetch << CONCONTROL_RD_FETCH_SHIFT)|
		DMC_CONCONTROL_IO_PD_CON(0x2),
		&drex1->concontrol);

	/* Enable Clock Gating Control for DMC
	 * this saves around 25 mw dmc power as compared to the power
	 * consumption without these bits enabled
	 */
        setbits_le32(&drex0->cgcontrol, DMC_INTERNAL_CG);
        setbits_le32(&drex1->cgcontrol, DMC_INTERNAL_CG);

	/*
	 * As per exynos5422 UM ver 0.00 section 17.13.2.1
	 * CONCONTROL register bit 3 [update_mode], Exynos 5422 does not
	 * support the PHY initiated update. And it is recommanded to set
	 * this field to 1'b1 during initialization
	 *
	 * When we apply PHY-initiated mode, DLL lock value is determined
	 * once at DMC init time and not updated later when we change the MIF
	 * voltage based on ASV group in kernel. Applying MC-initiated mode
	 * makes sure that DLL tracing is ON so that silicon is able to
	 * compensate the voltage variation.
	 */
	val = readl(&drex0->concontrol);
	val |= CONCONTROL_UPDATE_MODE;
	writel(val , &drex0->concontrol);
	val = readl(&drex1->concontrol);
	val |= CONCONTROL_UPDATE_MODE;
	writel(val , &drex1->concontrol);

	return 0;
}
