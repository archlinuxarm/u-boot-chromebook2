/*
 * Copyright (c) 2013 Google, Inc
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

#ifndef __ASM_ARCH_SOUND_H
#define __ASM_ARCH_SOUND_H

/**
 * pcat_enable_beep() - Start a beep tone at a given frequency
 *
 * @frequency: Frequency of beep in Hz
 */
void pcat_enable_beep(uint32_t frequency);

/**
 * pcat_disable_beep() - Stop beep tone
 */
void pcat_disable_beep(void);

#endif
