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

#ifndef _TEGRA114_WARMBOOT_AVP_H_
#define _TEGRA114_WARMBOOT_AVP_H_

/* CRC_CCLK_BURST_POLICY_0 0x20 */
#define CCLK_PLLP_BURST_POLICY		0x20004444

/* CRC_PLLP_OUTA_0 0xa4 */
#define PLLP_OUTA_OUT1_RATIO_83		(83 << PLLP_OUT1_RATIO)
#define PLLP_OUTA_OUT2_RATIO_15		(15 << PLLP_OUT2_RATIO)
#define PLLP_OUTA_NRST (PLLP_OUTA_OUT2_RATIO_15 |	\
			PLL_OUT2_OVRRIDE |		\
			PLL_OUT2_CLKEN |		\
			PLL_OUT2_RSTN |			\
			PLLP_OUTA_OUT1_RATIO_83 |	\
			PLL_OUT_OVRRIDE |		\
			PLL_OUT_CLKEN |			\
			PLL_OUT_RSTN)

#define PLLP_OUTA_RST (PLLP_OUTA_OUT2_RATIO_15 |	\
			PLL_OUT2_OVRRIDE |		\
			PLL_OUT2_CLKEN |		\
			PLLP_OUTA_OUT1_RATIO_83 |	\
			PLL_OUT_OVRRIDE |		\
			PLL_OUT_CLKEN)

/* CRC_PLLP_OUTB_0 0xa8 */
#define PLLP_OUTB_OUT3_RATIO_6		(6 << PLLP_OUT3_RATIO)
#define PLLP_OUTB_OUT4_RATIO_6		(6 << PLLP_OUT4_RATIO)
#define PLLP_OUTB_NRST (PLLP_OUTB_OUT4_RATIO_6 |	\
			PLLP_OUT4_OVRRIDE |		\
			PLLP_OUT4_CLKEN |		\
			PLLP_OUT4_RSTN_DIS |		\
			PLLP_OUTB_OUT3_RATIO_6 |	\
			PLLP_OUT3_OVRRIDE |		\
			PLLP_OUT3_CLKEN |		\
			PLLP_OUT3_RSTN_DIS)

#define PLLP_OUTB_RST (PLLP_OUTB_OUT4_RATIO_6 |	\
			PLLP_OUT4_OVRRIDE |		\
			PLLP_OUT4_CLKEN |		\
			PLLP_OUTB_OUT3_RATIO_6 |	\
			PLLP_OUT3_OVRRIDE |		\
			PLLP_OUT3_CLKEN)

/* CRC_CLK_SOURCE_MSELECT_0, 0x3b4 */
#define MSELECT_CLK_DIVISOR_6		6

/* PMC_SCRATCH4 bit 31 defines which Cluster suspends (1 = LP Cluster) */
#define CPU_WAKEUP_CLUSTER		(1 << 31)

/* CPU_SOFTRST_CTRL2_0, 0x388 */
#define CAR2PMC_CPU_ACK_WIDTH_408	408

#endif	/* _TEGRA114_WARMBOOT_AVP_H_ */
