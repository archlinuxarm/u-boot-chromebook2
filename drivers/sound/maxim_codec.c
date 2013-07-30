/*
 * maxim_codec.c -- MAXIM CODEC Common driver
 *
 * Copyright 2011 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <asm/arch/clk.h>
#include <asm/arch/cpu.h>
#include <asm/arch/power.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <common.h>
#include <div64.h>
#include <fdtdec.h>
#include <i2c.h>
#include <sound.h>
#include "i2s.h"
#include "maxim_codec.h"
#include "max98095.h"

struct sound_codec_info g_codec_info;
struct maxim_codec_priv g_maxim_codec_info;
static unsigned int g_maxim_codec_i2c_dev_addr;

/*
 * Writes value to a device register through i2c
 *
 * @param reg	reg number to be write
 * @param data	data to be writen to the above registor
 *
 * @return	int value 1 for change, 0 for no change or negative error code.
 */
int maxim_codec_i2c_write(unsigned int reg, unsigned char data)
{
	debug("%s: Write Addr : 0x%02X, Data :  0x%02X\n",
	      __func__, reg, data);

	return i2c_write(g_maxim_codec_i2c_dev_addr, reg, 1, &data, 1);
}

/*
 * Read a value from a device register through i2c
 *
 * @param reg	reg number to be read
 * @param data	address of read data to be stored
 *
 * @return	int value 0 for success, -1 in case of error.
 */
unsigned int maxim_codec_i2c_read(unsigned int reg, unsigned char *data)
{
	int ret;

	ret = i2c_read(g_maxim_codec_i2c_dev_addr, reg, 1, data, 1);
	if (ret != 0) {
		debug("%s: Error while reading register %#04x\n",
		      __func__, reg);
		return -1;
	}

	return 0;
}

/*
 * update device register bits through i2c
 *
 * @param reg	codec register
 * @param mask	register mask
 * @param value	new value
 *
 * @return int value 0 for success, non-zero error code.
 */
int maxim_codec_update_bits(unsigned int reg, unsigned char mask,
				unsigned char value)
{
	int change, ret = 0;
	unsigned char old, new;

	if (maxim_codec_i2c_read(reg, &old) != 0)
		return -1;
	new = (old & ~mask) | (value & mask);
	change  = (old != new) ? 1 : 0;
	if (change)
		ret = maxim_codec_i2c_write(reg, new);
	if (ret < 0)
		return ret;

	return change;
}

static int maxim_codec_do_init(struct sound_codec_info *pcodec_info,
			int sampling_rate, int mclk_freq,
			int bits_per_sample)
{
	int ret = 0;

	/* Enable codec clock */
	set_xclkout();

	/* shift the device address by 1 for 7 bit addressing */
	g_maxim_codec_i2c_dev_addr = pcodec_info->i2c_dev_addr >> 1;

	if (pcodec_info->codec_type == CODEC_MAX_98095) {
		g_maxim_codec_info.devtype = MAX98095;
		ret = max98095_device_init(&g_maxim_codec_info);
	} else {
		printf("%s: Codec id [%d] not defined\n", __func__,
		       pcodec_info->codec_type);
		return -1;
	}

	if (ret < 0) {
		debug("%s: maxim codec chip init failed\n", __func__);
		return ret;
	}

	if (pcodec_info->codec_type == CODEC_MAX_98095) {
		ret = max98095_set_sysclk(&g_maxim_codec_info, mclk_freq);
		if (ret < 0) {
			debug("%s: max98095 codec set sys clock failed\n",
			      __func__);
			return ret;
		}
		ret = max98095_hw_params(&g_maxim_codec_info, sampling_rate,
				bits_per_sample);
		if (ret == 0) {
			ret = max98095_set_fmt(&g_maxim_codec_info,
						SND_SOC_DAIFMT_I2S |
						SND_SOC_DAIFMT_NB_NF |
						SND_SOC_DAIFMT_CBS_CFS);
		}
	}

	return ret;
}

static int get_maxim_codec_values(struct sound_codec_info *pcodec_info,
				const void *blob, const char *codectype)
{
	int error = 0;
#ifdef CONFIG_OF_CONTROL
	enum fdt_compat_id compat;
	int node;
	int parent;

	/* Get the node from FDT for codec */
	node = fdtdec_next_compatible(blob, 0, COMPAT_MAXIM_98095_CODEC);
	if (node <= 0) {
		debug("EXYNOS_SOUND: No node for codec COMPAT_MAXIM_98095_CODEC in device tree\n");
		debug("node = %d\n", node);
		return -1;
	}

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}

	compat = fdtdec_lookup(blob, parent);

	switch (compat) {
	case COMPAT_SAMSUNG_S3C2440_I2C:
	case COMPAT_SAMSUNG_EXYNOS5_I2C:
		pcodec_info->i2c_bus = i2c_get_bus_num_fdt(parent);
		error |= pcodec_info->i2c_bus;
		debug("i2c bus = %d\n", pcodec_info->i2c_bus);
		pcodec_info->i2c_dev_addr = fdtdec_get_int(blob, node,
							"reg", 0);
		error |= pcodec_info->i2c_dev_addr;
		debug("i2c dev addr = %x\n", pcodec_info->i2c_dev_addr);
		break;
	default:
		debug("%s: Unknown compat id %d\n", __func__, compat);
		return -1;
	}
#else
	pcodec_info->i2c_bus = MAXIM_AUDIO_I2C_BUS;
	pcodec_info->i2c_dev_addr = MAXIM_AUDIO_I2C_REG;
	debug("i2c dev addr = %d\n", pcodec_info->i2c_dev_addr);
#endif
	if (!strcmp(codectype, "max98095"))
		pcodec_info->codec_type = CODEC_MAX_98095;

	if (error == -1) {
		debug("fail to get max98095 codec node properties\n");
		return -1;
	}

	return 0;
}

/* maxim Device Initialisation */
int maxim_codec_init(const void *blob, const char *codectype, int sampling_rate,
			int mclk_freq, int bits_per_sample)
{
	int ret;
	int old_bus = i2c_get_bus_num();
	struct sound_codec_info *pcodec_info = &g_codec_info;

	if (get_maxim_codec_values(pcodec_info, blob, codectype) < 0) {
		printf("FDT Codec values failed\n");
		 return -1;
	}

	i2c_set_bus_num(pcodec_info->i2c_bus);
	ret = maxim_codec_do_init(pcodec_info, sampling_rate, mclk_freq,
				bits_per_sample);
	i2c_set_bus_num(old_bus);

	return ret;
}
