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

#ifndef _TEGRA114_FUSE_H_
#define _TEGRA114_FUSE_H_

/* FUSE registers */
struct fuse_regs {
	u32 reserved0[9];		/* 0x00 - 0x20: */
	u32 fuse_bypass;		/* 0x24: FUSE_FUSEBYPASS */
	u32 private_key_disable;	/* 0x28: FUSE_PRIVATEKEYDISABLE */
	u32 disable_reg_program;	/* 0x2C:  FUSE_DISABLEREGPROGRAM */
	u32 write_access_sw;		/* 0x30:  FUSE_WRITE_ACCESS_SW */
	u32 reserved01[51];		/* 0x34 - 0xFC: */
	u32 production_mode;		/* 0x100: FUSE_PRODUCTION_MODE */
	u32 reserved1[3];		/* 0x104 - 0x10c: */
	u32 sku_info;			/* 0x110 */
	u32 reserved2[13];		/* 0x114 - 0x144: */
	u32 fa;				/* 0x148: FUSE_FA */
	u32 reserved3[21];		/* 0x14C - 0x19C: */
	u32 security_mode;		/* 0x1A0 - FUSE_SECURITY_MODE */
};

#endif	/* _TEGRA114_FUSE_H_ */
