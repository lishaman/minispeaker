/*
 * regulator.c - This file defines board power information.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Large Dipper <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/pmu-common.h>
#include <linux/i2c.h>
#include <linux/cpufreq.h>
#include <gpio.h>

//#include "board_v1.h"
//#include "board_core.h"
#define PMU_I2C_BUSNUM		1

#if defined(CONFIG_USB_DWC2_DUAL_ROLE) || defined(CONFIG_USB_DWC2_HOST_ONLY)
FIXED_REGULATOR_DEF(
        vbus,
        "VBUS",           3000000,       GPIO_PB(25),
        HIGH_ENABLE,    UN_AT_BOOT,     0,
        NULL,           "vbus",       NULL);
#endif

static struct platform_device *fixed_regulator_devices[] __initdata = {
#if defined(CONFIG_USB_DWC2_DUAL_ROLE) || defined(CONFIG_USB_DWC2_HOST_ONLY)
        &vbus_regulator_device,
#endif
};

/**
 * Core voltage Regulator.
 * Couldn't be modified.
 * Voltage was inited at bootloader.
 */
CORE_REGULATOR_DEF(
        trunk,  1000000,        1200000);

IO_REGULATOR_DEF(
        vwifi,
        "vwifi",       3500000,        1);
/**
 * Exclusive Regulators.
 * They are only used by one device each other.
 */
EXCLUSIVE_REGULATOR_DEF(
        vldo2,
        "Vldo2",	"vldo2",        NULL,
        NULL,   	2500000,        1);

EXCLUSIVE_REGULATOR_DEF(
        vldo3,
        "Vldo3",       "vldo3",		NULL,
        NULL,   	3300000,        1);

EXCLUSIVE_REGULATOR_DEF(
        vpower,
        "Vpower",       "vpower",	NULL,
        NULL,   	3300000,        0);

/*
 * Regulators definitions used in PMU.
 *
 * If +5V is supplied by PMU, please define "VBUS" here with init_data NULL,
 * otherwise it should be supplied by a exclusive DC-DC, and you should define
 * it as a fixed regulator.
 */
static struct regulator_info pmu_regulators[] = {
        {"DC2", &trunk_vcore_init_data},
        {"LDO4", &vwifi_init_data},
        {"LDO3", &vldo3_init_data},
        {"LDO2", &vldo2_init_data},
	{"POWER", &vpower_init_data},
};

static struct charger_board_info charger_board_info = {
        .gpio           = -1,
        .enable_level   = LOW_ENABLE,
};

static struct pmu_platform_data pmu_pdata = {
        .gpio                   = GPIO_PC(23),
        .num_regulators         = ARRAY_SIZE(pmu_regulators),
        .regulators             = pmu_regulators,
        .charger_board_info     = &charger_board_info,
};

//#define PMU_I2C_BUSNUM 1  //by xhhu:change 1

struct i2c_board_info pmu_board_info = {
        I2C_BOARD_INFO("axp173", 0x34),
        .platform_data = &pmu_pdata,
};

static int __init pmu_dev_init(void)
{
        struct i2c_adapter *adap;
        struct i2c_client *client;
        int busnum = PMU_I2C_BUSNUM;

        adap = i2c_get_adapter(busnum);
        if (!adap) {
                pr_err("failed to get adapter i2c%d\n", busnum);
                return -1;
        }

        client = i2c_new_device(adap, &pmu_board_info);
        if (!client) {
                pr_err("failed to register pmu to i2c%d\n", busnum);
                return -1;
        }

        i2c_put_adapter(adap);

        return 0;
}

subsys_initcall_sync(pmu_dev_init);



static int __init regulator_dev_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fixed_regulator_devices); i++)
		fixed_regulator_devices[i]->id = i;

	return platform_add_devices(fixed_regulator_devices,
				    ARRAY_SIZE(fixed_regulator_devices));
}

arch_initcall_sync(regulator_dev_init);

