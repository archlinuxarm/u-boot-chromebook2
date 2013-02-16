/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* Implementation of per-board power management function */

#include <cros/power_management.h>
#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/ibmpc.h>
#include <pci.h>

#define PM1_STS         0x00
#define   PWRBTN_STS    (1 << 8)
#define PM1_EN          0x02
#define PM1_CNT         0x04
#define   SLP_EN        (1 << 13)
#define   SLP_TYP       (7 << 10)
#define   SLP_TYP_S0    (0 << 10)
#define   SLP_TYP_S1    (1 << 10)
#define   SLP_TYP_S3    (5 << 10)
#define   SLP_TYP_S4    (6 << 10)
#define   SLP_TYP_S5    (7 << 10)
#define GPE0_EN         0x2c

#define RST_CNT         0xcf9
#define   SYS_RST       (1 << 1)
#define   RST_CPU       (1 << 2)

DECLARE_GLOBAL_DATA_PTR;

int is_processor_reset(void)
{
	/*
	 * This isn't actually whether or not this boot is the result of a
	 * cold boot, it's really whether u-boot was started from the ELF
	 * entry point or from the start of the image. It also isn't really
	 * being used to check if the processor was reset either, it's
	 * checking if this copy of u-boot is the RW or RO firmware. This is a
	 * good enough approximation, though, and causes the right behavior.
	 */
	return gd->flags & GD_FLG_COLD_BOOT;
}

/* Do a hard reset through the chipset's reset control register. This
 * register is available on all x86 systems (at least those built in
 * the last 10ys)
 *
 * This function never returns.
 */
void cold_reboot(void)
{
	printf("Rebooting...\n");

	outb(SYS_RST | RST_CPU, RST_CNT);
	for (;;)
		asm("hlt");
}

/* Power down the machine by using the power management sleep control
 * of the chipset. This will currently only work on Intel chipsets.
 * However, adapting it to new chipsets is fairly simple. You will
 * have to find the IO address of the power management register block
 * in your southbridge, and look up the appropriate SLP_TYP_S5 value
 * from your southbridge's data sheet.
 *
 * This function never returns.
 */
void power_off(void)
{
	u16 id, pmbase;
	u32 reg32;

	/* Make sure this is an Intel chipset with the
	 * LPC device hard coded at 0:1f.0
	 */
	pci_read_config_word(PCI_BDF(0, 0x1f, 0), 0x00, &id);
	if(id != 0x8086) {
		printf("Power off is not implemented for this chipset. "
		       "Halting the CPU.\n");
		for (;;)
			asm("hlt");
	}

	/* Find the base address of the powermanagement registers */
	pci_read_config_word(PCI_BDF(0, 0x1f, 0), 0x40, &pmbase);
	pmbase &= 0xfffe;

	/* Mask interrupts or system might stay in a coma
	 * (not executing code anymore, but not powered off either)
	 */
	asm("cli");

	/* Avoid any GPI waking the system from S5
	 * or the system might stay in a coma
	 */
	outl(0x00000000, pmbase + GPE0_EN);

	/* Clear Power Button Status */
	outw(PWRBTN_STS, pmbase + PM1_STS);

	/* PMBASE + 4, Bit 10-12, Sleeping Type,
	 * set to 111 -> S5, soft_off */

	reg32 = inl(pmbase + PM1_CNT);

	/* Set Sleeping Type to S5 (poweroff) */
	reg32 &= ~(SLP_EN | SLP_TYP);
	reg32 |= SLP_TYP_S5;
	outl(reg32, pmbase + PM1_CNT);

	/* Now set the Sleep Enable bit */
	reg32 |= SLP_EN;
	outl(reg32, pmbase + PM1_CNT);

	for (;;)
		asm("hlt");
}

int do_coldboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cold_reboot();
	return 0;
}

U_BOOT_CMD(coldboot, 1, 1, do_coldboot, "Initiate a cold reboot.", "");

int do_poweroff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	power_off();
	return 0;
}

U_BOOT_CMD(poweroff, 1, 1, do_poweroff, "Switch off power", "");
