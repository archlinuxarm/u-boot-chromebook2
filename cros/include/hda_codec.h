/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 */

/* HDA codec interface for Chrome OS verified boot */

#ifndef CHROMEOS_HDA_CODEC_H_
#define CHROMEOS_HDA_CODEC_H_

/* Beep control */

/*
 * Start the beep using the HDA codec.
 *
 * @param frequency	The frequency of the beep in Hz
 */
void enable_beep_hda(uint32_t frequency);

/*
 * Stop the beep using the HDA codec
 */
void disable_beep_hda(void);

#endif /* CHROMEOS_PHDA_CODEC_H_ */
