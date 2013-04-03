/*
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
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

/* This code should work for both the S3C2400 and the S3C2410
 * as they seem to have the same I2C controller inside.
 * The different address mapping is handled by the s3c24xx.h files below.
 */

#include <common.h>
#include <fdtdec.h>
#if (defined CONFIG_EXYNOS4 || defined CONFIG_EXYNOS5)
#include <asm/arch/clk.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/clock.h>
#else
#include <asm/arch/s3c24x0_cpu.h>
#endif
#include <asm/io.h>
#include <i2c.h>
#include "s3c24x0_i2c.h"

#ifdef CONFIG_HARD_I2C

#define	I2C_WRITE	0
#define I2C_READ	1

#define I2C_OK		0
#define I2C_NOK		1
#define I2C_NACK	2
#define I2C_NOK_LA	3	/* Lost arbitration */
#define I2C_NOK_TOUT	4	/* time out */

/* HSI2C specific register description */

/* I2C_CTL Register bits */
#define HSI2C_FUNC_MODE_I2C		(1u << 0)
#define HSI2C_MASTER			(1u << 3)
#define HSI2C_RXCHON			(1u << 6)	/* Write/Send */
#define HSI2C_TXCHON			(1u << 7)	/* Read/Receive */
#define HSI2C_SW_RST			(1u << 31)

/* I2C_FIFO_CTL Register bits */
#define HSI2C_RXFIFO_EN			(1u << 0)
#define HSI2C_TXFIFO_EN			(1u << 1)
#define HSI2C_TXFIFO_TRIGGER_LEVEL	(0x20 << 16)
#define HSI2C_RXFIFO_TRIGGER_LEVEL	(0x20 << 4)

/* I2C_TRAILING_CTL Register bits */
#define HSI2C_TRAILING_COUNT		(0xff)

/* I2C_INT_EN Register bits */
#define HSI2C_INT_TX_ALMOSTEMPTY_EN	(1u << 0)
#define HSI2C_INT_RX_ALMOSTFULL_EN	(1u << 1)
#define HSI2C_INT_TRAILING_EN		(1u << 6)
#define HSI2C_INT_I2C_EN		(1u << 9)

/* I2C_CONF Register bits */
#define HSI2C_AUTO_MODE			(1u << 31)
#define HSI2C_10BIT_ADDR_MODE		(1u << 30)
#define HSI2C_HS_MODE			(1u << 29)

/* I2C_AUTO_CONF Register bits */
#define HSI2C_READ_WRITE		(1u << 16)
#define HSI2C_STOP_AFTER_TRANS		(1u << 17)
#define HSI2C_MASTER_RUN		(1u << 31)

/* I2C_TIMEOUT Register bits */
#define HSI2C_TIMEOUT_EN		(1u << 31)

/* I2C_TRANS_STATUS register bits */
#define HSI2C_MASTER_BUSY		(1u << 17)
#define HSI2C_SLAVE_BUSY		(1u << 16)
#define HSI2C_NO_DEV			(1u << 3)
#define HSI2C_NO_DEV_ACK		(1u << 2)
#define HSI2C_TRANS_ABORT		(1u << 1)
#define HSI2C_TRANS_DONE		(1u << 0)
#define HSI2C_TIMEOUT_AUTO		(0u << 0)

#define HSI2C_SLV_ADDR_MAS(x)		((x & 0x3ff) << 10)

/* Controller operating frequency, timing values for operation
 * are calculated against this frequency
 */
#define HSI2C_FS_TX_CLOCK		1000000

/* S3C I2C Controller bits */
#define I2CSTAT_BSY	0x20	/* Busy bit */
#define I2CSTAT_NACK	0x01	/* Nack bit */
#define I2CCON_ACKGEN	0x80	/* Acknowledge generation */
#define I2CCON_IRPND	0x10	/* Interrupt pending bit */
#define I2C_MODE_MT	0xC0	/* Master Transmit Mode */
#define I2C_MODE_MR	0x80	/* Master Receive Mode */
#define I2C_START_STOP	0x20	/* START / STOP */
#define I2C_TXRX_ENA	0x10	/* I2C Tx/Rx enable */

#define I2C_TIMEOUT 1		/* 1 second */

#define	HSI2C_TIMEOUT	100

/*
 * For SPL boot some boards need i2c before SDRAM is initialised so force
 * variables to live in SRAM
 */
static unsigned int g_current_bus __attribute__((section(".data")));
#ifdef CONFIG_OF_CONTROL
static int i2c_busses __attribute__((section(".data")));
static struct s3c24x0_i2c_bus i2c_bus[CONFIG_MAX_I2C_NUM]
			__attribute__((section(".data")));
#endif

static struct s3c24x0_i2c_bus *get_bus(unsigned int bus_idx)
{
	if (bus_idx < i2c_busses)
		return &i2c_bus[bus_idx];

	debug("Undefined bus: %d\n", bus_idx);
	return NULL;
}

#if !(defined CONFIG_EXYNOS4 || defined CONFIG_EXYNOS5)
static int GetI2CSDA(void)
{
	struct s3c24x0_gpio *gpio = s3c24x0_get_base_gpio();

#ifdef CONFIG_S3C2410
	return (readl(&gpio->gpedat) & 0x8000) >> 15;
#endif
#ifdef CONFIG_S3C2400
	return (readl(&gpio->pgdat) & 0x0020) >> 5;
#endif
}

static void SetI2CSCL(int x)
{
	struct s3c24x0_gpio *gpio = s3c24x0_get_base_gpio();

#ifdef CONFIG_S3C2410
	writel((readl(&gpio->gpedat) & ~0x4000) |
					(x & 1) << 14, &gpio->gpedat);
#endif
#ifdef CONFIG_S3C2400
	writel((readl(&gpio->pgdat) & ~0x0040) | (x & 1) << 6, &gpio->pgdat);
#endif
}
#endif

static int WaitForXfer(struct s3c24x0_i2c *i2c)
{
	int i;

	i = I2C_TIMEOUT * 10000;
	while (!(readl(&i2c->iiccon) & I2CCON_IRPND) && (i > 0)) {
		udelay(100);
		i--;
	}

	return (readl(&i2c->iiccon) & I2CCON_IRPND) ? I2C_OK : I2C_NOK_TOUT;
}

static int hsi2c_wait_for_irq(struct exynos5_hsi2c *i2c)
{
	int i = HSI2C_TIMEOUT * 10;
	int ret = I2C_NOK_TOUT;

	while (i > 0) {
		/* wait for a while and retry */
		udelay(100);
		if (readl(&i2c->usi_int_stat) &
			(HSI2C_INT_I2C_EN | HSI2C_INT_TX_ALMOSTEMPTY_EN)) {
			ret = I2C_OK;
			break;
		}
		i--;
	}

	return ret;
}

static void ReadWriteByte(struct s3c24x0_i2c *i2c)
{
	writel(readl(&i2c->iiccon) & ~I2CCON_IRPND, &i2c->iiccon);
}

static void hsi2c_clear_irqpd(struct exynos5_hsi2c *i2c)
{
	writel(readl(&i2c->usi_int_stat), &i2c->usi_int_stat);
}

static int hsi2c_isack(struct exynos5_hsi2c *i2c)
{
	return readl(&i2c->usi_trans_status) &
			(HSI2C_NO_DEV | HSI2C_NO_DEV_ACK);
}

static int IsACK(struct s3c24x0_i2c *i2c)
{
	return !(readl(&i2c->iicstat) & I2CSTAT_NACK);
}

static struct s3c24x0_i2c *get_base_i2c(void)
{
#ifdef CONFIG_EXYNOS4
	struct s3c24x0_i2c *i2c = (struct s3c24x0_i2c *)(samsung_get_base_i2c()
							+ (EXYNOS4_I2C_SPACING
							* g_current_bus));
	return i2c;
#elif defined CONFIG_EXYNOS5
	struct s3c24x0_i2c *i2c = (struct s3c24x0_i2c *)(samsung_get_base_i2c()
							+ (EXYNOS5_I2C_SPACING
							* g_current_bus));
	return i2c;
#else
	return s3c24x0_get_base_i2c();
#endif
}

static struct exynos5_hsi2c *get_base_hsi2c(void)
{
	struct exynos5_hsi2c *i2c = NULL;
	int bus = g_current_bus;

	if (proid_is_exynos5420() && bus < 8)
		i2c = (struct exynos5_hsi2c *)(EXYNOS5420_I2C_BASE +
					((bus) * EXYNOS5_I2C_SPACING));
	else if (proid_is_exynos5420() && (bus > 7 && bus < 11))
		i2c = (struct exynos5_hsi2c *)(EXYNOS5420_I2C_8910_BASE +
					((bus - 8) * EXYNOS5_I2C_SPACING));
	else
		printf("Wrong bus number\n");

	return i2c;
}

static void i2c_ch_init(struct s3c24x0_i2c *i2c, int speed, int slaveadd)
{
	ulong freq, pres = 16, div;
#if (defined CONFIG_EXYNOS4 || defined CONFIG_EXYNOS5)
	freq = get_i2c_clk();
#else
	freq = get_PCLK();
#endif
	/* calculate prescaler and divisor values */
	if ((freq / pres / (16 + 1)) > speed)
		/* set prescaler to 512 */
		pres = 512;

	div = 0;
	while ((freq / pres / (div + 1)) > speed)
		div++;

	/* set prescaler, divisor according to freq, also set ACKGEN, IRQ */
	writel((div & 0x0F) | 0xA0 | ((pres == 512) ? 0x40 : 0), &i2c->iiccon);

	/* init to SLAVE REVEIVE and set slaveaddr */
	writel(0, &i2c->iicstat);
	writel(slaveadd, &i2c->iicadd);
	/* program Master Transmit (and implicit STOP) */
	writel(I2C_MODE_MT | I2C_TXRX_ENA, &i2c->iicstat);
}

static void hsi2c_ch_init(struct exynos5_hsi2c *i2c, int speed)
{
	u32 i2c_timeout;
	ulong clkin = get_i2c_clk();
	u32 i2c_timing_s1;
	u32 i2c_timing_s2;
	u32 i2c_timing_s3;
	u32 i2c_timing_sla;
	unsigned int op_clk = HSI2C_FS_TX_CLOCK;
	unsigned int n_clkdiv;
	unsigned int t_start_su, t_start_hd;
	unsigned int t_stop_su;
	unsigned int t_data_su, t_data_hd;
	unsigned int t_scl_l, t_scl_h;
	unsigned int t_sr_release;
	unsigned int t_ftl_cycle;
	unsigned int i = 0, utemp0 = 0, utemp1 = 0, utemp2 = 0;

	/* FPCLK / FI2C =
	 * (CLK_DIV + 1) * (TSCLK_L + TSCLK_H + 2) + 8 + 2 * FLT_CYCLE
	 * uTemp0 = (CLK_DIV + 1) * (TSCLK_L + TSCLK_H + 2)
	 * uTemp1 = (TSCLK_L + TSCLK_H + 2)
	 * uTemp2 = TSCLK_L + TSCLK_H
	 */
	t_ftl_cycle = (readl(&i2c->usi_conf) >> 16) & 0x7;
	utemp0 = (clkin / op_clk) - 8 - 2 * t_ftl_cycle;

	/* CLK_DIV max is 256 */
	for (i = 0; i < 256; i++) {
		utemp1 = utemp0 / (i + 1);
		/* SCLK_L/H max is 256 / 2 */
		if (utemp1 < 128) {
			utemp2 = utemp1 - 2;
			break;
		}
	}

	n_clkdiv = i;
	t_scl_l = utemp2 / 2;
	t_scl_h = utemp2 / 2;
	t_start_su = t_scl_l;
	t_start_hd = t_scl_l;
	t_stop_su = t_scl_l;
	t_data_su = t_scl_l / 2;
	t_data_hd = t_scl_l / 2;
	t_sr_release = utemp2;

	i2c_timing_s1 = t_start_su << 24 | t_start_hd << 16 | t_stop_su << 8;
	i2c_timing_s2 = t_data_su << 24 | t_scl_l << 8 | t_scl_h << 0;
	i2c_timing_s3 = n_clkdiv << 16 | t_sr_release << 0;
	i2c_timing_sla = t_data_hd << 0;

	writel(HSI2C_TRAILING_COUNT, &i2c->usi_trailing_ctl);

	/* Clear to enable Timeout */
	i2c_timeout = readl(&i2c->usi_timeout);
	i2c_timeout &= ~HSI2C_TIMEOUT_EN;
	writel(i2c_timeout, &i2c->usi_timeout);

	writel(readl(&i2c->usi_conf) | HSI2C_AUTO_MODE, &i2c->usi_conf);

	/* Currently operating in Fast speed mode. */
	writel(i2c_timing_s1, &i2c->usi_timing_fs1);
	writel(i2c_timing_s2, &i2c->usi_timing_fs2);
	writel(i2c_timing_s3, &i2c->usi_timing_fs3);
	writel(i2c_timing_sla, &i2c->usi_timing_sla);
}

/*
 * MULTI BUS I2C support
 */

#ifdef CONFIG_I2C_MULTI_BUS
int i2c_set_bus_num(unsigned int bus)
{
	if ((bus < 0) || (bus >= CONFIG_MAX_I2C_NUM)) {
		debug("Bad bus: %d\n", bus);
		return -1;
	}

	g_current_bus = bus;
	if (proid_is_exynos5420() && (bus > 3)) {
		hsi2c_ch_init(get_base_hsi2c(),
						CONFIG_SYS_I2C_SPEED);
	} else
		i2c_ch_init(get_base_i2c(), CONFIG_SYS_I2C_SPEED,
						CONFIG_SYS_I2C_SLAVE);

	return 0;
}

unsigned int i2c_get_bus_num(void)
{
	return g_current_bus;
}
#endif

void i2c_init(int speed, int slaveadd)
{
	struct s3c24x0_i2c *i2c;
#if !(defined CONFIG_EXYNOS4 || defined CONFIG_EXYNOS5)
	struct s3c24x0_gpio *gpio = s3c24x0_get_base_gpio();
#endif
	int i;

	/* By default i2c channel 0 is the current bus */
	g_current_bus = 0;
	i2c = get_base_i2c();

	/* wait for some time to give previous transfer a chance to finish */
	i = I2C_TIMEOUT * 1000;
	while ((readl(&i2c->iicstat) & I2CSTAT_BSY) && (i > 0)) {
		udelay(1000);
		i--;
	}

#if !(defined CONFIG_EXYNOS4 || defined CONFIG_EXYNOS5)
	if ((readl(&i2c->iicstat) & I2CSTAT_BSY) || GetI2CSDA() == 0) {
#ifdef CONFIG_S3C2410
		ulong old_gpecon = readl(&gpio->gpecon);
#endif
#ifdef CONFIG_S3C2400
		ulong old_gpecon = readl(&gpio->pgcon);
#endif
		/* bus still busy probably by (most) previously interrupted
		   transfer */

#ifdef CONFIG_S3C2410
		/* set I2CSDA and I2CSCL (GPE15, GPE14) to GPIO */
		writel((readl(&gpio->gpecon) & ~0xF0000000) | 0x10000000,
		       &gpio->gpecon);
#endif
#ifdef CONFIG_S3C2400
		/* set I2CSDA and I2CSCL (PG5, PG6) to GPIO */
		writel((readl(&gpio->pgcon) & ~0x00003c00) | 0x00001000,
		       &gpio->pgcon);
#endif

		/* toggle I2CSCL until bus idle */
		SetI2CSCL(0);
		udelay(1000);
		i = 10;
		while ((i > 0) && (GetI2CSDA() != 1)) {
			SetI2CSCL(1);
			udelay(1000);
			SetI2CSCL(0);
			udelay(1000);
			i--;
		}
		SetI2CSCL(1);
		udelay(1000);

		/* restore pin functions */
#ifdef CONFIG_S3C2410
		writel(old_gpecon, &gpio->gpecon);
#endif
#ifdef CONFIG_S3C2400
		writel(old_gpecon, &gpio->pgcon);
#endif
	}
#endif /* #if !(defined CONFIG_EXYNOS4 || defined CONFIG_EXYNOS5) */
	i2c_ch_init(i2c, speed, slaveadd);
}

/*
 * Send a STOP event and wait for it to have completed
 *
 * @param mode	If it is a master transmitter or receiver
 * @return I2C_OK if the line became idle before timeout I2C_NOK_TOUT otherwise
 */
static int hsi2c_send_stop(struct exynos5_hsi2c *i2c, int result)
{
	int timeout;
	int ret = I2C_NOK_TOUT;

	/* Wait for the STOP to send and the bus to go idle */
	for (timeout = HSI2C_TIMEOUT; timeout > 0; timeout -= 5) {
		if (!(readl(&i2c->usi_trans_status) & HSI2C_MASTER_BUSY)) {
			ret = I2C_OK;
			goto out;
		}
		udelay(5);
	}
 out:
	/* Setting the STOP event to fire */
	writel(HSI2C_FUNC_MODE_I2C, &i2c->usi_ctl);
	writel(0x0, &i2c->usi_int_en);

	return (result == I2C_OK) ? ret : result;
}

static int hsi2c_write(unsigned char chip,
			unsigned char addr[],
			unsigned char alen,
			unsigned char data[],
			unsigned short len)
{
	struct exynos5_hsi2c *i2c = get_base_hsi2c();
	int i = 0, result = I2C_OK;
	u32 i2c_auto_conf;
	u32 stat;

	/* Check I2C bus idle */
	i = HSI2C_TIMEOUT * 20;
	while ((readl(&i2c->usi_trans_status) & HSI2C_MASTER_BUSY)
							&& (i > 0)) {
		udelay(50);
		i--;
	}

	stat = readl(&i2c->usi_trans_status);

	if (stat & HSI2C_MASTER_BUSY) {
		debug("%s: bus busy\n", __func__);
		return I2C_NOK_TOUT;
	}
	/* Disable TXFIFO and RXFIFO */
	writel(0, &i2c->usi_fifo_ctl);

	/* chip address */
	writel(HSI2C_SLV_ADDR_MAS(chip), &i2c->i2c_addr);

	/* Enable interrupts */
	writel((HSI2C_INT_I2C_EN | HSI2C_INT_TX_ALMOSTEMPTY_EN),
						&i2c->usi_int_en);

	/* usi_ctl enable i2c func, master write configure */
	writel((HSI2C_TXCHON | HSI2C_FUNC_MODE_I2C | HSI2C_MASTER),
							&i2c->usi_ctl);

	/* i2c_conf configure */
	writel(readl(&i2c->usi_conf) | HSI2C_AUTO_MODE, &i2c->usi_conf);

	/* auto_conf for write length and stop configure */
	i2c_auto_conf = ((len + alen) | HSI2C_STOP_AFTER_TRANS);
	i2c_auto_conf &= ~HSI2C_READ_WRITE;
	writel(i2c_auto_conf, &i2c->usi_auto_conf);

	/* Master run, start xfer */
	writel(readl(&i2c->usi_auto_conf) | HSI2C_MASTER_RUN,
						&i2c->usi_auto_conf);

	result = hsi2c_wait_for_irq(i2c);
	if ((result == I2C_OK) && hsi2c_isack(i2c)) {
		result = I2C_NACK;
		goto err;
	}

	for (i = 0; i < alen && (result == I2C_OK); i++) {
		writel(addr[i], &i2c->usi_txdata);
		result = hsi2c_wait_for_irq(i2c);
	}

	for (i = 0; i < len && (result == I2C_OK); i++) {
		writel(data[i], &i2c->usi_txdata);
		result = hsi2c_wait_for_irq(i2c);
	}

 err:
	hsi2c_clear_irqpd(i2c);
	return hsi2c_send_stop(i2c, result);
}

static int hsi2c_read(unsigned char chip,
			unsigned char addr[],
			unsigned char alen,
			unsigned char data[],
			unsigned short len,
			int check)
{
	struct exynos5_hsi2c *i2c = get_base_hsi2c();
	int i, result;
	u32 i2c_auto_conf;

	if (!check) {
		result =  hsi2c_write(chip, addr, alen, data, 0);
		if (result != I2C_OK) {
			debug("write failed Result = %d\n", result);
			return result;
		}
	}

	/* start read */
	/* Disable TXFIFO and RXFIFO */
	writel(0, &i2c->usi_fifo_ctl);

	/* chip address */
	writel(HSI2C_SLV_ADDR_MAS(chip), &i2c->i2c_addr);

	/* Enable interrupts */
	writel(HSI2C_INT_I2C_EN, &i2c->usi_int_en);

		/* i2c_conf configure */
	writel(readl(&i2c->usi_conf) | HSI2C_AUTO_MODE, &i2c->usi_conf);

	/* auto_conf, length and stop configure */
	i2c_auto_conf = (len | HSI2C_STOP_AFTER_TRANS | HSI2C_READ_WRITE);
	writel(i2c_auto_conf, &i2c->usi_auto_conf);

	/* usi_ctl enable i2c func, master WRITE configure */
	writel((HSI2C_RXCHON | HSI2C_FUNC_MODE_I2C | HSI2C_MASTER),
							&i2c->usi_ctl);

	/* Master run, start xfer */
	writel(readl(&i2c->usi_auto_conf) | HSI2C_MASTER_RUN,
						&i2c->usi_auto_conf);

	result = hsi2c_wait_for_irq(i2c);
	if ((result == I2C_OK) && hsi2c_isack(i2c)) {
		result = I2C_NACK;
		goto err;
	}

	for (i = 0; i < len && (result == I2C_OK); i++) {
		result = hsi2c_wait_for_irq(i2c);
		data[i] = readl(&i2c->usi_rxdata);
		udelay(100);
	}
 err:
	/* Stop and quit */
	hsi2c_clear_irqpd(i2c);
	return hsi2c_send_stop(i2c, result);
}

/*
 * cmd_type is 0 for write, 1 for read.
 *
 * addr_len can take any value from 0-255, it is only limited
 * by the char, we could make it larger if needed. If it is
 * 0 we skip the address write cycle.
 */
static int i2c_transfer(struct s3c24x0_i2c *i2c,
			unsigned char cmd_type,
			unsigned char chip,
			unsigned char addr[],
			unsigned char addr_len,
			unsigned char data[],
			unsigned short data_len)
{
	int i, result;

	if (data == 0 || data_len == 0) {
		debug("i2c_transfer: bad call\n");
		return I2C_NOK;
	}

	/* Check I2C bus idle */
	i = I2C_TIMEOUT * 1000;
	while ((readl(&i2c->iicstat) & I2CSTAT_BSY) && (i > 0)) {
		udelay(1000);
		i--;
	}

	if (readl(&i2c->iicstat) & I2CSTAT_BSY)
		return I2C_NOK_TOUT;

	writel(readl(&i2c->iiccon) | I2CCON_ACKGEN, &i2c->iiccon);
	result = I2C_OK;

	switch (cmd_type) {
	case I2C_WRITE:
		if (addr && addr_len) {
			writel(chip, &i2c->iicds);
			/* send START */
			writel(I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP,
			       &i2c->iicstat);
			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				result = WaitForXfer(i2c);
				writel(addr[i], &i2c->iicds);
				ReadWriteByte(i2c);
				i++;
			}
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				result = WaitForXfer(i2c);
				writel(data[i], &i2c->iicds);
				ReadWriteByte(i2c);
				i++;
			}
		} else {
			writel(chip, &i2c->iicds);
			/* send START */
			writel(I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP,
			       &i2c->iicstat);
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				result = WaitForXfer(i2c);
				writel(data[i], &i2c->iicds);
				ReadWriteByte(i2c);
				i++;
			}
		}

		if (result == I2C_OK)
			result = WaitForXfer(i2c);

		/* send STOP */
		writel(I2C_MODE_MT | I2C_TXRX_ENA, &i2c->iicstat);
		ReadWriteByte(i2c);
		break;

	case I2C_READ:
		if (addr && addr_len) {
			writel(chip, &i2c->iicds);
			/* send START */
			writel(I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP,
				&i2c->iicstat);
			result = WaitForXfer(i2c);
			if (IsACK(i2c)) {
				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
					writel(addr[i], &i2c->iicds);
					ReadWriteByte(i2c);
					result = WaitForXfer(i2c);
					i++;
				}

				writel(chip, &i2c->iicds);
				/* resend START */
				writel(I2C_MODE_MR | I2C_TXRX_ENA |
				       I2C_START_STOP, &i2c->iicstat);
			ReadWriteByte(i2c);
			result = WaitForXfer(i2c);
				i = 0;
				while ((i < data_len) && (result == I2C_OK)) {
					/* disable ACK for final READ */
					if (i == data_len - 1)
						writel(readl(&i2c->iiccon)
							& ~I2CCON_ACKGEN,
							&i2c->iiccon);
				ReadWriteByte(i2c);
				result = WaitForXfer(i2c);
					data[i] = readl(&i2c->iicds);
					i++;
				}
			} else {
				result = I2C_NACK;
			}

		} else {
			writel(chip, &i2c->iicds);
			/* send START */
			writel(I2C_MODE_MR | I2C_TXRX_ENA | I2C_START_STOP,
				&i2c->iicstat);
			result = WaitForXfer(i2c);

			if (IsACK(i2c)) {
				i = 0;
				while ((i < data_len) && (result == I2C_OK)) {
					/* disable ACK for final READ */
					if (i == data_len - 1)
						writel(readl(&i2c->iiccon) &
							~I2CCON_ACKGEN,
							&i2c->iiccon);
					ReadWriteByte(i2c);
					result = WaitForXfer(i2c);
					data[i] = readl(&i2c->iicds);
					i++;
				}
			} else {
				result = I2C_NACK;
			}
		}

		/* send STOP */
		writel(I2C_MODE_MR | I2C_TXRX_ENA, &i2c->iicstat);
		ReadWriteByte(i2c);
		break;

	default:
		debug("i2c_transfer: bad call\n");
		result = I2C_NOK;
		break;
	}

	return result;
}

int i2c_probe(uchar chip)
{
	struct s3c24x0_i2c_bus *i2c_bus;
	uchar buf[1];
	int ret;

	i2c_bus = get_bus(g_current_bus);
	if (!i2c_bus)
		return -1;
	buf[0] = 0;

	/*
	 * What is needed is to send the chip address and verify that the
	 * address was <ACK>ed (i.e. there was a chip at that address which
	 * drove the data line low).
	 */
	if (board_i2c_claim_bus(i2c_bus->node)) {
		debug("I2C cannot claim bus %d\n", i2c_bus->bus_num);
		return -1;
	}
	if (proid_is_exynos5420() && (g_current_bus > 3))
		ret = hsi2c_read(chip, 0, 1, buf, 1, 1);
	else
		ret = i2c_transfer
			(i2c_bus->regs, I2C_READ, chip << 1, 0, 0, buf, 1);

	board_i2c_release_bus(i2c_bus->node);

	return ret != I2C_OK;
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	struct s3c24x0_i2c_bus *i2c_bus;
	uchar xaddr[4];
	int ret;

	if (alen > 4) {
		debug("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}

#ifdef CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW
	/*
	 * EEPROM chips that implement "address overflow" are ones
	 * like Catalyst 24WC04/08/16 which has 9/10/11 bits of
	 * address and the extra bits end up in the "chip address"
	 * bit slots. This makes a 24WC08 (1Kbyte) chip look like
	 * four 256 byte chips.
	 *
	 * Note that we consider the length of the address field to
	 * still be one byte because the extra address bits are
	 * hidden in the chip address.
	 */
	if (alen > 0)
		chip |= ((addr >> (alen * 8)) &
			 CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW);
#endif
	i2c_bus = get_bus(g_current_bus);
	if (!i2c_bus)
		return -1;
	if (board_i2c_claim_bus(i2c_bus->node)) {
		debug("I2C cannot claim bus %d\n", i2c_bus->bus_num);
		return -1;
	}

	if (proid_is_exynos5420() && (g_current_bus > 3))
		ret = hsi2c_read(chip, &xaddr[4 - alen],
						alen, buffer, len, 0);
	else
		ret = i2c_transfer(i2c_bus->regs, I2C_READ, chip << 1,
				&xaddr[4 - alen], alen, buffer, len);

	board_i2c_release_bus(i2c_bus->node);
	if (ret != 0) {
		debug("I2c read: failed %d\n", ret);
		return 1;
	}
	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	struct s3c24x0_i2c_bus *i2c_bus;
	uchar xaddr[4];
	int ret;

	if (alen > 4) {
		debug("I2C write: addr len %d not supported\n", alen);
		return 1;
	}

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}
#ifdef CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW
	/*
	 * EEPROM chips that implement "address overflow" are ones
	 * like Catalyst 24WC04/08/16 which has 9/10/11 bits of
	 * address and the extra bits end up in the "chip address"
	 * bit slots. This makes a 24WC08 (1Kbyte) chip look like
	 * four 256 byte chips.
	 *
	 * Note that we consider the length of the address field to
	 * still be one byte because the extra address bits are
	 * hidden in the chip address.
	 */
	if (alen > 0)
		chip |= ((addr >> (alen * 8)) &
			 CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW);
#endif
	i2c_bus = get_bus(g_current_bus);
	if (!i2c_bus)
		return -1;
	if (board_i2c_claim_bus(i2c_bus->node)) {
		debug("I2C cannot claim bus %d\n", i2c_bus->bus_num);
		return -1;
	}

	if (proid_is_exynos5420() && (g_current_bus > 3))
		ret = hsi2c_write(chip, &xaddr[4 - alen],
					alen, buffer, len);
	else
		ret = i2c_transfer(i2c_bus->regs, I2C_WRITE, chip << 1,
				&xaddr[4 - alen], alen, buffer, len);

	board_i2c_release_bus(i2c_bus->node);

	return ret != 0;
}

void board_i2c_init(const void *blob)
{
	int i;
#ifdef CONFIG_OF_CONTROL
	int node_list[CONFIG_MAX_I2C_NUM];
	int count;

	count = fdtdec_find_aliases_for_id(blob, "i2c",
		COMPAT_SAMSUNG_S3C2440_I2C, node_list,
		CONFIG_MAX_I2C_NUM);

	for (i = 0; i < count; i++) {
		struct s3c24x0_i2c_bus *bus;
		int node = node_list[i];

		if (node <= 0)
			continue;
		bus = &i2c_bus[i];
		bus->regs = (struct s3c24x0_i2c *)
			fdtdec_get_addr(blob, node, "reg");
		bus->hsregs = (struct exynos5_hsi2c *)
				fdtdec_get_addr(blob, node, "reg");
		bus->id = pinmux_decode_periph_id(blob, node);
		bus->node = node;
		bus->bus_num = i2c_busses++;
		exynos_pinmux_config(bus->id, 0);
	}
#else
	for (i = 0; i < CONFIG_MAX_I2C_NUM; i++) {
		exynos_pinmux_config((PERIPH_ID_I2C0 + i),
				     PINMUX_FLAG_NONE);
	}
#endif
}

#ifdef CONFIG_OF_CONTROL
int i2c_get_bus_num_fdt(int node)
{
	int i;

	for (i = 0; i < i2c_busses; i++) {
		if (node == i2c_bus[i].node)
			return i;
	}

	debug("%s: Can't find any matched I2C bus\n", __func__);
	return -1;
}

int i2c_reset_port_fdt(const void *blob, int node)
{
	struct s3c24x0_i2c_bus *i2c;
	int bus;

	bus = i2c_get_bus_num_fdt(node);
	if (bus < 0) {
		debug("could not get bus for node %d\n", node);
		return -1;
	}

	i2c = get_bus(bus);
	if (!i2c) {
		debug("get_bus() failed for node node %d\n", node);
		return -1;
	}

	if (proid_is_exynos5420() && (bus > 3))
		hsi2c_ch_init(i2c->hsregs, CONFIG_SYS_I2C_SPEED);
	else
		i2c_ch_init(i2c->regs, CONFIG_SYS_I2C_SPEED,
						CONFIG_SYS_I2C_SLAVE);

	return 0;
}
#endif

#endif /* CONFIG_HARD_I2C */
