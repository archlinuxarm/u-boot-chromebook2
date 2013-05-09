/*
 * (C) Copyright 2010 - 2013
 * NVIDIA Corporation <www.nvidia.com>
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

#ifndef _TEGRA20_WARM_BOOT_H_
#define _TEGRA20_WARM_BOOT_H_

#include <asm/arch-tegra/warmboot.h>

/*
 * Defines the code header information for the boot rom.
 *
 * The code immediately follows the code header.
 *
 * Note that the code header needs to be 16 bytes aligned to preserve
 * the alignment of relevant data for hash and decryption computations without
 * requiring extra copies to temporary memory areas.
 */
struct wb_header {
	u32 length_insecure;	/* length of the code header */
	u32 reserved[3];
	struct hash hash;	/* hash of header+code, starts next field*/
	struct hash random_aes_block;	/* a data block to aid security. */
	u32 length_secure;	/* length of the code header */
	u32 destination;	/* destination address to put the wb code */
	u32 entry_point;	/* execution address of the wb code */
	u32 code_length;	/* length of the code */
};

/*
 * The warm boot code needs direct access to these registers since it runs in
 * SRAM and cannot call other U-Boot code.
 */
union osc_ctrl_reg {
	struct {
		u32 xoe:1;
		u32 xobp:1;
		u32 reserved0:2;
		u32 xofs:6;
		u32 reserved1:2;
		u32 xods:5;
		u32 reserved2:3;
		u32 oscfi_spare:8;
		u32 pll_ref_div:2;
		u32 osc_freq:2;
	};
	u32 word;
};

union pllx_base_reg {
	struct {
		u32 divm:5;
		u32 reserved0:3;
		u32 divn:10;
		u32 reserved1:2;
		u32 divp:3;
		u32 reserved2:4;
		u32 lock:1;
		u32 reserved3:1;
		u32 ref_dis:1;
		u32 enable:1;
		u32 bypass:1;
	};
	u32 word;
};

union pllx_misc_reg {
	struct {
		u32 vcocon:4;
		u32 lfcon:4;
		u32 cpcon:4;
		u32 lock_sel:6;
		u32 reserved0:1;
		u32 lock_enable:1;
		u32 reserved1:1;
		u32 dccon:1;
		u32 pts:2;
		u32 reserved2:6;
		u32 out1_div_byp:1;
		u32 out1_inv_clk:1;
	};
	u32 word;
};

/*
 * TODO: This register is not documented in the TRM yet. We could move this
 * into the EMC and give it a proper interface, but not while it is
 * undocumented.
 */
union scratch3_reg {
	struct {
		u32 pllx_base_divm:5;
		u32 pllx_base_divn:10;
		u32 pllx_base_divp:3;
		u32 pllx_misc_lfcon:4;
		u32 pllx_misc_cpcon:4;
	};
	u32 word;
};

#endif	/* _TEGRA20_WARM_BOOT_H_ */
