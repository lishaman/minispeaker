/*
 * drivers/regulator/axp173-regulator.c
 *
 * Regulator driver for RICOH R5T619 power management chip.
 *
 * Copyright (C) 2012-2014 RICOH COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*#define DEBUG			1*/
/*#define VERBOSE_DEBUG		1*/
#include <config.h>
#include <common.h>
#include <linux/err.h>
#include <linux/list.h>
#include <regulator.h>
#include <ingenic_soft_i2c.h>
#include <power/axp173.h>

#define AXP173_I2C_ADDR    0x34

static struct i2c axp173_i2c;
static struct i2c *i2c;

int axp173_write_reg(u8 reg, u8 *val)
{
	unsigned int  ret;
	ret = i2c_write(i2c, AXP173_I2C_ADDR, reg, 1, val, 1);
	if(ret) {
		debug("axp173 write register error\n");
		return -EIO;
	}
	return 0;
}

int axp173_read_reg(u8 reg, u8 *val, u32 len)
{
	int ret;
	ret = i2c_read(i2c, AXP173_I2C_ADDR, reg, 1, val, len);
	if(ret) {
		debug("axp173 read register error\n");
		return -EIO;
	}
	return 0;
}

int axp173_power_off(void)
{
	int ret;
	uint8_t reg_val;

	ret = axp173_read_reg(AXP173_POWER_OFF, &reg_val,1);
	if (ret < 0)
		printf("Error in reading the POWEROFF_Reg\n");
	reg_val |= (1 << 7);
	ret = axp173_write_reg(AXP173_POWER_OFF, &reg_val);
	if (ret < 0)
		printf("Error in writing the POWEROFF_Reg\n");
	return 0;
}

int axp173_regulator_init(void)
{
	int ret;
	axp173_i2c.scl = CONFIG_AXP173_I2C_SCL;
	axp173_i2c.sda = CONFIG_AXP173_I2C_SDA;
	i2c = &axp173_i2c;
	i2c_init(i2c);

	ret = i2c_probe(i2c, AXP173_I2C_ADDR);
	if(ret) {
		printf("probe axp173 error, i2c addr 0x%x\n", AXP173_I2C_ADDR);
		return -EIO;
	}

	return 0;
}
