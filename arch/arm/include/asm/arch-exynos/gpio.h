/*
 * (C) Copyright 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H

#include <asm-generic/gpio.h>

#ifndef __ASSEMBLY__
struct s5p_gpio_bank {
	unsigned int	con;
	unsigned int	dat;
	unsigned int	pull;
	unsigned int	drv;
	unsigned int	pdn_con;
	unsigned int	pdn_pull;
	unsigned char	res1[8];
};

struct exynos4_gpio_part1 {
	struct s5p_gpio_bank a0;
	struct s5p_gpio_bank a1;
	struct s5p_gpio_bank b;
	struct s5p_gpio_bank c0;
	struct s5p_gpio_bank c1;
	struct s5p_gpio_bank d0;
	struct s5p_gpio_bank d1;
	struct s5p_gpio_bank e0;
	struct s5p_gpio_bank e1;
	struct s5p_gpio_bank e2;
	struct s5p_gpio_bank e3;
	struct s5p_gpio_bank e4;
	struct s5p_gpio_bank f0;
	struct s5p_gpio_bank f1;
	struct s5p_gpio_bank f2;
	struct s5p_gpio_bank f3;
};

struct exynos4_gpio_part2 {
	struct s5p_gpio_bank j0;
	struct s5p_gpio_bank j1;
	struct s5p_gpio_bank k0;
	struct s5p_gpio_bank k1;
	struct s5p_gpio_bank k2;
	struct s5p_gpio_bank k3;
	struct s5p_gpio_bank l0;
	struct s5p_gpio_bank l1;
	struct s5p_gpio_bank l2;
	struct s5p_gpio_bank y0;
	struct s5p_gpio_bank y1;
	struct s5p_gpio_bank y2;
	struct s5p_gpio_bank y3;
	struct s5p_gpio_bank y4;
	struct s5p_gpio_bank y5;
	struct s5p_gpio_bank y6;
	struct s5p_gpio_bank res1[80];
	struct s5p_gpio_bank x0;
	struct s5p_gpio_bank x1;
	struct s5p_gpio_bank x2;
	struct s5p_gpio_bank x3;
};

struct exynos4_gpio_part3 {
	struct s5p_gpio_bank z;
};

struct exynos4x12_gpio_part1 {
	struct s5p_gpio_bank a0;
	struct s5p_gpio_bank a1;
	struct s5p_gpio_bank b;
	struct s5p_gpio_bank c0;
	struct s5p_gpio_bank c1;
	struct s5p_gpio_bank d0;
	struct s5p_gpio_bank d1;
	struct s5p_gpio_bank res1[0x5];
	struct s5p_gpio_bank f0;
	struct s5p_gpio_bank f1;
	struct s5p_gpio_bank f2;
	struct s5p_gpio_bank f3;
	struct s5p_gpio_bank res2[0x2];
	struct s5p_gpio_bank j0;
	struct s5p_gpio_bank j1;
};

struct exynos4x12_gpio_part2 {
	struct s5p_gpio_bank res1[0x2];
	struct s5p_gpio_bank k0;
	struct s5p_gpio_bank k1;
	struct s5p_gpio_bank k2;
	struct s5p_gpio_bank k3;
	struct s5p_gpio_bank l0;
	struct s5p_gpio_bank l1;
	struct s5p_gpio_bank l2;
	struct s5p_gpio_bank y0;
	struct s5p_gpio_bank y1;
	struct s5p_gpio_bank y2;
	struct s5p_gpio_bank y3;
	struct s5p_gpio_bank y4;
	struct s5p_gpio_bank y5;
	struct s5p_gpio_bank y6;
	struct s5p_gpio_bank res2[0x3];
	struct s5p_gpio_bank m0;
	struct s5p_gpio_bank m1;
	struct s5p_gpio_bank m2;
	struct s5p_gpio_bank m3;
	struct s5p_gpio_bank m4;
	struct s5p_gpio_bank res3[0x48];
	struct s5p_gpio_bank x0;
	struct s5p_gpio_bank x1;
	struct s5p_gpio_bank x2;
	struct s5p_gpio_bank x3;
};

struct exynos4x12_gpio_part3 {
	struct s5p_gpio_bank z;
};

struct exynos4x12_gpio_part4 {
	struct s5p_gpio_bank v0;
	struct s5p_gpio_bank v1;
	struct s5p_gpio_bank res1[0x1];
	struct s5p_gpio_bank v2;
	struct s5p_gpio_bank v3;
	struct s5p_gpio_bank res2[0x1];
	struct s5p_gpio_bank v4;
};

struct exynos5420_gpio_part1 {
	struct s5p_gpio_bank a0;
	struct s5p_gpio_bank a1;
	struct s5p_gpio_bank a2;
	struct s5p_gpio_bank b0;
	struct s5p_gpio_bank b1;
	struct s5p_gpio_bank b2;
	struct s5p_gpio_bank b3;
	struct s5p_gpio_bank b4;
	struct s5p_gpio_bank h0;
};

struct exynos5420_gpio_part2 {
	struct s5p_gpio_bank y7; /* 0x1340_0000 */
	struct s5p_gpio_bank res[0x5f]; /*  */
	struct s5p_gpio_bank x0; /* 0x1340_0C00 */
	struct s5p_gpio_bank x1; /* 0x1340_0C20 */
	struct s5p_gpio_bank x2; /* 0x1340_0C40 */
	struct s5p_gpio_bank x3; /* 0x1340_0C60 */
};

struct exynos5420_gpio_part3 {
	struct s5p_gpio_bank c0;
	struct s5p_gpio_bank c1;
	struct s5p_gpio_bank c2;
	struct s5p_gpio_bank c3;
	struct s5p_gpio_bank c4;
	struct s5p_gpio_bank d1;
	struct s5p_gpio_bank y0;
	struct s5p_gpio_bank y1;
	struct s5p_gpio_bank y2;
	struct s5p_gpio_bank y3;
	struct s5p_gpio_bank y4;
	struct s5p_gpio_bank y5;
	struct s5p_gpio_bank y6;
};

struct exynos5420_gpio_part4 {
	struct s5p_gpio_bank e0; /* 0x1400_0000 */
	struct s5p_gpio_bank e1; /* 0x1400_0020 */
	struct s5p_gpio_bank f0; /* 0x1400_0040 */
	struct s5p_gpio_bank f1; /* 0x1400_0060 */
	struct s5p_gpio_bank g0; /* 0x1400_0080 */
	struct s5p_gpio_bank g1; /* 0x1400_00A0 */
	struct s5p_gpio_bank g2; /* 0x1400_00C0 */
	struct s5p_gpio_bank j4; /* 0x1400_00E0 */
};

struct exynos5420_gpio_part5 {
	struct s5p_gpio_bank z0; /* 0x0386_0000 */
};

struct exynos5_gpio_part1 {
	struct s5p_gpio_bank a0;
	struct s5p_gpio_bank a1;
	struct s5p_gpio_bank a2;
	struct s5p_gpio_bank b0;
	struct s5p_gpio_bank b1;
	struct s5p_gpio_bank b2;
	struct s5p_gpio_bank b3;
	struct s5p_gpio_bank c0;
	struct s5p_gpio_bank c1;
	struct s5p_gpio_bank c2;
	struct s5p_gpio_bank c3;
	struct s5p_gpio_bank d0;
	struct s5p_gpio_bank d1;
	struct s5p_gpio_bank y0;
	struct s5p_gpio_bank y1;
	struct s5p_gpio_bank y2;
	struct s5p_gpio_bank y3;
	struct s5p_gpio_bank y4;
	struct s5p_gpio_bank y5;
	struct s5p_gpio_bank y6;
};

struct exynos5_gpio_part2 {
	struct s5p_gpio_bank c4;
};

struct exynos5_gpio_part3 {
	struct s5p_gpio_bank x0;
	struct s5p_gpio_bank x1;
	struct s5p_gpio_bank x2;
	struct s5p_gpio_bank x3;
};

struct exynos5_gpio_part4 {
	struct s5p_gpio_bank e0;
	struct s5p_gpio_bank e1;
	struct s5p_gpio_bank f0;
	struct s5p_gpio_bank f1;
	struct s5p_gpio_bank g0;
	struct s5p_gpio_bank g1;
	struct s5p_gpio_bank g2;
	struct s5p_gpio_bank h0;
	struct s5p_gpio_bank h1;
};

struct exynos5_gpio_part5 {
	struct s5p_gpio_bank v0;
	struct s5p_gpio_bank v1;
};

struct exynos5_gpio_part6 {
	struct s5p_gpio_bank v2;
	struct s5p_gpio_bank v3;
};

struct exynos5_gpio_part7 {
	struct s5p_gpio_bank v4;
};

struct exynos5_gpio_part8 {
	struct s5p_gpio_bank z;
};


/* functions */
void s5p_gpio_cfg_pin(struct s5p_gpio_bank *bank, int gpio, int cfg);
void s5p_gpio_direction_output(struct s5p_gpio_bank *bank, int gpio, int en);
void s5p_gpio_direction_input(struct s5p_gpio_bank *bank, int gpio);
void s5p_gpio_set_value(struct s5p_gpio_bank *bank, int gpio, int en);
unsigned int s5p_gpio_get_value(struct s5p_gpio_bank *bank, int gpio);
void s5p_gpio_set_pull(struct s5p_gpio_bank *bank, int gpio, int mode);
void s5p_gpio_set_drv(struct s5p_gpio_bank *bank, int gpio, int mode);
void s5p_gpio_set_rate(struct s5p_gpio_bank *bank, int gpio, int mode);

/* GPIO pins per bank  */
#define GPIO_PER_BANK 8

#define exynos4_gpio_part1_get_nr(bank, pin) \
	((((((unsigned int) &(((struct exynos4_gpio_part1 *) \
			       EXYNOS4_GPIO_PART1_BASE)->bank)) \
	    - EXYNOS4_GPIO_PART1_BASE) / sizeof(struct s5p_gpio_bank)) \
	  * GPIO_PER_BANK) + pin)

#define EXYNOS4_GPIO_PART1_MAX ((sizeof(struct exynos4_gpio_part1) \
			    / sizeof(struct s5p_gpio_bank)) * GPIO_PER_BANK)

#define exynos4_gpio_part2_get_nr(bank, pin) \
	(((((((unsigned int) &(((struct exynos4_gpio_part2 *) \
				EXYNOS4_GPIO_PART2_BASE)->bank)) \
	    - EXYNOS4_GPIO_PART2_BASE) / sizeof(struct s5p_gpio_bank)) \
	  * GPIO_PER_BANK) + pin) + EXYNOS4_GPIO_PART1_MAX)

#define exynos4x12_gpio_part1_get_nr(bank, pin) \
	((((((unsigned int) &(((struct exynos4x12_gpio_part1 *) \
			       EXYNOS4X12_GPIO_PART1_BASE)->bank)) \
	    - EXYNOS4X12_GPIO_PART1_BASE) / sizeof(struct s5p_gpio_bank)) \
	  * GPIO_PER_BANK) + pin)

#define EXYNOS4X12_GPIO_PART1_MAX ((sizeof(struct exynos4x12_gpio_part1) \
			    / sizeof(struct s5p_gpio_bank)) * GPIO_PER_BANK)

#define exynos4x12_gpio_part2_get_nr(bank, pin) \
	(((((((unsigned int) &(((struct exynos4x12_gpio_part2 *) \
				EXYNOS4X12_GPIO_PART2_BASE)->bank)) \
	    - EXYNOS4X12_GPIO_PART2_BASE) / sizeof(struct s5p_gpio_bank)) \
	  * GPIO_PER_BANK) + pin) + EXYNOS4X12_GPIO_PART1_MAX)

#define EXYNOS4X12_GPIO_PART2_MAX ((sizeof(struct exynos4x12_gpio_part2) \
			    / sizeof(struct s5p_gpio_bank)) * GPIO_PER_BANK)

#define exynos4x12_gpio_part3_get_nr(bank, pin) \
	(((((((unsigned int) &(((struct exynos4x12_gpio_part3 *) \
				EXYNOS4X12_GPIO_PART3_BASE)->bank)) \
	    - EXYNOS4X12_GPIO_PART3_BASE) / sizeof(struct s5p_gpio_bank)) \
	  * GPIO_PER_BANK) + pin) + EXYNOS4X12_GPIO_PART2_MAX)

static inline unsigned int s5p_gpio_base(int nr)
{
	if (cpu_is_exynos4()) {
		if (nr < EXYNOS4_GPIO_PART1_MAX)
			return EXYNOS4_GPIO_PART1_BASE;
		else
			return EXYNOS4_GPIO_PART2_BASE;
	}

	return 0;
}

static inline unsigned int s5p_gpio_part_max(int nr)
{
	if (cpu_is_exynos4()) {
		if (nr < EXYNOS4_GPIO_PART1_MAX)
			return 0;
		else
			return EXYNOS4_GPIO_PART1_MAX;
	}

	return 0;
}

/* A list of valid GPIO numbers for the asm-generic/gpio.h interface */
enum exynos5_gpio_pin {
	/* GPIO_PART1_STARTS */
	EXYNOS5_GPIO_A00,
	EXYNOS5_GPIO_A01,
	EXYNOS5_GPIO_A02,
	EXYNOS5_GPIO_A03,
	EXYNOS5_GPIO_A04,
	EXYNOS5_GPIO_A05,
	EXYNOS5_GPIO_A06,
	EXYNOS5_GPIO_A07,
	EXYNOS5_GPIO_A10,
	EXYNOS5_GPIO_A11,
	EXYNOS5_GPIO_A12,
	EXYNOS5_GPIO_A13,
	EXYNOS5_GPIO_A14,
	EXYNOS5_GPIO_A15,
	EXYNOS5_GPIO_A16,
	EXYNOS5_GPIO_A17,
	EXYNOS5_GPIO_A20,
	EXYNOS5_GPIO_A21,
	EXYNOS5_GPIO_A22,
	EXYNOS5_GPIO_A23,
	EXYNOS5_GPIO_A24,
	EXYNOS5_GPIO_A25,
	EXYNOS5_GPIO_A26,
	EXYNOS5_GPIO_A27,
	EXYNOS5_GPIO_B00,
	EXYNOS5_GPIO_B01,
	EXYNOS5_GPIO_B02,
	EXYNOS5_GPIO_B03,
	EXYNOS5_GPIO_B04,
	EXYNOS5_GPIO_B05,
	EXYNOS5_GPIO_B06,
	EXYNOS5_GPIO_B07,
	EXYNOS5_GPIO_B10,
	EXYNOS5_GPIO_B11,
	EXYNOS5_GPIO_B12,
	EXYNOS5_GPIO_B13,
	EXYNOS5_GPIO_B14,
	EXYNOS5_GPIO_B15,
	EXYNOS5_GPIO_B16,
	EXYNOS5_GPIO_B17,
	EXYNOS5_GPIO_B20,
	EXYNOS5_GPIO_B21,
	EXYNOS5_GPIO_B22,
	EXYNOS5_GPIO_B23,
	EXYNOS5_GPIO_B24,
	EXYNOS5_GPIO_B25,
	EXYNOS5_GPIO_B26,
	EXYNOS5_GPIO_B27,
	EXYNOS5_GPIO_B30,
	EXYNOS5_GPIO_B31,
	EXYNOS5_GPIO_B32,
	EXYNOS5_GPIO_B33,
	EXYNOS5_GPIO_B34,
	EXYNOS5_GPIO_B35,
	EXYNOS5_GPIO_B36,
	EXYNOS5_GPIO_B37,
	EXYNOS5_GPIO_C00,
	EXYNOS5_GPIO_C01,
	EXYNOS5_GPIO_C02,
	EXYNOS5_GPIO_C03,
	EXYNOS5_GPIO_C04,
	EXYNOS5_GPIO_C05,
	EXYNOS5_GPIO_C06,
	EXYNOS5_GPIO_C07,
	EXYNOS5_GPIO_C10,
	EXYNOS5_GPIO_C11,
	EXYNOS5_GPIO_C12,
	EXYNOS5_GPIO_C13,
	EXYNOS5_GPIO_C14,
	EXYNOS5_GPIO_C15,
	EXYNOS5_GPIO_C16,
	EXYNOS5_GPIO_C17,
	EXYNOS5_GPIO_C20,
	EXYNOS5_GPIO_C21,
	EXYNOS5_GPIO_C22,
	EXYNOS5_GPIO_C23,
	EXYNOS5_GPIO_C24,
	EXYNOS5_GPIO_C25,
	EXYNOS5_GPIO_C26,
	EXYNOS5_GPIO_C27,
	EXYNOS5_GPIO_C30,
	EXYNOS5_GPIO_C31,
	EXYNOS5_GPIO_C32,
	EXYNOS5_GPIO_C33,
	EXYNOS5_GPIO_C34,
	EXYNOS5_GPIO_C35,
	EXYNOS5_GPIO_C36,
	EXYNOS5_GPIO_C37,
	EXYNOS5_GPIO_D00,
	EXYNOS5_GPIO_D01,
	EXYNOS5_GPIO_D02,
	EXYNOS5_GPIO_D03,
	EXYNOS5_GPIO_D04,
	EXYNOS5_GPIO_D05,
	EXYNOS5_GPIO_D06,
	EXYNOS5_GPIO_D07,
	EXYNOS5_GPIO_D10,
	EXYNOS5_GPIO_D11,
	EXYNOS5_GPIO_D12,
	EXYNOS5_GPIO_D13,
	EXYNOS5_GPIO_D14,
	EXYNOS5_GPIO_D15,
	EXYNOS5_GPIO_D16,
	EXYNOS5_GPIO_D17,
	EXYNOS5_GPIO_Y00,
	EXYNOS5_GPIO_Y01,
	EXYNOS5_GPIO_Y02,
	EXYNOS5_GPIO_Y03,
	EXYNOS5_GPIO_Y04,
	EXYNOS5_GPIO_Y05,
	EXYNOS5_GPIO_Y06,
	EXYNOS5_GPIO_Y07,
	EXYNOS5_GPIO_Y10,
	EXYNOS5_GPIO_Y11,
	EXYNOS5_GPIO_Y12,
	EXYNOS5_GPIO_Y13,
	EXYNOS5_GPIO_Y14,
	EXYNOS5_GPIO_Y15,
	EXYNOS5_GPIO_Y16,
	EXYNOS5_GPIO_Y17,
	EXYNOS5_GPIO_Y20,
	EXYNOS5_GPIO_Y21,
	EXYNOS5_GPIO_Y22,
	EXYNOS5_GPIO_Y23,
	EXYNOS5_GPIO_Y24,
	EXYNOS5_GPIO_Y25,
	EXYNOS5_GPIO_Y26,
	EXYNOS5_GPIO_Y27,
	EXYNOS5_GPIO_Y30,
	EXYNOS5_GPIO_Y31,
	EXYNOS5_GPIO_Y32,
	EXYNOS5_GPIO_Y33,
	EXYNOS5_GPIO_Y34,
	EXYNOS5_GPIO_Y35,
	EXYNOS5_GPIO_Y36,
	EXYNOS5_GPIO_Y37,
	EXYNOS5_GPIO_Y40,
	EXYNOS5_GPIO_Y41,
	EXYNOS5_GPIO_Y42,
	EXYNOS5_GPIO_Y43,
	EXYNOS5_GPIO_Y44,
	EXYNOS5_GPIO_Y45,
	EXYNOS5_GPIO_Y46,
	EXYNOS5_GPIO_Y47,
	EXYNOS5_GPIO_Y50,
	EXYNOS5_GPIO_Y51,
	EXYNOS5_GPIO_Y52,
	EXYNOS5_GPIO_Y53,
	EXYNOS5_GPIO_Y54,
	EXYNOS5_GPIO_Y55,
	EXYNOS5_GPIO_Y56,
	EXYNOS5_GPIO_Y57,
	EXYNOS5_GPIO_Y60,
	EXYNOS5_GPIO_Y61,
	EXYNOS5_GPIO_Y62,
	EXYNOS5_GPIO_Y63,
	EXYNOS5_GPIO_Y64,
	EXYNOS5_GPIO_Y65,
	EXYNOS5_GPIO_Y66,
	EXYNOS5_GPIO_Y67,

	/* GPIO_PART2_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_1,
	EXYNOS5_GPIO_C40 = EXYNOS5_GPIO_MAX_PORT_PART_1,
	EXYNOS5_GPIO_C41,
	EXYNOS5_GPIO_C42,
	EXYNOS5_GPIO_C43,
	EXYNOS5_GPIO_C44,
	EXYNOS5_GPIO_C45,
	EXYNOS5_GPIO_C46,
	EXYNOS5_GPIO_C47,

	/* GPIO_PART3_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_2,
	EXYNOS5_GPIO_X00 = EXYNOS5_GPIO_MAX_PORT_PART_2,
	EXYNOS5_GPIO_X01,
	EXYNOS5_GPIO_X02,
	EXYNOS5_GPIO_X03,
	EXYNOS5_GPIO_X04,
	EXYNOS5_GPIO_X05,
	EXYNOS5_GPIO_X06,
	EXYNOS5_GPIO_X07,
	EXYNOS5_GPIO_X10,
	EXYNOS5_GPIO_X11,
	EXYNOS5_GPIO_X12,
	EXYNOS5_GPIO_X13,
	EXYNOS5_GPIO_X14,
	EXYNOS5_GPIO_X15,
	EXYNOS5_GPIO_X16,
	EXYNOS5_GPIO_X17,
	EXYNOS5_GPIO_X20,
	EXYNOS5_GPIO_X21,
	EXYNOS5_GPIO_X22,
	EXYNOS5_GPIO_X23,
	EXYNOS5_GPIO_X24,
	EXYNOS5_GPIO_X25,
	EXYNOS5_GPIO_X26,
	EXYNOS5_GPIO_X27,
	EXYNOS5_GPIO_X30,
	EXYNOS5_GPIO_X31,
	EXYNOS5_GPIO_X32,
	EXYNOS5_GPIO_X33,
	EXYNOS5_GPIO_X34,
	EXYNOS5_GPIO_X35,
	EXYNOS5_GPIO_X36,
	EXYNOS5_GPIO_X37,

	/* GPIO_PART4_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_3,
	EXYNOS5_GPIO_E00 = EXYNOS5_GPIO_MAX_PORT_PART_3,
	EXYNOS5_GPIO_E01,
	EXYNOS5_GPIO_E02,
	EXYNOS5_GPIO_E03,
	EXYNOS5_GPIO_E04,
	EXYNOS5_GPIO_E05,
	EXYNOS5_GPIO_E06,
	EXYNOS5_GPIO_E07,
	EXYNOS5_GPIO_E10,
	EXYNOS5_GPIO_E11,
	EXYNOS5_GPIO_E12,
	EXYNOS5_GPIO_E13,
	EXYNOS5_GPIO_E14,
	EXYNOS5_GPIO_E15,
	EXYNOS5_GPIO_E16,
	EXYNOS5_GPIO_E17,
	EXYNOS5_GPIO_F00,
	EXYNOS5_GPIO_F01,
	EXYNOS5_GPIO_F02,
	EXYNOS5_GPIO_F03,
	EXYNOS5_GPIO_F04,
	EXYNOS5_GPIO_F05,
	EXYNOS5_GPIO_F06,
	EXYNOS5_GPIO_F07,
	EXYNOS5_GPIO_F10,
	EXYNOS5_GPIO_F11,
	EXYNOS5_GPIO_F12,
	EXYNOS5_GPIO_F13,
	EXYNOS5_GPIO_F14,
	EXYNOS5_GPIO_F15,
	EXYNOS5_GPIO_F16,
	EXYNOS5_GPIO_F17,
	EXYNOS5_GPIO_G00,
	EXYNOS5_GPIO_G01,
	EXYNOS5_GPIO_G02,
	EXYNOS5_GPIO_G03,
	EXYNOS5_GPIO_G04,
	EXYNOS5_GPIO_G05,
	EXYNOS5_GPIO_G06,
	EXYNOS5_GPIO_G07,
	EXYNOS5_GPIO_G10,
	EXYNOS5_GPIO_G11,
	EXYNOS5_GPIO_G12,
	EXYNOS5_GPIO_G13,
	EXYNOS5_GPIO_G14,
	EXYNOS5_GPIO_G15,
	EXYNOS5_GPIO_G16,
	EXYNOS5_GPIO_G17,
	EXYNOS5_GPIO_G20,
	EXYNOS5_GPIO_G21,
	EXYNOS5_GPIO_G22,
	EXYNOS5_GPIO_G23,
	EXYNOS5_GPIO_G24,
	EXYNOS5_GPIO_G25,
	EXYNOS5_GPIO_G26,
	EXYNOS5_GPIO_G27,
	EXYNOS5_GPIO_H00,
	EXYNOS5_GPIO_H01,
	EXYNOS5_GPIO_H02,
	EXYNOS5_GPIO_H03,
	EXYNOS5_GPIO_H04,
	EXYNOS5_GPIO_H05,
	EXYNOS5_GPIO_H06,
	EXYNOS5_GPIO_H07,
	EXYNOS5_GPIO_H10,
	EXYNOS5_GPIO_H11,
	EXYNOS5_GPIO_H12,
	EXYNOS5_GPIO_H13,
	EXYNOS5_GPIO_H14,
	EXYNOS5_GPIO_H15,
	EXYNOS5_GPIO_H16,
	EXYNOS5_GPIO_H17,

	/* GPIO_PART4_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_4,
	EXYNOS5_GPIO_V00 = EXYNOS5_GPIO_MAX_PORT_PART_4,
	EXYNOS5_GPIO_V01,
	EXYNOS5_GPIO_V02,
	EXYNOS5_GPIO_V03,
	EXYNOS5_GPIO_V04,
	EXYNOS5_GPIO_V05,
	EXYNOS5_GPIO_V06,
	EXYNOS5_GPIO_V07,
	EXYNOS5_GPIO_V10,
	EXYNOS5_GPIO_V11,
	EXYNOS5_GPIO_V12,
	EXYNOS5_GPIO_V13,
	EXYNOS5_GPIO_V14,
	EXYNOS5_GPIO_V15,
	EXYNOS5_GPIO_V16,
	EXYNOS5_GPIO_V17,

	/* GPIO_PART5_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_5,
	EXYNOS5_GPIO_V20 = EXYNOS5_GPIO_MAX_PORT_PART_5,
	EXYNOS5_GPIO_V21,
	EXYNOS5_GPIO_V22,
	EXYNOS5_GPIO_V23,
	EXYNOS5_GPIO_V24,
	EXYNOS5_GPIO_V25,
	EXYNOS5_GPIO_V26,
	EXYNOS5_GPIO_V27,
	EXYNOS5_GPIO_V30,
	EXYNOS5_GPIO_V31,
	EXYNOS5_GPIO_V32,
	EXYNOS5_GPIO_V33,
	EXYNOS5_GPIO_V34,
	EXYNOS5_GPIO_V35,
	EXYNOS5_GPIO_V36,
	EXYNOS5_GPIO_V37,

	/* GPIO_PART6_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_6,
	EXYNOS5_GPIO_V40 = EXYNOS5_GPIO_MAX_PORT_PART_6,
	EXYNOS5_GPIO_V41,
	EXYNOS5_GPIO_V42,
	EXYNOS5_GPIO_V43,
	EXYNOS5_GPIO_V44,
	EXYNOS5_GPIO_V45,
	EXYNOS5_GPIO_V46,
	EXYNOS5_GPIO_V47,

	/* GPIO_PART6_STARTS */
	EXYNOS5_GPIO_MAX_PORT_PART_7,
	EXYNOS5_GPIO_Z0 = EXYNOS5_GPIO_MAX_PORT_PART_7,
	EXYNOS5_GPIO_Z1,
	EXYNOS5_GPIO_Z2,
	EXYNOS5_GPIO_Z3,
	EXYNOS5_GPIO_Z4,
	EXYNOS5_GPIO_Z5,
	EXYNOS5_GPIO_Z6,
	EXYNOS5_GPIO_MAX_PORT
};

struct gpio_info {
	unsigned int reg_addr;	/* Address of register for this part */
	unsigned int max_gpio;	/* Maximum GPIO in this part */
};

#define EXYNOS5_GPIO_NUM_PARTS	8
static struct gpio_info exynos5_gpio_data[EXYNOS5_GPIO_NUM_PARTS] = {
	{ EXYNOS5_GPIO_PART1_BASE, EXYNOS5_GPIO_MAX_PORT_PART_1 },
	{ EXYNOS5_GPIO_PART2_BASE, EXYNOS5_GPIO_MAX_PORT_PART_2 },
	{ EXYNOS5_GPIO_PART3_BASE, EXYNOS5_GPIO_MAX_PORT_PART_3 },
	{ EXYNOS5_GPIO_PART4_BASE, EXYNOS5_GPIO_MAX_PORT_PART_4 },
	{ EXYNOS5_GPIO_PART5_BASE, EXYNOS5_GPIO_MAX_PORT_PART_5 },
	{ EXYNOS5_GPIO_PART6_BASE, EXYNOS5_GPIO_MAX_PORT_PART_6 },
	{ EXYNOS5_GPIO_PART7_BASE, EXYNOS5_GPIO_MAX_PORT_PART_7 },
	{ EXYNOS5_GPIO_PART8_BASE, EXYNOS5_GPIO_MAX_PORT },
};

static inline struct gpio_info *get_gpio_data(void)
{
	if (cpu_is_exynos5())
		return exynos5_gpio_data;
	else
		return NULL;
}

static inline unsigned int get_bank_num(void)
{
	if (cpu_is_exynos5())
		return EXYNOS5_GPIO_NUM_PARTS;
	else
		return 0;
}

void gpio_cfg_pin(int gpio, int cfg);
void gpio_set_pull(int gpio, int value);
void gpio_set_drv(int gpio, int drv);
int gpio_direction_output(unsigned gpio, int value);
int gpio_set_value(unsigned gpio, int value);
#endif

/* Pin configurations */
#define GPIO_INPUT	0x0
#define GPIO_OUTPUT	0x1
#define GPIO_IRQ	0xf
#define GPIO_FUNC(x)	(x)

/* Pull mode */
#define GPIO_PULL_NONE	0x0
#define GPIO_PULL_DOWN	0x1
#define GPIO_PULL_UP	0x3

/* Drive Strength level */
#define GPIO_DRV_1X	0x0
#define GPIO_DRV_3X	0x1
#define GPIO_DRV_2X	0x2
#define GPIO_DRV_4X	0x3
#define GPIO_DRV_FAST	0x0
#define GPIO_DRV_SLOW	0x1

/**
 * Decode a list of GPIOs into an integer.
 *
 * TODO(sjg@chromium.org): This could perhaps become a generic function?
 *
 * Each GPIO pin can be put into three states using external resistors:
 *	- pulled up
 *	- pulled down
 *	- not connected
 *
 * Read each GPIO in turn to produce an integer value. The first GPIO
 * produces a number 1 * (0 to 2), the second produces 3 * (0 to 2), etc.
 * In this way, each GPIO increases the number of possible states by a
 * factor of 3.
 *
 * @param gpio_list	List of GPIO numbers to decode
 * @param count		Number of GPIOs in list
 * @return -1 if the value cannot be determined, or any GPIO number is
 *		invalid. Otherwise returns the calculated value
 */

int gpio_decode_number(unsigned gpio_list[], int count);

/**
 * Set GPIO pull mode.
 *
 * @param gpio	GPIO pin
 * @param mode	Either GPIO_PULL_DOWN or GPIO_PULL_UP
 */
void gpio_set_pull(int gpio, int mode);

#endif
