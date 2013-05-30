/*
 * (C) Copyright 2002
 * Daniel Engström, Omicron Ceti AB, <daniel@omicron.se>
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
#include <asm/i8254.h>

#define TIMER2_VALUE 0x0a8e /* 440Hz */

#define PPC_PORTB       0x61
#define PORTB_BEEP_ENABLE 0x3
#define PIT_HZ 1193180L

int pcat_timer_init(void)
{
	/*
	 * initialize 2, used to drive the speaker
	 * (to start a beep: write 3 to port 0x61,
	 * to stop it again: write 0)
	 */
	outb(PIT_CMD_CTR2 | PIT_CMD_BOTH | PIT_CMD_MODE3,
			PIT_BASE + PIT_COMMAND);
	outb(TIMER2_VALUE & 0xff, PIT_BASE + PIT_T2);
	outb(TIMER2_VALUE >> 8, PIT_BASE + PIT_T2);

	return 0;
}

/* Timer 2 legacy PC beep functions */
void pcat_enable_beep(uint32_t frequency)
{
	uint32_t countdown;

	if (!frequency)
		return;

	countdown = PIT_HZ / frequency;

	outb(PIT_CMD_CTR2 | PIT_CMD_BOTH | PIT_CMD_MODE3,
	     PIT_BASE + PIT_COMMAND);
	outb(countdown & 0xff, PIT_BASE + PIT_T2);
	outb((countdown >> 8) & 0xff , PIT_BASE + PIT_T2);
	outb(inb(PPC_PORTB) | PORTB_BEEP_ENABLE, PPC_PORTB);
}

void pcat_disable_beep(void)
{
	outb(inb(PPC_PORTB) & !PORTB_BEEP_ENABLE, PPC_PORTB);
}
