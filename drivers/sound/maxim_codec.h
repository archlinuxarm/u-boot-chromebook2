/*
 * maxim_codec.h -- MAXIM codec common interface file
 *
 * Copyright (C) 2013 Samsung Electronics
 * D Krishna Mohan <krishna.md@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAXIM_COMMON_H__
#define __MAXIM_COMMON_H__

enum maxim_codec_type {
	MAX98095,
};

struct maxim_codec_priv {
	enum maxim_codec_type devtype;
	unsigned int sysclk;
	unsigned int rate;
	unsigned int fmt;
};

#define MAXIM_AUDIO_I2C_BUS		7
#define MAXIM_AUDIO_I2C_REG_98095	0x22

#define MAXIM_AUDIO_I2C_REG		MAXIM_AUDIO_I2C_REG_98095

int maxim_codec_i2c_write(unsigned int reg, unsigned char data);
unsigned int maxim_codec_i2c_read(unsigned int reg, unsigned char *data);
int maxim_codec_update_bits(unsigned int reg, unsigned char mask,
				unsigned char value);
int maxim_codec_init(const void *blob, const char *codectype, int sampling_rate,
			int mclk_freq, int bits_per_sample);

#endif /* __MAXIM_COMMON_H__ */
