/*
 * Copyright (C) 2012 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
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
#include <asm/io.h>
#include <asm/arch/power.h>

static void exynos4_mipi_phy_control(unsigned int dev_index,
					unsigned int enable)
{
	struct exynos4_power *pmu =
	    (struct exynos4_power *)samsung_get_base_power();
	unsigned int addr, cfg = 0;

	if (dev_index == 0)
		addr = (unsigned int)&pmu->mipi_phy0_control;
	else
		addr = (unsigned int)&pmu->mipi_phy1_control;


	cfg = readl(addr);
	if (enable)
		cfg |= (EXYNOS_MIPI_PHY_MRESETN | EXYNOS_MIPI_PHY_ENABLE);
	else
		cfg &= ~(EXYNOS_MIPI_PHY_MRESETN | EXYNOS_MIPI_PHY_ENABLE);

	writel(cfg, addr);
}

void set_mipi_phy_ctrl(unsigned int dev_index, unsigned int enable)
{
	if (cpu_is_exynos4())
		exynos4_mipi_phy_control(dev_index, enable);
}

void exynos5_set_usbhost_phy_ctrl(unsigned int enable)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	if (enable) {
		/* Enabling USBHOST_PHY */
		setbits_le32(&power->usbhost_phy_control,
				POWER_USB_HOST_PHY_CTRL_EN);
	} else {
		/* Disabling USBHOST_PHY */
		clrbits_le32(&power->usbhost_phy_control,
				POWER_USB_HOST_PHY_CTRL_EN);
	}
}

void set_usbhost_phy_ctrl(unsigned int enable)
{
	if (cpu_is_exynos5())
		exynos5_set_usbhost_phy_ctrl(enable);
}

static void exynos5_set_usbdrd_phy_ctrl(unsigned int enable, int dev_index)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	/*
	 * Assuming here that the DRD_PHY_CONTROL registers
	 * are contiguous, so that :
	 * addressof(DRD_PHY1_CONTROL) = addressof(DRD_PHY_CONTROL) + 0x4;
	 * which is the case with exynos5420.
	 * For exynos5250 this should work out of box, since dev_index will
	 * always be '0' in that case
	 */
	if (enable) {
		/* Enabling USBDRD_PHY */
		setbits_le32(&power->usbdrd_phy_control + dev_index,
				POWER_USB_DRD_PHY_CTRL_EN);
	} else {
		/* Disabling USBDRD_PHY */
		clrbits_le32(&power->usbdrd_phy_control + dev_index,
				POWER_USB_DRD_PHY_CTRL_EN);
	}
}

void set_usbdrd_phy_ctrl(unsigned int enable, int dev_index)
{
	if (cpu_is_exynos5())
		exynos5_set_usbdrd_phy_ctrl(enable, dev_index);
}

static void exynos5_dp_phy_control(unsigned int enable)
{
	unsigned int cfg;
	struct exynos5_power *power =
	    (struct exynos5_power *)samsung_get_base_power();

	cfg = readl(&power->dptx_phy_control);
	if (enable)
		cfg |= EXYNOS_DP_PHY_ENABLE;
	else
		cfg &= ~EXYNOS_DP_PHY_ENABLE;

	writel(cfg, &power->dptx_phy_control);
}

void set_dp_phy_ctrl(unsigned int enable)
{
	if (cpu_is_exynos5())
		exynos5_dp_phy_control(enable);
}

static void exynos5_set_ps_hold_ctrl(void)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	/* Set PS-Hold high */
	setbits_le32(&power->ps_hold_control,
			EXYNOS_PS_HOLD_CONTROL_DATA_HIGH);
}

/*
 * Set ps_hold data driving value high
 * This enables the machine to stay powered on
 * after the initial power-on condition goes away
 * (e.g. power button).
 */
void set_ps_hold_ctrl(void)
{
	if (cpu_is_exynos5())
		exynos5_set_ps_hold_ctrl();
}

void exynos5_power_shutdown(void)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	clrbits_le32(&power->ps_hold_control, EXYNOS_PS_HOLD_CONTROL_DATA_HIGH);
}

void exynos5_power_reset(void)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	/* Clear inform1 so there's no change we think we've got a wake reset */
	power->inform1 = 0;

	setbits_le32(&power->swreset, 1);
}

static void exynos5_set_xclkout(void)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	/* use xxti for xclk out */
	clrsetbits_le32(&power->pmu_debug, PMU_DEBUG_CLKOUT_SEL_MASK,
				PMU_DEBUG_XXTI);
}

void set_xclkout(void)
{
	if (cpu_is_exynos5())
		exynos5_set_xclkout();
}

/* Enables hardware tripping to power off the system when TMU fails */
void set_hw_thermal_trip(void)
{
	if (cpu_is_exynos5()) {
		struct exynos5_power *power =
			(struct exynos5_power *)samsung_get_base_power();

		/* PS_HOLD_CONTROL register ENABLE_HW_TRIP bit*/
		setbits_le32(&power->ps_hold_control, POWER_ENABLE_HW_TRIP);
	}
}

void power_shutdown(void)
{
	if (cpu_is_exynos5())
		exynos5_power_shutdown();
}

void power_reset(void)
{
	if (cpu_is_exynos5())
		exynos5_power_reset();
}

static uint32_t exynos5_get_reset_status(void)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();

	return power->inform1;
}

static uint32_t exynos4_get_reset_status(void)
{
	struct exynos4_power *power =
		(struct exynos4_power *)samsung_get_base_power();

	return power->inform1;
}

uint32_t get_reset_status(void)
{
	if (cpu_is_exynos5())
		return exynos5_get_reset_status();
	else
		return  exynos4_get_reset_status();
}

static void exynos5_power_exit_wakeup(void)
{
	struct exynos5_power *power =
		(struct exynos5_power *)samsung_get_base_power();
	typedef void (*resume_func)(void);

	((resume_func)power->inform0)();
}

static void exynos4_power_exit_wakeup(void)
{
	struct exynos4_power *power =
		(struct exynos4_power *)samsung_get_base_power();
	typedef void (*resume_func)(void);

	((resume_func)power->inform0)();
}

void power_exit_wakeup(void)
{
	if (cpu_is_exynos5())
		exynos5_power_exit_wakeup();
	else
		exynos4_power_exit_wakeup();
}

/*
 * HACK out power to the A7s so they don't come up and mess us up.
 *
 * TODO: Belongs in some better place, needs new structure def, etc.
 */
void hack_off_a7s(void)
{
	if (proid_is_exynos5420()) {
		int i;
		void *kfc_base = ((void*)samsung_get_base_power()) + 0x2200;

		for (i = 0; i < 4; i++) {
			void *kfc_coreN_base = kfc_base + (i * 0x80);
			u32 *kfc_coreN_configuration = kfc_coreN_base + 0;

			writel(0, kfc_coreN_configuration);
		}
	}
}

int power_init(void)
{
	/* Assert PS_HOLD to indicate that we're up and running */
	set_ps_hold_ctrl();

	hack_off_a7s();

	return 0;
}

bool power_watchdog_fired(void)
{
	if (cpu_is_exynos5()) {
		struct exynos5_power *power =
			(struct exynos5_power *)samsung_get_base_power();
		unsigned int val;

		val = readl(&power->rst_stat);
		if (proid_is_exynos5420())
			val &= EXYNOS5420_RST_STAT_SYS_WDTRESET;
		else if (proid_is_exynos5250())
			val &= EXYNOS5250_RST_STAT_SYS_WDTRESET;
		else
			val = 0;

		return val != 0;
	}

	return false;
}
