/*
 * (C) Copyright 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/gpio.h>

#define CON_MASK(x)		(0xf << ((x) << 2))
#define CON_SFR(x, v)		((v) << ((x) << 2))

#define DAT_MASK(x)		(0x1 << (x))
#define DAT_SET(x)		(0x1 << (x))

#define PULL_MASK(x)		(0x3 << ((x) << 1))
#define PULL_MODE(x, v)		((v) << ((x) << 1))

#define DRV_MASK(x)		(0x3 << ((x) << 1))
#define DRV_SET(x, m)		((m) << ((x) << 1))
#define RATE_MASK(x)		(0x1 << (x + 16))
#define RATE_SET(x)		(0x1 << (x + 16))

/*
 * This structure helps mapping symbolic GPIO names into indices from
 * exynos5_gpio_pin/exynos5420_gpio_pin enums.
 *
 * By convention, symbolic GPIO name is defined as follows:
 *
 * g[p]<bank><set><bit>, where
 *   p is optional
 *   <bank> - a single character bank name, as defined by the SOC
 *   <set> - a single digit set number
 *   <bit> - bit number within the set (in 0..7 range).
 *
 * <set><bit> essentially form an octal number of the GPIO pin within the bank
 * space. On the 5420 architecture some banks' sets do not start not from zero
 * ('d' starts from 1 and 'j' starts from 4). To compensate for that and
 * maintain flat number space withoout holes, those banks use offsets to be
 * deducted from the pin number.
 */
struct gpio_name_num_table {
	char bank;		/* bank name symbol */
	u8 bank_size;		/* total number of pins in the bank */
	char bank_offset;	/* offset of the first bank's pin */
	unsigned int base;	/* index of the first bank's pin in the enum */
};

#define GPIO_ENTRY(name, base, top, offset) { name, top - base, offset, base }
static const struct gpio_name_num_table exynos5_gpio_table[] = {
	GPIO_ENTRY('a', EXYNOS5_GPIO_A00, EXYNOS5_GPIO_B00, 0),
        GPIO_ENTRY('b', EXYNOS5_GPIO_B00, EXYNOS5_GPIO_C00, 0),
	GPIO_ENTRY('c', EXYNOS5_GPIO_C00, EXYNOS5_GPIO_D00, 0),
	GPIO_ENTRY('d', EXYNOS5_GPIO_D00, EXYNOS5_GPIO_Y00, 0),
	GPIO_ENTRY('y', EXYNOS5_GPIO_Y00, EXYNOS5_GPIO_C40, 0),
	GPIO_ENTRY('x', EXYNOS5_GPIO_X00, EXYNOS5_GPIO_E00, 0),
	GPIO_ENTRY('e', EXYNOS5_GPIO_E00, EXYNOS5_GPIO_F00, 0),
	GPIO_ENTRY('f', EXYNOS5_GPIO_F00, EXYNOS5_GPIO_G00, 0),
	GPIO_ENTRY('g', EXYNOS5_GPIO_G00, EXYNOS5_GPIO_H00, 0),
	GPIO_ENTRY('h', EXYNOS5_GPIO_H00, EXYNOS5_GPIO_V00, 0),
	GPIO_ENTRY('v', EXYNOS5_GPIO_V00, EXYNOS5_GPIO_Z0, 0),
	GPIO_ENTRY('z', EXYNOS5_GPIO_Z0, EXYNOS5_GPIO_MAX_PORT, 0),
	{ 0 }
};

static const struct gpio_name_num_table exynos5420_gpio_table[] = {
	GPIO_ENTRY('a', EXYNOS5420_GPIO_A00, EXYNOS5420_GPIO_B00, 0),
	GPIO_ENTRY('b', EXYNOS5420_GPIO_B00, EXYNOS5420_GPIO_H00, 0),
	GPIO_ENTRY('h', EXYNOS5420_GPIO_H00, EXYNOS5420_GPIO_Y70, 0),
	GPIO_ENTRY('x', EXYNOS5420_GPIO_X00, EXYNOS5420_GPIO_C00, 0),
	GPIO_ENTRY('c', EXYNOS5420_GPIO_C00, EXYNOS5420_GPIO_D10, 0),
	GPIO_ENTRY('d', EXYNOS5420_GPIO_D10, EXYNOS5420_GPIO_Y00, 010),
	GPIO_ENTRY('y', EXYNOS5420_GPIO_Y00, EXYNOS5420_GPIO_E00, 0),
	GPIO_ENTRY('e', EXYNOS5420_GPIO_E00, EXYNOS5420_GPIO_F00, 0),
	GPIO_ENTRY('f', EXYNOS5420_GPIO_F00, EXYNOS5420_GPIO_G00, 0),
	GPIO_ENTRY('g', EXYNOS5420_GPIO_G00, EXYNOS5420_GPIO_J40, 0),
	GPIO_ENTRY('j', EXYNOS5420_GPIO_J40, EXYNOS5420_GPIO_Z0, 040),
	GPIO_ENTRY('z', EXYNOS5420_GPIO_Z0, EXYNOS5420_GPIO_MAX_PORT, 0),
	{ 0 }
};

void s5p_gpio_cfg_pin(struct s5p_gpio_bank *bank, int gpio, int cfg)
{
	unsigned int value;

	value = readl(&bank->con);
	value &= ~CON_MASK(gpio);
	value |= CON_SFR(gpio, cfg);
	writel(value, &bank->con);
}

void s5p_gpio_direction_output(struct s5p_gpio_bank *bank, int gpio, int en)
{
	unsigned int value;

	s5p_gpio_cfg_pin(bank, gpio, S5P_GPIO_OUTPUT);

	value = readl(&bank->dat);
	value &= ~DAT_MASK(gpio);
	if (en)
		value |= DAT_SET(gpio);
	writel(value, &bank->dat);
}

void s5p_gpio_direction_input(struct s5p_gpio_bank *bank, int gpio)
{
	s5p_gpio_cfg_pin(bank, gpio, S5P_GPIO_INPUT);
}

void s5p_gpio_set_value(struct s5p_gpio_bank *bank, int gpio, int en)
{
	unsigned int value;

	value = readl(&bank->dat);
	value &= ~DAT_MASK(gpio);
	if (en)
		value |= DAT_SET(gpio);
	writel(value, &bank->dat);
}

unsigned int s5p_gpio_get_value(struct s5p_gpio_bank *bank, int gpio)
{
	unsigned int value;

	value = readl(&bank->dat);
	return !!(value & DAT_MASK(gpio));
}

void s5p_gpio_set_pull(struct s5p_gpio_bank *bank, int gpio, int mode)
{
	unsigned int value;

	value = readl(&bank->pull);
	value &= ~PULL_MASK(gpio);

	switch (mode) {
	case S5P_GPIO_PULL_DOWN:
	case S5P_GPIO_PULL_UP:
		value |= PULL_MODE(gpio, mode);
		break;
	default:
		break;
	}

	writel(value, &bank->pull);
}

void s5p_gpio_set_drv(struct s5p_gpio_bank *bank, int gpio, int mode)
{
	unsigned int value;

	value = readl(&bank->drv);
	value &= ~DRV_MASK(gpio);

	switch (mode) {
	case S5P_GPIO_DRV_1X:
	case S5P_GPIO_DRV_2X:
	case S5P_GPIO_DRV_3X:
	case S5P_GPIO_DRV_4X:
		value |= DRV_SET(gpio, mode);
		break;
	default:
		return;
	}

	writel(value, &bank->drv);
}

void s5p_gpio_set_rate(struct s5p_gpio_bank *bank, int gpio, int mode)
{
	unsigned int value;

	value = readl(&bank->drv);
	value &= ~RATE_MASK(gpio);

	switch (mode) {
	case S5P_GPIO_DRV_FAST:
	case S5P_GPIO_DRV_SLOW:
		value |= RATE_SET(gpio);
		break;
	default:
		return;
	}

	writel(value, &bank->drv);
}

static struct s5p_gpio_bank *s5p_gpio_get_bank(unsigned int gpio)
{
	const struct gpio_info *data;
	unsigned int upto;
	int i, count;

	data = get_gpio_data();
	count = get_bank_num();
	for (i = upto = 0; i < count;
			i++, upto = data->max_gpio, data++) {
		debug("i=%d, upto=%d\n", i, upto);
		if (gpio < data->max_gpio) {
			struct s5p_gpio_bank *bank;
			bank = (struct s5p_gpio_bank *)data->reg_addr;
			bank += (gpio - upto) / GPIO_PER_BANK;
			debug("gpio=%d, bank=%p\n", gpio, bank);
			return bank;
		}
	}
	return NULL;
}

int s5p_gpio_get_pin(unsigned gpio)
{
	return gpio % GPIO_PER_BANK;
}

/* Common GPIO API */

int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_direction_input(unsigned gpio)
{
	s5p_gpio_direction_input(s5p_gpio_get_bank(gpio),
				s5p_gpio_get_pin(gpio));
	return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
	s5p_gpio_direction_output(s5p_gpio_get_bank(gpio),
				 s5p_gpio_get_pin(gpio), value);
	return 0;
}

int gpio_get_value(unsigned gpio)
{
	return (int) s5p_gpio_get_value(s5p_gpio_get_bank(gpio),
				       s5p_gpio_get_pin(gpio));
}

int gpio_set_value(unsigned gpio, int value)
{
	s5p_gpio_set_value(s5p_gpio_get_bank(gpio),
			  s5p_gpio_get_pin(gpio), value);

	return 0;
}

void gpio_set_pull(int gpio, int value)
{
	s5p_gpio_set_pull(s5p_gpio_get_bank(gpio),
			  s5p_gpio_get_pin(gpio), value);
}

void gpio_cfg_pin(int gpio, int cfg)
{
	s5p_gpio_cfg_pin(s5p_gpio_get_bank(gpio),
			 s5p_gpio_get_pin(gpio), cfg);
}

void gpio_set_drv(int gpio, int drv)
{
	s5p_gpio_set_drv(s5p_gpio_get_bank(gpio),
			 s5p_gpio_get_pin(gpio), drv);
}

/*
 * Add a delay here to give the lines time to settle
 * TODO(sjg): 1us does not always work, 2 is stable, so use 5 to be safe
 * Come back to this and sort out what the datasheet says
 */
#define GPIO_DELAY_US 5

int gpio_decode_number(unsigned gpio_list[], int count)
{
	int result = 0;
	int multiplier = 1;
	int value, high, low;
	int gpio, i;

	for (i = 0; i < count; i++) {
		gpio = gpio_list[i];

		/* TODO(SLSI): Fix this up to check correctly:
		 *
		 * if (gpio >= GPIO_MAX_PORT)
		 *	return -1;
		 */
		gpio_direction_input(gpio);
		gpio_set_pull(gpio, S5P_GPIO_PULL_UP);
		udelay(GPIO_DELAY_US);
		high = gpio_get_value(gpio);
		gpio_set_pull(gpio, S5P_GPIO_PULL_DOWN);
		udelay(GPIO_DELAY_US);
		low = gpio_get_value(gpio);

		if (high && low)
			value = 2;
		else if (!high && !low)
			value = 1;
		else
			value = 0;
		result += value * multiplier;
		multiplier *= 3;
	}

	return result;
}

int s5p_name_to_gpio(const char *name)
{
	unsigned num, irregular_set_number, irregular_bank_base;
	const struct gpio_name_num_table *tabp;
	char this_bank, bank_name, irregular_bank_name;
	char *endp;

	/*
	 * The gpio name starts with either 'g' ot 'gp' followed by the bank
	 * name character. Skip one or two characters depending on the prefix.
	 */
	if (name[1] == 'p')
		name += 2;
	else
		name++;
	bank_name = *name++;
	if (!*name)
		return -1; /* At least one digit is required/expected. */

	/*
	 * On both exynos5 and exynos5420 architectures there is a bank of
	 * GPIOs which does not fall into the regular address pattern. Those
	 * banks are c4 on Exynos5 and y7 on Exynos5420. The rest of the below
	 * assignments help to handle these irregularities.
	 */
	if (proid_is_exynos5420()) {
		tabp = exynos5420_gpio_table;
		irregular_bank_name = 'y';
		irregular_set_number = '7';
		irregular_bank_base = EXYNOS5420_GPIO_Y70;
	} else {
		tabp = exynos5_gpio_table;
		irregular_bank_name = 'c';
		irregular_set_number = '4';
		irregular_bank_base = EXYNOS5_GPIO_C40;
	}

	this_bank = tabp->bank;
	do {
		if (bank_name == this_bank) {
			unsigned pin_index; /* pin number within the bank */
			if ((bank_name == irregular_bank_name) &&
			    (name[0] == irregular_set_number)) {
				pin_index = name[1] - '0';
				/* Irregular sets have 8 pins. */
				if (pin_index >= GPIO_PER_BANK)
					return -1;
				num = irregular_bank_base + pin_index;
			} else {
				pin_index = simple_strtoul(name, &endp, 8);
				pin_index -= tabp->bank_offset;
				/*
				 * Sanity check: bunk 'z' has no set number,
				 * for all other banks there must be exactly
				 * two octal digits, and the resulting number
				 * should not exceed the number of pins in the
				 * bank.
				 */
				if (((bank_name != 'z') && !name[1]) ||
				    *endp ||
				    (pin_index >= tabp->bank_size))
					return -1;
				num = tabp->base + pin_index;
			}
			return num;
		}
		this_bank = (++tabp)->bank;
	} while (this_bank);

	return -1;
}

void s5p_gpio_describe(const char* gpio_name)
{
	struct s5p_gpio_bank *bank;
	int index = s5p_name_to_gpio(gpio_name);
	int pin = s5p_gpio_get_pin(index);

	if (index < 0) {
		printf ("%s is not a valid GPIO name\n", gpio_name);
		return;
	}

	bank = s5p_gpio_get_bank(index);

	printf("%s: pin %d, index %d, config 0x%x at %p\n",
	       gpio_name, pin, index,
	       ((readl(&bank->con) & CON_MASK(pin)) >> (pin << 2)) & 0xf,
	       &bank->con);
}
