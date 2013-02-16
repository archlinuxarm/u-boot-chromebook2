/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* GPIO interface for Chrome OS verified boot */

#ifndef __VBOOT_FLAG_H__
#define __VBOOT_FLAG_H__

#include <fdtdec.h>

#define VBOOT_FLAGS							\
	VBF(VBOOT_FLAG_WRITE_PROTECT = 0, "vboot-flag-write-protect")	\
	VBF(VBOOT_FLAG_RECOVERY, "vboot-flag-recovery")			\
	VBF(VBOOT_FLAG_DEVELOPER, "vboot-flag-developer")		\
	VBF(VBOOT_FLAG_LID_OPEN, "vboot-flag-lid-open")			\
	VBF(VBOOT_FLAG_POWER_OFF, "vboot-flag-power-off")		\
	VBF(VBOOT_FLAG_EC_IN_RW, "vboot-flag-ec-in-rw")			\
	VBF(VBOOT_FLAG_OPROM_LOADED, "vboot-flag-oprom-loaded")

enum vboot_flag_id {
#define VBF(__e, __s) __e,
	VBOOT_FLAGS
#undef VBF
	VBOOT_FLAG_MAX_FLAGS
};

/* The struct to store the context for each flag */
struct vboot_flag_context {
	struct vboot_flag_driver *driver;	/* the driver for ths eflag */
	int config_node;	/* offset of the config node in fdt */
	int node;		/* offset of the vboot flag node in fdt */
	int initialized;	/* 1 if the driver is initialized; 0 if not */
	struct fdt_gpio_state gpio_state;	/* the gpio state of fdt */
	int gpio_valid_time;	/* time of delay when gpio value is valid */
	/* previous value of the flag (1 or 0), or -1 if not known */
	int prev_value;
};

/* GPIO details required in the crossystem data structure */
struct vboot_flag_details {
	int value;		/* the value of the flag, either 1 or 0 */
	int port;		/* the gpio port number */
	int active_high;	/* 1 if the port is active high; 0 active low */
	/* previous value of the flag (1 or 0), or -1 if not known */
	int prev_value;
};

/* API function pointers of the driver to get vboot flag info */
struct vboot_flag_driver {
	/* the fdt compatible type */
	enum fdt_compat_id type;
	/**
	 * The setup() function will be call in vboot_flag_init()
	 *
	 * @prarm id		ID of VBoot flag
	 * @param context	Pointer to the context which keeps internal data
	 * @return zero on success and non-zero on failures
	 */
	int (*setup)(enum vboot_flag_id id, struct vboot_flag_context *context);
	/**
	 * The fetch() function returns the details of the vboot flag
	 *
	 * @prarm id		ID of VBoot flag
	 * @param context	Pointer to the context which keeps internal data
	 @ @param details	Returns the details of the flag
	 * @return zero on success and non-zero on failures
	 */
	int (*fetch)(enum vboot_flag_id id, struct vboot_flag_context *context,
			struct vboot_flag_details *details);
};

/**
 * Get the node name of the vboot flag
 *
 * @prarm id		ID of VBoot flag
 * @return the node name string for the flag
 */
const char *vboot_flag_node_name(enum vboot_flag_id id);

/**
 * Return the type of the required vboot flag
 *
 * @param id		ID of VBoot flag
 * @return COMPAT_GOOGLE_*_FLAG on success and COMPAT_UNKNOWN on failures
 */
enum fdt_compat_id vboot_flag_type(enum vboot_flag_id id);

/**
 * Fetch the details of the required vboot flag
 *
 * @param id		ID of VBoot flag
 * @param details	Returns the GPIO details
 * @return zero on success and non-zero on failures
 */
int vboot_flag_fetch(enum vboot_flag_id id, struct vboot_flag_details *details);

/**
 * Dump the vboot flag details to screen
 *
 * @param id		ID of VBoot flag
 * @param details	GPIO details; if NULL, get one from vboot_flag_fetch()
 * @return zero on success and non-zero on failures
 */
int vboot_flag_dump(enum vboot_flag_id id, struct vboot_flag_details *details);

#endif /* __VBOOT_FLAG_H__ */
