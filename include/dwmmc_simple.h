/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

#ifndef __DWMMC_SIMPLE_H
#define __DWMMC_SIMPLE_H

/*
 * Simplified DWMMC driver.
 *
 * This driver is a simplified version of the standard DWMMC driver.
 * It is intended to be used in scenarios where it is not possible
 * to include the entire MMC framework and DWMMC driver, such as SPL.
 *
 * The standard dwmci_host and mmc_cmd structs are used, with
 * dwmci_simple_init initialing the dwmci_host struct.
 *
 * This driver only supports sending commands and receiving responses
 * from the MMC, not sending or receiving data blocks.
 *
 * Since this driver is designed to communicate with the boot MMC, which
 * on exynos is always MMC0, the MMC index is hard-coded, but can be changed
 * below in DWMMC_SIMPLE_INDEX.
 */

/* The exynos chips only support booting from MMC0, so only support one MMC */
#define DWMMC_SIMPLE_INDEX		0
#define DWMMC_SIMPLE_PERIPH_ID		(PERIPH_ID_SDMMC0 + DWMMC_SIMPLE_INDEX)
#define DWMMC_SIMPLE_RCA		0
#define DWMMC_SIMPLE_FREQ		400000		/* 400kHz bus clock */
#define DWMMC_SIMPLE_PINMUX_FLAGS	PINMUX_FLAG_8BIT_MODE
#define DWMMC_SIMPLE_VOLTAGES	\
	(MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195)

/**
 * Initialize dwmci host peripheral
 *
 * Initializes dwmci host peripheral specified by DWMMC_SIMPLE_INDEX.
 *
 * Once the peripheral has been initialized, the connect MMC can be started.
 *
 * @param host	uninitialized dwmci_host struct. This will be initialized
 *		with the peripheral, and can be passed to other functions.
 * @return 0 on success, -ve on error
 */
int dwmci_simple_init(struct dwmci_host *host);

/**
 * Send command to MMC on host
 *
 * Send a command and receive a response from the MMC connected to host.
 *
 * Commands which require a data transfer are not supported, however the
 * command response will be written to cmd.
 *
 * @param host	initialized host to send command over
 * @param cmd	command to send to MMC
 * @return 0 on success, -ve on error
 */
int dwmci_simple_send_cmd(struct dwmci_host *host, struct mmc_cmd *cmd);

/**
 * Startup MMC on dwmci to transfer mode
 *
 * Reset MMC and initialize it to transfer mode, at which point standard
 * commands can be sent.
 *
 * @param host	initialized dwmci host peripheral
 * @return 0 on success, -ve on error
 */
int dwmci_simple_startup(struct dwmci_host *host);
#endif	/* __DWMMC_SIMPLE_H */
