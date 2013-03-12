/*
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
#include <asm/arch/clk.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dmc.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/periph.h>
#include <asm/arch/power.h>
#include <asm/arch/spi.h>
#include <asm/arch/spl.h>
#include "clock_init.h"
#include "setup.h"

DECLARE_GLOBAL_DATA_PTR;

/* Index into irom ptr table */
enum index {
	MMC_INDEX,
	EMMC44_INDEX,
	EMMC44_END_INDEX,
	SPI_INDEX,
	USB_INDEX,
};

/* IROM Function Pointers Table */
u32 irom_ptr_table[] = {
	[MMC_INDEX] = 0x02020030,	/* iROM Function Pointer-SDMMC boot */
	[EMMC44_INDEX] = 0x02020044,	/* iROM Function Pointer-EMMC4.4 boot*/
	[EMMC44_END_INDEX] = 0x02020048,/* iROM Function Pointer
						-EMMC4.4 end boot operation */
	[SPI_INDEX] = 0x02020058,	/* iROM Function Pointer-SPI boot */
	[USB_INDEX] = 0x02020070,	/* iROM Function Pointer-USB boot*/
	};

void *get_irom_func(int index)
{
	return (void *)*(u32 *)irom_ptr_table[index];
}

/*
 * Set/clear program flow prediction and return the previous state.
 */
static int config_branch_prediction(int set_cr_z)
{
	unsigned int cr;

	/* System Control Register: 11th bit Z Branch prediction enable */
	cr = get_cr();
	set_cr(set_cr_z ? cr | CR_Z : cr & ~CR_Z);

	return cr & CR_Z;
}

#ifdef CONFIG_EXYNOS_FAST_SPI_BOOT
static void spi_rx_tx(struct exynos_spi *regs, int todo,
			void *dinp, void const *doutp, int i)
{
	uint *rxp = (uint *)(dinp + (i * (32 * 1024)));
	int rx_lvl, tx_lvl;
	uint out_bytes, in_bytes;

	in_bytes = todo;
	out_bytes = todo;
	setbits_le32(&regs->ch_cfg, SPI_CH_RST);
	clrbits_le32(&regs->ch_cfg, SPI_CH_RST);
	writel(((todo * 8) / 32) | SPI_PACKET_CNT_EN, &regs->pkt_cnt);

	while (in_bytes) {
		uint32_t spi_sts;
		int temp;

		spi_sts = readl(&regs->spi_sts);
		rx_lvl = ((spi_sts >> 15) & 0x7f);
		tx_lvl = ((spi_sts >> 6) & 0x7f);
		while (tx_lvl < 32 && out_bytes) {
			temp = 0xffffffff;
			writel(temp, &regs->tx_data);
			out_bytes -= 4;
			tx_lvl += 4;
		}
		while (rx_lvl >= 4 && in_bytes) {
			temp = readl(&regs->rx_data);
			if (rxp)
				*rxp++ = temp;
			in_bytes -= 4;
			rx_lvl -= 4;
		}
	}
}

/**
 * Copy uboot from spi flash to RAM
 *
 * @parma uboot_size	size of u-boot to copy
 * @param uboot_addr	address of u-boot to copy
 */
static void exynos_spi_copy(unsigned int uboot_size, unsigned int uboot_addr)
{
	int upto, todo;
	int i;
	struct exynos_spi *regs = (struct exynos_spi *)CONFIG_ENV_SPI_BASE;

	set_spi_clk(PERIPH_ID_SPI1, 50000000); /* set spi clock to 50Mhz */
	/* set the spi1 GPIO */
	exynos_pinmux_config(PERIPH_ID_SPI1, PINMUX_FLAG_NONE);

	/* set pktcnt and enable it */
	writel(4 | SPI_PACKET_CNT_EN, &regs->pkt_cnt);
	/* set FB_CLK_SEL */
	writel(SPI_FB_DELAY_180, &regs->fb_clk);
	/* set CH_WIDTH and BUS_WIDTH as word */
	setbits_le32(&regs->mode_cfg, SPI_MODE_CH_WIDTH_WORD |
					SPI_MODE_BUS_WIDTH_WORD);
	clrbits_le32(&regs->ch_cfg, SPI_CH_CPOL_L); /* CPOL: active high */

	/* clear rx and tx channel if set priveously */
	clrbits_le32(&regs->ch_cfg, SPI_RX_CH_ON | SPI_TX_CH_ON);

	typedef u32 (*spi_copy_func_t)(u32 offset, u32 nblock, u32 dst);
	setbits_le32(&regs->swap_cfg, SPI_RX_SWAP_EN |
		SPI_RX_BYTE_SWAP |
		SPI_RX_HWORD_SWAP);

	/* do a soft reset */
	setbits_le32(&regs->ch_cfg, SPI_CH_RST);
	clrbits_le32(&regs->ch_cfg, SPI_CH_RST);

	/* now set rx and tx channel ON */
	setbits_le32(&regs->ch_cfg, SPI_RX_CH_ON | SPI_TX_CH_ON | SPI_CH_HS_EN);
	clrbits_le32(&regs->cs_reg, SPI_SLAVE_SIG_INACT); /* CS low */

	/* Send read instruction (0x3h) followed by a 24 bit addr */
	writel((SF_READ_DATA_CMD << 24) | SPI_FLASH_UBOOT_POS, &regs->tx_data);

	/* waiting for TX done */
	while (!(readl(&regs->spi_sts) & SPI_ST_TX_DONE))
		;

	for (upto = 0, i = 0; upto < uboot_size; upto += todo, i++) {
		todo = min(uboot_size - upto, (1 << 15));
		spi_rx_tx(regs, todo, (void *)(uboot_addr),
			  (void *)(SPI_FLASH_UBOOT_POS), i);
	}

	setbits_le32(&regs->cs_reg, SPI_SLAVE_SIG_INACT);/* make the CS high */

	/*
	 * Let put controller mode to BYTE as
	 * SPI driver does not support WORD mode yet
	 */
	clrbits_le32(&regs->mode_cfg, SPI_MODE_CH_WIDTH_WORD |
					SPI_MODE_BUS_WIDTH_WORD);
	writel(0, &regs->swap_cfg);

	/*
	 * Flush spi tx, rx fifos and reset the SPI controller
	 * and clear rx/tx channel
	 */
	clrsetbits_le32(&regs->ch_cfg, SPI_CH_HS_EN, SPI_CH_RST);
	clrbits_le32(&regs->ch_cfg, SPI_CH_RST);
	clrbits_le32(&regs->ch_cfg, SPI_TX_CH_ON | SPI_RX_CH_ON);
}
#endif /* CONFIG_EXYNOS_FAST_SPI_BOOT */

/*
* Copy U-boot from mmc to RAM:
* COPY_BL2_FNPTR_ADDR: Address in iRAM, which Contains
* Pointer to API (Data transfer from mmc to ram)
*/
void copy_uboot_to_ram(void)
{
	int is_cr_z_set;
	unsigned int sec_boot_check;
	enum boot_mode bootmode = BOOT_MODE_OM;
#ifndef CONFIG_EXYNOS_FAST_SPI_BOOT
	u32 (*spi_copy)(u32 offset, u32 nblock, u32 dst);
#endif
	struct spl_machine_param *param = spl_get_machine_params();
	u32 (*copy_bl2)(u32 offset, u32 nblock, u32 dst);
	u32 (*copy_bl2_from_emmc)(u32 nblock, u32 dst);
	void (*end_bootop_from_emmc)(void);
	u32 (*usb_copy)(void);

	/* Read iRAM location to check for secondary USB boot mode */
	sec_boot_check = readl(EXYNOS_IRAM_SECONDARY_BASE);
	if (sec_boot_check == EXYNOS_USB_SECONDARY_BOOT)
		bootmode = BOOT_MODE_USB;

	if (bootmode == BOOT_MODE_OM)
		bootmode = readl(EXYNOS5_POWER_BASE) & OM_STAT;

	switch (bootmode) {
	case BOOT_MODE_SERIAL:
#ifdef CONFIG_EXYNOS_FAST_SPI_BOOT
		/* let us our own function to copy u-boot from SF */
		exynos_spi_copy(param->uboot_size, CONFIG_SYS_TEXT_BASE);
#else
		spi_copy = get_irom_func(SPI_INDEX);
		spi_copy(SPI_FLASH_UBOOT_POS, param->uboot_size,
			 CONFIG_SYS_TEXT_BASE);
#endif
		break;
	case BOOT_MODE_MMC:
		copy_bl2 = get_irom_func(MMC_INDEX);
		copy_bl2(BL2_START_OFFSET, BL2_SIZE_BLOC_COUNT,
			 CONFIG_SYS_TEXT_BASE);
		break;
	case BOOT_MODE_EMMC:
		/* Set the FSYS1 clock divisor value for EMMC boot */
		emmc_boot_clk_div_set();

		copy_bl2_from_emmc = get_irom_func(EMMC44_INDEX);
		end_bootop_from_emmc = get_irom_func(EMMC44_END_INDEX);

		copy_bl2_from_emmc(BL2_SIZE_BLOC_COUNT, CONFIG_SYS_TEXT_BASE);
		end_bootop_from_emmc();
		break;
	case BOOT_MODE_USB:
		/*
		 * iROM needs program flow prediction to be disabled
		 * before copy from USB device to RAM
		 */
		is_cr_z_set = config_branch_prediction(0);
		usb_copy = get_irom_func(USB_INDEX);
		usb_copy();
		config_branch_prediction(is_cr_z_set);
		break;
	default:
		break;
	}
}

/* Tell the loaded U-Boot that it was loaded from SPL */
static void exynos5_set_spl_marker(void)
{
	uint32_t *marker = (uint32_t *)CONFIG_SPL_MARKER;

	*marker = EXYNOS5_SPL_MARKER;
}

void memzero(void *s, size_t n)
{
	char *ptr = s;
	size_t i;

	for (i = 0; i < n; i++)
		*ptr++ = '\0';
}

/**
 * Set up the U-Boot global_data pointer
 *
 * This sets the address of the global data, and sets up basic values.
 *
 * @param gdp   Value to give to gd
 */
static void setup_global_data(gd_t *gdp)
{
	gd = gdp;
	memzero((void *)gd, sizeof(gd_t));
	gd->flags |= GD_FLG_RELOC;
	gd->baudrate = CONFIG_BAUDRATE;
	gd->have_console = 1;
}

void board_init_f(unsigned long bootflag)
{
	__attribute__((aligned(8))) gd_t local_gd;
	__attribute__((noreturn)) void (*uboot)(void);

	exynos5_set_spl_marker();
	setup_global_data(&local_gd);

	if (do_lowlevel_init())
		power_exit_wakeup();

	copy_uboot_to_ram();

	/* Jump to U-Boot image */
	uboot = (void *)CONFIG_SYS_TEXT_BASE;
	(*uboot)();
	/* Never returns Here */
}

/* Place Holders */
void board_init_r(gd_t *id, ulong dest_addr)
{
	/* Function attribute is no-return */
	/* This Function never executes */
	while (1)
		;
}
void save_boot_params(u32 r0, u32 r1, u32 r2, u32 r3) {}

void hang(void)
{
	for (;;)
		;
}
