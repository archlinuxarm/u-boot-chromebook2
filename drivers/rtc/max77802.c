/*
 * Copyright (c) 2013 The Chromium OS Authors.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <fdtdec.h>
#include <rtc.h>
#include <power/pmic.h>
#include <power/max77802_pmic.h>
#include <asm/arch-exynos/spl.h>

DECLARE_GLOBAL_DATA_PTR;

enum {
	DATA_SECOND,
	DATA_MINUTE,
	DATA_HOUR,
	DATA_WEEKDAY,
	DATA_MONTH,
	DATA_YEAR,
	DATA_DATE,

	DATA_SIZE
};

/*
 * According to the 77802 spec, trnsferring data between holding registers and
 * counters could take up to 200 us.
 *
 * Say we communicate with the chip using 1MHz clock (in reality it is
 * slower). Reading the status register involves sending the i2 address, i2c
 * offset, i2c address again, and the actual value, i.e. 40 clocks (8 bit per
 * byte plus start and stop).
 *
 * Let's give it a three fold margin of error (megahertzs and microseonds
 * nicely compensate each other).
 */
#define MAX_RTC_UPDATE_CYCLES (3 * 200 / 40)

struct pmic *rtc_pmic;

static int rtc_init(void)
{
	struct pmic *ppmic;
	int ret;

	if (rtc_pmic)
		return 0;

	ppmic = pmic_get(MAX77802_DEVICE_NAME);

	ret = pmic_reg_write(ppmic, MAX77802_REG_PMIC_RTCCNTLM,
			     MAX77802_RTCCNTLM_BCDM |
			     MAX77802_RTCCNTLM_HRMODEM);
	if  (ret) {
		printf("%s: Failed to set rtccntlm.\n", __func__);
		return ret;
	}

	ret = pmic_reg_write(ppmic, MAX77802_REG_PMIC_RTCCNTL,
			     MAX77802_RTCCNTL_HRMODE);
	if (ret) {
		printf("%s: Failed to set rtccntl.\n", __func__);
		return ret;
	}

	rtc_pmic = ppmic;
	return 0;
}

int rtc_get(struct rtc_time *tm)
{
	uint8_t data[DATA_SIZE];
	uint32_t update0;
	int i = 0;

	if (rtc_init())
		return -1;

	if (pmic_reg_read(rtc_pmic, MAX77802_REG_PMIC_RTCUPDATE0, &update0) ||
	    pmic_reg_write(rtc_pmic, MAX77802_REG_PMIC_RTCUPDATE0,
			   update0 | MAX77802_RTCUPDATE0_RBUDR)) {
		printf("%s: Failed to access rtcupdate0.\n", __func__);
		return -1;
	}

	do {
		if (i++ > MAX_RTC_UPDATE_CYCLES) {
			printf("%s: time out polling rtcupdate0.\n", __func__);
			return -1;
		}
		if (pmic_reg_read(rtc_pmic, MAX77802_REG_PMIC_RTCUPDATE0,
				  &update0)) {
			printf("%s: Failed to access rtcupdate0.\n", __func__);
			return -1;
		}
	} while (update0 & MAX77802_RTCUPDATE0_RBUDR);

	for (i = 0; i < sizeof(data); i++) {
		uint32_t value;

		if (pmic_reg_read(rtc_pmic,
				  MAX77802_REG_PMIC_RTCSEC + i, &value)) {
			printf("%s: Failed to read from the RTC (%d).\n",
			       __func__, i);
			return -1;
		}
		data[i] = value;
	}

	tm->tm_sec = data[DATA_SECOND];
	tm->tm_min = data[DATA_MINUTE];
	tm->tm_hour = data[DATA_HOUR] & 0x3f;
	tm->tm_wday = fls(data[DATA_WEEKDAY]) - 1;
	tm->tm_mday = data[DATA_DATE];
	tm->tm_mon = data[DATA_MONTH];
	tm->tm_year = data[DATA_YEAR];

	/* Support years from 1970 to 2069. */
	if (tm->tm_year < 70)
		tm->tm_year += 2000;
	else
		tm->tm_year += 1900;

	return 0;
}

int rtc_set(struct rtc_time *tm)
{
	uint8_t data[DATA_SIZE];
	uint32_t update0;
	int i;

	if (rtc_init()) {
		printf("%s: Failed to initialize the RTC.\n", __func__);
		return -1;
	}

	memset(data, 0, sizeof(data));
	data[DATA_SECOND] = tm->tm_sec;
	data[DATA_MINUTE] = tm->tm_min;
	data[DATA_HOUR] = tm->tm_hour;
	data[DATA_WEEKDAY] = 1 << tm->tm_wday;
	data[DATA_DATE] = tm->tm_mday;
	data[DATA_MONTH] = tm->tm_mon;
	data[DATA_YEAR] = tm->tm_year % 100;

	if (tm->tm_hour > 12)
		data[DATA_HOUR] |= MAX77802_RTCHOUR_AMPM;

	for (i = 0; i < sizeof(data); i++) {
		if (pmic_reg_write(rtc_pmic, MAX77802_REG_PMIC_RTCSEC + i,
				   data[i])) {
			printf("%s: Failed to set data registers (%d).\n",
			       __func__, i);
			return -1;
		}
	}

	if (pmic_reg_read(rtc_pmic, MAX77802_REG_PMIC_RTCUPDATE0, &update0) ||
	    pmic_reg_write(rtc_pmic, MAX77802_REG_PMIC_RTCUPDATE0,
			   update0 | MAX77802_RTCUPDATE0_UDR)) {
		printf("%s: Failed to access rtcupdate0.\n", __func__);
		return -1;
	}

	i = 0;
	do {
		if (i++ > MAX_RTC_UPDATE_CYCLES) {
			printf("%s: time out polling rtcupdate0.\n", __func__);
			return -1;
		}
		if (pmic_reg_read(rtc_pmic, MAX77802_REG_PMIC_RTCUPDATE0,
				  &update0)) {
			printf("%s: Failed to access rtcupdate0.\n", __func__);
			return -1;
		}
	} while (update0 & MAX77802_RTCUPDATE0_UDR);

	return 0;
}

void rtc_reset(void)
{
	struct rtc_time tm;

	if (!rtc_pmic)
		return;
	memset(&tm, 0, sizeof(tm));
	rtc_set(&tm);
	rtc_pmic = 0;
}
