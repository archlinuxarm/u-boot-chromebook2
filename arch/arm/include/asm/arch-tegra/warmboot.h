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

#ifndef _WARM_BOOT_H_
#define _WARM_BOOT_H_

#define STRAP_OPT_A_RAM_CODE_SHIFT	4
#define STRAP_OPT_A_RAM_CODE_MASK	(0xf << STRAP_OPT_A_RAM_CODE_SHIFT)

/* Defines the supported operating modes */
enum fuse_operating_mode {
	MODE_PRODUCTION = 3,
	MODE_UNDEFINED,
};

/* Defines the CMAC-AES-128 hash length in 32 bit words. (128 bits = 4 words) */
enum {
	HASH_LENGTH = 4
};

/* Defines the storage for a hash value (128 bits) */
struct hash {
	u32 hash[HASH_LENGTH];
};

/**
 * Save warmboot memory settings for a later resume
 *
 * @return 0 if ok, -1 on error
 */
int warmboot_save_sdram_params(void);

int warmboot_prepare_code(u32 seg_address, u32 seg_length);
int sign_data_block(u8 *source, u32 length, u8 *signature);
void wb_start(void);	/* Start of WB assembly code */
void wb_end(void);	/* End of WB assembly code */

#endif	/* _WARM_BOOT_H_ */
