/*
 * Copyright (c) 2010 - 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TEGRA114_WARM_BOOT_H_
#define _TEGRA114_WARM_BOOT_H_

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
	u32 length_in_secure;	/* length of the code header */
	u32 reserved[3];
	u32 modules[64];	/* RsaKeyModulus */
	union {
		struct hash hash;  /* hash of header+code, starts next field */
		u32 signature[64]; /* The RSA-PSS signature*/
	} hash_signature;
	struct hash random_aes_block;	/* a data block to aid security. */
	u32 length_secure;	/* length of the code header */
	u32 destination;	/* destination address to put the wb code */
	u32 entry_point;	/* execution address of the wb code */
	u32 code_length;	/* length of the code */
};

#endif	/* _TEGRA114_WARM_BOOT_H_ */
