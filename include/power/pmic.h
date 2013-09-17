/*
 *  Copyright (C) 2011-2012 Samsung Electronics
 *  Lukasz Majewski <l.majewski@samsung.com>
 *
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

#ifndef __CORE_PMIC_H_
#define __CORE_PMIC_H_

#include <common.h>
#include <linux/list.h>
#include <i2c.h>
#include <power/power_chrg.h>
#include <fdtdec.h>

enum { PMIC_I2C, PMIC_SPI, PMIC_NONE};
enum { I2C_PMIC, I2C_NUM, };
enum { PMIC_READ, PMIC_WRITE, };
enum { PMIC_SENSOR_BYTE_ORDER_LITTLE, PMIC_SENSOR_BYTE_ORDER_BIG, };

struct p_i2c {
	unsigned char addr;
	unsigned char *buf;
	unsigned char tx_num;
};

struct p_spi {
	unsigned int cs;
	unsigned int mode;
	unsigned int bitlen;
	unsigned int clk;
	unsigned int flags;
	u32 (*prepare_tx)(u32 reg, u32 *val, u32 write);
};

struct pmic;
struct power_fg {
	int (*fg_battery_check) (struct pmic *p, struct pmic *bat);
	int (*fg_battery_update) (struct pmic *p, struct pmic *bat);
};

struct power_chrg {
	int (*chrg_type) (struct pmic *p);
	int (*chrg_bat_present) (struct pmic *p);
	int (*chrg_state) (struct pmic *p, int state, int current);
};

struct power_battery {
	struct battery *bat;
	int (*battery_init) (struct pmic *bat, struct pmic *p1,
			     struct pmic *p2, struct pmic *p3);
	int (*battery_charge) (struct pmic *bat);
	/* Keep info about power devices involved with battery operation */
	struct pmic *chrg, *fg, *muic;
};

struct pmic {
	const char *name;
	unsigned char bus;
	unsigned char interface;
	unsigned char sensor_byte_order;
	unsigned int number_of_regs;
	union hw {
		struct p_i2c i2c;
		struct p_spi spi;
	} hw;

	void (*low_power_mode) (void);
	struct power_battery *pbat;
	struct power_chrg *chrg;
	struct power_fg *fg;

	struct pmic *parent;
	struct list_head list;
#ifdef CONFIG_OF_CONTROL
	enum fdt_compat_id compat_id;
#endif
};

int pmic_init(unsigned char bus);
int pmic_dialog_init(unsigned char bus);
int check_reg(struct pmic *p, u32 reg);
struct pmic *pmic_alloc(void);
struct pmic *pmic_get(const char *s);
int pmic_probe(struct pmic *p);
int pmic_reg_read(struct pmic *p, u32 reg, u32 *val);
int pmic_reg_write(struct pmic *p, u32 reg, u32 val);
int pmic_set_output(struct pmic *p, u32 reg, int ldo, int on);
int pmic_set_voltage(u32 new_voltage);

/**
 * Find registered PMIC based on its compatibility ID.
 *
 * @param pmic_compat   compatibility ID of the PMIC to search for.
 * @return pointer to the relevant 'struct pmic' on success or NULL
 */
struct pmic *pmic_get_by_id(enum fdt_compat_id pmic_compat);

/**
 * Update contents of a pmic register.
 *
 * This function reads a register, sets the bits which are set in the third
 * parameter and writes the resulting value back. Its use is not error prone
 * as if it is trying to set bit fields and the fields, the new value will be
 * mixed with the old value.
 *
 * TODO(vbendeb): Introduce a mask to make sure that the field is set as
 * required (http://crosbug.com/39884).
 *
 * @param p	  pointer to the pmic structure of the PMIC to access
 * @param reg	  register address of the register to modify
 * @param regval  bits to set in the register
 * @return zero on success, nonzero on failure
 */
int pmic_reg_update(struct pmic *p, int reg, uint regval);

/**
 * Update contents of a pmic register by optionally clearing and setting bits.
 *
 * This function reads a register, clears the bits in bic, sets the bits in
 * or, and writes the resulting value back.
 *
 * @param p	  pointer to the pmic structure of the PMIC to access
 * @param reg	  register address of the register to modify
 * @param bic	  bits to clear in the register (0=none)
 * @param or	  bits to set in the register (0=none)
 * @return zero on success, nonzero on failure
 */
int pmic_reg_clear_bits_masked(struct pmic *p, int reg, uint bic, uint or);

#ifdef CONFIG_OF_CONTROL
enum pmic_reg_op {
	PMIC_REG_BAIL,
	PMIC_REG_WRITE,
	PMIC_REG_UPDATE,
	PMIC_REG_CLEAR,
};

struct pmic_init_ops {
	enum pmic_reg_op reg_op;
	u8	reg_addr;
	u8	reg_value;
};

/**
 * Common function used to intialize an i2c based PMIC.
 *
 * This function finds the PMIC in the device tree based on its compatibility
 * ID. If found, the struct pmic is allocated, initialized and registered.
 *
 * Then the table of initialization settings is scanned and the PMIC registers
 * are set as dictated by the table contents,
 *
 * @param pmic_compat   compatibility ID f the PMIC to be initialized.
 * @param pmic_ops      a pointer to the table containing PMIC initialization
 *			settings. The last entry contains reg_op
 *			of PMIC_REG_BAIL.
 * @return zero on success, nonzero on failure
 */
int pmic_common_init(enum fdt_compat_id pmic_compat,
		     const struct pmic_init_ops *pmic_ops);
#endif

#define pmic_i2c_addr (p->hw.i2c.addr)
#define pmic_i2c_tx_num (p->hw.i2c.tx_num)

#define pmic_spi_bitlen (p->hw.spi.bitlen)
#define pmic_spi_flags (p->hw.spi.flags)

#endif /* __CORE_PMIC_H_ */
