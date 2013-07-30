/*
 * max98095.c -- MAX98095 ALSA SoC Audio driver
 *
 * Copyright 2011 Maxim Integrated Products
 *
 * Modified for uboot by R. Chandrasekar (rcsekar@samsung.com)
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

/* Index 0 is reserved. */
int rate_table[] = {0, 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000,
		88200, 96000};

/*
 * codec mclk clock divider coefficients based on sampling rate
 *
 * @param rate sampling rate
 * @param value address of indexvalue to be stored
 *
 * @return	0 for success or negative error code.
 */
static int rate_value(int rate, u8 *value)
{
	int i;

	for (i = 1; i < ARRAY_SIZE(rate_table); i++) {
		if (rate_table[i] >= rate) {
			*value = i;
			return 0;
		}
	}
	*value = 1;

	return -1;
}

/*
 * Sets hw params for max98095
 *
 * @param max98095	max98095 information pointer
 * @param rate		Sampling rate
 * @param bits_per_sample	Bits per sample
 *
 * @return -1 for error  and 0  Success.
 */
int max98095_hw_params(struct maxim_codec_priv *max98095,
		unsigned int rate, unsigned int bits_per_sample)
{
	u8 regval;
	int error;

	switch (bits_per_sample) {
	case 16:
		error = maxim_codec_update_bits(M98095_02A_DAI1_FORMAT,
			M98095_DAI_WS, 0);
		break;
	case 24:
		error = maxim_codec_update_bits(M98095_02A_DAI1_FORMAT,
			M98095_DAI_WS, M98095_DAI_WS);
		break;
	default:
		debug("%s: Illegal bits per sample %d.\n",
			__func__, bits_per_sample);
		return -1;
	}

	if (rate_value(rate, &regval)) {
		debug("%s: Failed to set sample rate to %d.\n",
			__func__, rate);
		return -1;
	}
	max98095->rate = rate;

	error |= maxim_codec_update_bits(M98095_027_DAI1_CLKMODE,
		M98095_CLKMODE_MASK, regval);

	/* Update sample rate mode */
	if (rate < 50000)
		error |= maxim_codec_update_bits(M98095_02E_DAI1_FILTERS,
			M98095_DAI_DHF, 0);
	else
		error |= maxim_codec_update_bits(M98095_02E_DAI1_FILTERS,
			M98095_DAI_DHF, M98095_DAI_DHF);

	if (error < 0) {
		debug("%s: Error setting hardware params.\n", __func__);
		return -1;
	}

	return 0;
}

/*
 * Configures Audio interface system clock for the given frequency
 *
 * @param max98095	max98095 information
 * @param freq		Sampling frequency in Hz
 *
 * @return -1 for error and 0 success.
 */
int max98095_set_sysclk(struct maxim_codec_priv *max98095,
				unsigned int freq)
{
	int error = 0;

	/* Requested clock frequency is already setup */
	if (freq == max98095->sysclk)
		return 0;

	/* Setup clocks for slave mode, and using the PLL
	 * PSCLK = 0x01 (when master clk is 10MHz to 20MHz)
	 *	0x02 (when master clk is 20MHz to 40MHz)..
	 *	0x03 (when master clk is 40MHz to 60MHz)..
	 */
	if ((freq >= 10000000) && (freq < 20000000)) {
		error = maxim_codec_i2c_write(M98095_026_SYS_CLK, 0x10);
	} else if ((freq >= 20000000) && (freq < 40000000)) {
		error = maxim_codec_i2c_write(M98095_026_SYS_CLK, 0x20);
	} else if ((freq >= 40000000) && (freq < 60000000)) {
		error = maxim_codec_i2c_write(M98095_026_SYS_CLK, 0x30);
	} else {
		debug("%s: Invalid master clock frequency\n", __func__);
		return -1;
	}

	debug("%s: Clock at %uHz\n", __func__, freq);

	if (error < 0)
		return -1;

	max98095->sysclk = freq;
	return 0;
}

/*
 * Sets Max98095 I2S format
 *
 * @param max98095	max98095 information
 * @param fmt		i2S format - supports a subset of the options defined
 *			in i2s.h.
 *
 * @return -1 for error and 0  Success.
 */
int max98095_set_fmt(struct maxim_codec_priv *max98095, int fmt)
{
	u8 regval = 0;
	int error = 0;

	if (fmt == max98095->fmt)
		return 0;

	max98095->fmt = fmt;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		/* Slave mode PLL */
		error |= maxim_codec_i2c_write(M98095_028_DAI1_CLKCFG_HI,
					0x80);
		error |= maxim_codec_i2c_write(M98095_029_DAI1_CLKCFG_LO,
					0x00);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		/* Set to master mode */
		regval |= M98095_DAI_MAS;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
	case SND_SOC_DAIFMT_CBM_CFS:
	default:
		debug("%s: Clock mode unsupported\n", __func__);
		return -1;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		regval |= M98095_DAI_DLY;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		debug("%s: Unrecognized format.\n", __func__);
		return -1;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		regval |= M98095_DAI_WCI;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		regval |= M98095_DAI_BCI;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		regval |= M98095_DAI_BCI | M98095_DAI_WCI;
		break;
	default:
		debug("%s: Unrecognized inversion settings.\n", __func__);
		return -1;
	}

	error |= maxim_codec_update_bits(M98095_02A_DAI1_FORMAT,
		M98095_DAI_MAS | M98095_DAI_DLY | M98095_DAI_BCI |
		M98095_DAI_WCI, regval);

	error |= maxim_codec_i2c_write(M98095_02B_DAI1_CLOCK,
		M98095_DAI_BSEL64);

	if (error < 0) {
		debug("%s: Error setting i2s format.\n", __func__);
		return -1;
	}

	return 0;
}

/*
 * resets the audio codec
 *
 * @return -1 for error and 0 success.
 */
static int max98095_reset(void)
{
	int i, ret;

	/*
	 * Gracefully reset the DSP core and the codec hardware in a proper
	 * sequence.
	 */
	ret = maxim_codec_i2c_write(M98095_00F_HOST_CFG, 0);
	if (ret != 0) {
		debug("%s: Failed to reset DSP: %d\n", __func__, ret);
		return ret;
	}

	ret = maxim_codec_i2c_write(M98095_097_PWR_SYS, 0);
	if (ret != 0) {
		debug("%s: Failed to reset codec: %d\n", __func__, ret);
		return ret;
	}

	/*
	 * Reset to hardware default for registers, as there is not a soft
	 * reset hardware control register.
	 */
	for (i = M98095_010_HOST_INT_CFG; i < M98095_REG_MAX_CACHED; i++) {
		ret = maxim_codec_i2c_write(i, 0);
		if (ret < 0) {
			debug("%s: Failed to reset: %d\n", __func__, ret);
			return ret;
		}
	}

	return 0;
}

/*
 * Intialise max98095 codec device
 *
 * @param max98095	max98095 information
 *
 * @returns -1 for error  and 0 Success.
 */
int max98095_device_init(struct maxim_codec_priv *max98095)
{
	unsigned char id;
	int error = 0;

	/* reset the codec, the DSP core, and disable all interrupts */
	error = max98095_reset();
	if (error != 0) {
		debug("Reset\n");
		return error;
	}

	/* initialize private data */
	max98095->sysclk = -1U;
	max98095->rate = -1U;
	max98095->fmt = -1U;

	error = maxim_codec_i2c_read(M98095_0FF_REV_ID, &id);
	if (error < 0) {
		debug("%s: Failure reading hardware revision: %d\n",
			__func__, id);
		goto err_access;
	}
	debug("%s: Hardware revision: %c\n", __func__, (id - 0x40) + 'A');

	error |= maxim_codec_i2c_write(M98095_097_PWR_SYS, M98095_PWRSV);

	/*
	 * initialize registers to hardware default configuring audio
	 * interface2 to DAC
	 */
	error |= maxim_codec_i2c_write(M98095_048_MIX_DAC_LR,
		M98095_DAI1L_TO_DACL|M98095_DAI1R_TO_DACR);

	error |= maxim_codec_i2c_write(M98095_092_PWR_EN_OUT,
			M98095_SPK_SPREADSPECTRUM);
	error |= maxim_codec_i2c_write(M98095_04E_CFG_HP, M98095_HPNORMAL);

	error |= maxim_codec_i2c_write(M98095_02C_DAI1_IOCFG,
			M98095_S1NORMAL|M98095_SDATA);

	/* take the codec out of the shut down */
	error |= maxim_codec_update_bits(M98095_097_PWR_SYS, M98095_SHDNRUN,
			M98095_SHDNRUN);
	/* route DACL and DACR output to HO and Spekers */
	error |= maxim_codec_i2c_write(M98095_050_MIX_SPK_LEFT, 0x01); /*DACL*/
	error |= maxim_codec_i2c_write(M98095_051_MIX_SPK_RIGHT, 0x01);/*DACR*/
	error |= maxim_codec_i2c_write(M98095_04C_MIX_HP_LEFT, 0x01);  /*DACL*/
	error |= maxim_codec_i2c_write(M98095_04D_MIX_HP_RIGHT, 0x01); /*DACR*/

	/* power Enable */
	error |= maxim_codec_i2c_write(M98095_091_PWR_EN_OUT, 0xF3);

	/* set Volume */
	error |= maxim_codec_i2c_write(M98095_064_LVL_HP_L, 15);
	error |= maxim_codec_i2c_write(M98095_065_LVL_HP_R, 15);
	error |= maxim_codec_i2c_write(M98095_067_LVL_SPK_L, 16);
	error |= maxim_codec_i2c_write(M98095_068_LVL_SPK_R, 16);

	/* Enable DAIs */
	error |= maxim_codec_i2c_write(M98095_093_BIAS_CTRL, 0x30);
	error |= maxim_codec_i2c_write(M98095_096_PWR_DAC_CK, 0x01);

err_access:
	if (error < 0)
		return -1;

	return 0;
}

