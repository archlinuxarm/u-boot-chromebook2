/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * EXYNOS - CPU frequency scaling support
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <power/pmic.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpufreq.h>

/* APLL CON0 */
#define CON0_LOCK_BIT_MASK	(0x1 << 29)
#define MDIV_MASK(x)		(x << 16)
#define PDIV_MASK(x)		(x << 8)
#define SDIV_MASK(x)		(x << 0)
#define APLL_PMS_MASK		 ~(MDIV_MASK(0x3ff)	\
				| PDIV_MASK(0x3f) | SDIV_MASK(0x7))

/* MUX_STAT CPU select */
#define MUX_CPU_NONE		(0x7 << 16)
#define MUX_CPU_MOUT_APLL	(0x1 << 16)

/* CLK_DIV_CPU0_VAL */
#define DIV_CPU0_RSVD		~((0x7 << 28)	\
				| (0x7 << 24)	\
				| (0x7 << 20)	\
				| (0x7 << 16)	\
				| (0x7 << 12)	\
				| (0x7 << 8)	\
				| (0x7 << 4)	\
				| (0x7))

/* CLK_DIV_CPU1 */
#define DIV_CPU1_RSVD		~((0x7 << 4) | (0x7))

struct cpufreq_clkdiv {
	uint8_t cpu0;
	uint8_t cpu1;
};

struct cpufreq_data {
	uint8_t arm;
	uint8_t cpud;
	uint8_t acp;
	uint8_t periph;
	uint8_t atb;
	uint8_t pclk_dbg;
	uint8_t apll;
	uint8_t arm2;
	uint8_t copy;
	uint8_t hpm;
	uint32_t apll_mdiv;
	uint32_t apll_pdiv;
	uint32_t apll_sdiv;
	uint32_t volt;
};

static enum cpufreq_level old_freq_level;
static struct cpufreq_clkdiv exynos5250_clkdiv;

/*
 * Clock divider, PMS and ASV group voltage values corresponding
 * to frequencies.
 */
static struct cpufreq_data exynos5250_data_table[CPU_FREQ_LCOUNT] = {
	/*
	 * { ARM, CPUD, ACP, PERIPH, ATB, PCLK_DBG, APLL, ARM2, COPY, HPM,
	 * APLL_MDIV, APLL_PDIV, APLL_SDIV, VOLTAGE }
	 */
	{ 0, 1, 7, 7, 1, 1, 1, 0, 0, 2, 100, 3, 2, 925000 },	/* 200 MHz */
	{ 0, 1, 7, 7, 1, 1, 1, 0, 0, 2, 200, 4, 2, 937500 },	/* 300 MHz */
	{ 0, 1, 7, 7, 2, 1, 1, 0, 0, 2, 100, 3, 1, 950000 },	/* 400 MHz */
	{ 0, 1, 7, 7, 2, 1, 1, 0, 0, 2, 125, 3, 1, 975000 },	/* 500 MHz */
	{ 0, 1, 7, 7, 3, 1, 1, 0, 0, 2, 200, 4, 1, 1000000 },	/* 600 MHz */
	{ 0, 1, 7, 7, 3, 1, 1, 0, 0, 2, 175, 3, 1, 1012500 },	/* 700 MHz */
	{ 0, 1, 7, 7, 4, 1, 2, 0, 0, 2, 100, 3, 0, 1025000 },	/* 800 MHz */
	{ 0, 1, 7, 7, 4, 1, 2, 0, 0, 2, 150, 4, 0, 1050000 },	/* 900 MHz */
	{ 0, 1, 7, 7, 4, 1, 2, 0, 0, 2, 125, 3, 0, 1075000 },	/* 1000 MHz */
	{ 0, 3, 7, 7, 5, 1, 3, 0, 0, 2, 275, 6, 0, 1100000 },	/* 1100 MHz */
	{ 0, 2, 7, 7, 5, 1, 3, 0, 0, 2, 200, 4, 0, 1125000 },	/* 1200 MHz */
	{ 0, 2, 7, 7, 6, 1, 3, 0, 0, 2, 325, 6, 0, 1150000 },	/* 1300 MHz */
	{ 0, 2, 7, 7, 6, 1, 4, 0, 0, 2, 175, 3, 0, 1200000 },	/* 1400 MHz */
	{ 0, 2, 7, 7, 7, 1, 4, 0, 0, 2, 250, 4, 0, 1225000 },	/* 1500 MHz */
	{ 0, 3, 7, 7, 7, 1, 4, 0, 0, 2, 200, 3, 0, 1250000 },	/* 1600 MHz */
	{ 0, 3, 7, 7, 7, 2, 5, 0, 0, 2, 425, 6, 0, 1300000 },	/* 1700 MHz */
};

/*
 * Set clock divider values to alter exynos5 frequency
 *
 * @param new_freq_level	enum cpufreq_level, states new frequency
 * @param clk			struct exynos5_clock *,
 *				provides clock reg addresses
 */
static void exynos5_set_clkdiv(enum cpufreq_level new_freq_level,
					struct exynos5_clock *clk)
{
	unsigned int val;

	/* Change Divider - CPU0 */
	val = exynos5250_clkdiv.cpu0;

	val |= (exynos5250_data_table[new_freq_level].arm << 0) |
		(exynos5250_data_table[new_freq_level].cpud << 4) |
		(exynos5250_data_table[new_freq_level].acp << 8) |
		(exynos5250_data_table[new_freq_level].periph << 12) |
		(exynos5250_data_table[new_freq_level].atb << 16) |
		(exynos5250_data_table[new_freq_level].pclk_dbg << 20) |
		(exynos5250_data_table[new_freq_level].apll << 24) |
		(exynos5250_data_table[new_freq_level].arm2 << 28);

	writel(val, &clk->div_cpu0);

	/* Wait for CPU0 divider to be stable */
	while (readl(&clk->div_stat_cpu0) & 0x11111111)
		;

	/* Change Divider - CPU1 */
	val = exynos5250_clkdiv.cpu1;

	val |= (exynos5250_data_table[new_freq_level].copy << 0) |
		(exynos5250_data_table[new_freq_level].hpm << 4);

	writel(val, &clk->div_cpu1);

	/* Wait for CPU1 divider to be stable */
	while (readl(&clk->div_stat_cpu1) & 0x11)
		;
}

/*
 * Set APLL values to alter exynos5 frequency
 *
 * @param new_freq_level	enum cpufreq_level, states new frequency
 * @param clk			struct exynos5_clock *,
 *				provides clock reg addresses
 */
static void exynos5_set_apll(enum cpufreq_level new_freq_level,
					struct exynos5_clock *clk)
{
	unsigned int val, pdiv;

	/* Set APLL Lock time */
	pdiv = exynos5250_data_table[new_freq_level].apll_pdiv;
	writel((pdiv * 250), &clk->apll_lock);

	/* Change PLL PMS values */
	val = readl(&clk->apll_con0);
	val &= APLL_PMS_MASK;
	val |= MDIV_MASK(exynos5250_data_table[new_freq_level].apll_mdiv) |
		PDIV_MASK(exynos5250_data_table[new_freq_level].apll_pdiv) |
		SDIV_MASK(exynos5250_data_table[new_freq_level].apll_sdiv);
	writel(val, &clk->apll_con0);

	/* Wait for APLL lock time to complete */
	while (!(readl(&clk->apll_con0) & CON0_LOCK_BIT_MASK))
		;
}

/*
 * Switch ARM power corresponding to new frequency level
 *
 * @param new_volt_index	enum cpufreq_level, provides new voltage
 *				corresponing to new frequency level
 * @return			int value, 0 for success
 */
static int exynos5_set_voltage(enum cpufreq_level new_volt_index)
{
	u32 new_volt;

	new_volt =  exynos5250_data_table[new_volt_index].volt;

	return pmic_set_voltage(new_volt);
}

/*
 * Switch exybos5 frequency to new level
 *
 * @param new_freq_level	enum cpufreq_level, provides new frequency
 * @return			int value, 0 for success
 */
static int exynos5_set_frequency(enum cpufreq_level new_freq_level)
{
	int error = 0;
	struct exynos5_clock *clk =
		(struct exynos5_clock *)samsung_get_base_clock();

	if (old_freq_level < new_freq_level) {
		/* Alter voltage corresponding to new frequency */
		error = exynos5_set_voltage(new_freq_level);

		/* Change the system clock divider values */
		exynos5_set_clkdiv(new_freq_level, clk);

		/* Change the apll m,p,s value */
		exynos5_set_apll(new_freq_level, clk);
	} else if (old_freq_level > new_freq_level) {
		/* Change the apll m,p,s value */
		exynos5_set_apll(new_freq_level, clk);

		/* Change the system clock divider values */
		exynos5_set_clkdiv(new_freq_level, clk);

		/* Alter voltage corresponding to new frequency */
		error = exynos5_set_voltage(new_freq_level);
	}

	old_freq_level = new_freq_level;
	debug("ARM Frequency changed\n");

	return error;
}

/*
 * Switch ARM frequency to new level
 *
 * @param new_freq_level	enum cpufreq_level, states new frequency
 * @return			int value, 0 for success
 */
int exynos_set_frequency(enum cpufreq_level new_freq_level)
{
	if (cpu_is_exynos5()) {
		return exynos5_set_frequency(new_freq_level);
	} else {
		debug("CPUFREQ: Frequency scaling not allowed for this CPU\n");
		return -1;
	}
}

/*
 * Initialize frequency scaling for exynos5
 */
static void exynos5_cpufreq_init(void)
{
	unsigned int val;
	struct exynos5_clock *clk =
		(struct exynos5_clock *)samsung_get_base_clock();

	/* Save default divider ratios for CPU0 */
	val = readl(&clk->div_cpu0);
	val &= DIV_CPU0_RSVD;
	exynos5250_clkdiv.cpu0 = val;

	/* Save default divider ratios for CPU1 */
	val = readl(&clk->div_cpu1);
	val &= DIV_CPU1_RSVD;
	exynos5250_clkdiv.cpu1 = val;

	/* Calculate default ARM frequence level */
	old_freq_level = (get_pll_clk(APLL) / 100000000) - 2;
	debug("Current ARM frequency is %u\n", val);
}

/*
 * Initialize ARM frequency scaling
 *
 * @return	int value, 0 for success
 */
int exynos_cpufreq_init(void)
{
	if (cpu_is_exynos5()) {
		exynos5_cpufreq_init();
		return 0;
	} else {
		debug("CPUFREQ: Could not init for this CPU\n");
		return -1;
	}
}
