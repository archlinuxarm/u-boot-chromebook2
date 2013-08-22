/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __ARCH_EXYNOS_MCT_H__
#define __ARCH_EXYNOS_MCT_H__

#ifndef __ASSEMBLY__

/* A block of registers describing timer comparator. */
struct g_comp {
	unsigned g_comp_l;
	unsigned g_comp_u;
	unsigned g_comp_add_incr;
	unsigned pad;
};

/*
 * Only the global timer registers are included in the structure at this
 * time. See section 21.4 of the 5420 user manual rev 0.02.
 */
struct exynos5_mct {
	unsigned mct_cfg;
	char pad0[0xfc];
	unsigned g_cnt_l;
	unsigned g_cnt_u;
	char pad1[8];
	unsigned g_cnt_wstat;
	char pad2[0xec];
	struct g_comp g_comps[4];
	unsigned g_tcon;
	unsigned g_int_cstat;
	unsigned g_int_enb;
	unsigned g_wstat;
};

struct size_check {
	char dummy[(sizeof(struct exynos5_mct) == 0x250) ? 1 : -1];
};

#endif /* __ASSEMBLY__ */

#define MCT_G_TCON_TIMER_ENABLE (1 << 8)

#endif
