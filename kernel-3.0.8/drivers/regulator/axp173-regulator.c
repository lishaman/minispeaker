/*
 * act8600-regulator.c - regulator driver for ACT8600
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Written by Large Dipper <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/axp173-private.h>
#include <linux/mfd/pmu-common.h>


#define get_power_control_bit(id)       (1 << id)

struct axp173_reg {
	struct device	     * dev;
	struct axp173		 * iodev;
	struct regulator_dev **rdev;
};

enum {
	AXP173_DC1,
	AXP173_LDO4,
	AXP173_LDO2,
	AXP173_LDO3,
	AXP173_DC2,
	AXP173_DUMMY,
	AXP173_EXTEN,
	AXP173_POWER,
};

static int voltage_to_value_dc1_dc2_ldo4(unsigned int voltage)
{
   unsigned int value;
   value = (voltage -700000)/25000;
   return value;
}

static int voltage_to_value_ldo2_ldo3(unsigned int voltage)
{
   unsigned int value;
   value = (voltage -1800000)/100000;
   return value;
}

static int axp173_set_voltage_ldo4(struct regulator_dev *rdev,
			       int min_uV, int max_uV, unsigned *selector)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char value = 0;
	unsigned char reg = LDO4_REG;

	value = voltage_to_value_dc1_dc2_ldo4(min_uV);
	axp173_write_reg(client, reg, value);
	return 0;
}

static int axp173_set_voltage_ldo2(struct regulator_dev *rdev,
			       int min_uV, int max_uV, unsigned *selector)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char value = 0, tmp = 0;
	unsigned char reg = LDO2_REG;

	value = voltage_to_value_ldo2_ldo3(min_uV);
	axp173_read_reg(client, reg, &tmp);
	tmp &= ~(0xF << 4);
	tmp |= (value << 4);
	axp173_write_reg(client, reg, tmp);
	return 0;
}

static int axp173_set_voltage_ldo3(struct regulator_dev *rdev,
			       int min_uV, int max_uV, unsigned *selector)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char value = 0, tmp = 0;
	unsigned char reg = LDO3_REG;

	value = voltage_to_value_ldo2_ldo3(min_uV);
	axp173_read_reg(client, reg, &tmp);
	tmp &= ~0xF;
	tmp |= value;
	axp173_write_reg(client, reg, tmp);
	return 0;
}

static int axp173_is_enabled(struct regulator_dev *rdev)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char tmp = get_power_control_bit(rdev_get_id(rdev));
	unsigned char value;
	unsigned char reg = POWER_ON_OFF_REG;

	if (axp173_read_reg(client, reg, &value) < 0)
		return -1;
	return value & tmp;
}

static int axp173_enable(struct regulator_dev *rdev)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char tmp = get_power_control_bit(rdev_get_id(rdev));
	unsigned char value, timeout;
	unsigned char reg = POWER_ON_OFF_REG;

	axp173_read_reg(client, reg, &value);
	value |= tmp;
	axp173_write_reg(client, reg, value);

	for (timeout = 0; timeout < 20; timeout++) {
		axp173_read_reg(client, reg, &value);

		if (value & tmp)
			return 0;
		else
			dev_warn(axp173_reg->dev,
				 "not stable, wait for 10 ms\n");
		msleep(10);
	}
	dev_err(axp173_reg->dev, "enable failed!\n");

	return -1;
}

static int axp173_disable(struct regulator_dev *rdev)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char tmp = get_power_control_bit(rdev_get_id(rdev));
	unsigned char value;
	unsigned char reg = POWER_ON_OFF_REG;

	axp173_read_reg(client, reg, &value);
	value &= ~tmp;
	axp173_write_reg(client, reg, value);
	return 0;
}

static int axp173_get_current(struct regulator_dev *rdev)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char value;

	return 0;
}

static int axp173_set_current(struct regulator_dev *rdev,
			       int min_uA, int max_uA)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char value;

	return 0;
}

static int axp173_list_voltage(struct regulator_dev *rdev,
				unsigned int selector)
{
	return 0 ;
}

static int axp173_get_voltage(struct regulator_dev *rdev)
{
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char value;
	unsigned char reg ;

	return   0;
}

static int axp173_power_is_enabled(struct regulator_dev *rdev)
{
        struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
        struct i2c_client *client = axp173_reg->iodev->client;
        unsigned char tmp = get_power_control_bit(rdev_get_id(rdev));
        unsigned char value;
        unsigned char reg = POWER_BATDETECT_CHGLED_REG;

        if (axp173_read_reg(client, reg, &value) < 0)
                return -1;
        return value & tmp;
}

static int axp173_power_enable(struct regulator_dev *rdev)
{
	return 0;
}


static int axp173_power_disable(struct regulator_dev *rdev)
{
	printk("axp173  poweroff \n");
	struct axp173_reg *axp173_reg = rdev_get_drvdata(rdev);
	struct i2c_client *client = axp173_reg->iodev->client;
	unsigned char tmp = get_power_control_bit(rdev_get_id(rdev));
	unsigned char value;
	unsigned char reg = POWER_BATDETECT_CHGLED_REG;

	axp173_read_reg(client, reg, &value);
	value  |= tmp;
	axp173_write_reg(client, reg, value);

	return 0;
}

static struct regulator_ops axp173_dc2_ops = {
	.is_enabled = axp173_is_enabled,
	.enable = axp173_enable,
	.disable = axp173_disable,
	.list_voltage = axp173_list_voltage,
	.get_voltage = axp173_get_voltage,
};

static struct regulator_ops axp173_ldo2_ops = {
	.is_enabled = axp173_is_enabled,
	.enable = axp173_enable,
	.disable = axp173_disable,
	.list_voltage = axp173_list_voltage,
	.set_voltage = axp173_set_voltage_ldo2,
};

static struct regulator_ops axp173_ldo3_ops = {
	.is_enabled = axp173_is_enabled,
	.enable = axp173_enable,
	.disable = axp173_disable,
	.list_voltage = axp173_list_voltage,
	.set_voltage = axp173_set_voltage_ldo3,
};

static struct regulator_ops axp173_ldo4_ops = {
	.is_enabled = axp173_is_enabled,
	.enable = axp173_enable,
	.disable = axp173_disable,
	.set_voltage = axp173_set_voltage_ldo4,
};

static struct regulator_ops axp173_power_ops = {
	.is_enabled = axp173_power_is_enabled,
	.enable = axp173_power_enable,
	.disable = axp173_power_disable,
};

static struct regulator_ops axp173_exten_ops = {
	.is_enabled = axp173_is_enabled,
	.enable = axp173_enable,
	.disable = axp173_disable,
};

static struct regulator_desc axp173_regulators[] = {
	{
		.name = "DC2",
		.id = AXP173_DC2,
		.ops = &axp173_dc2_ops,
		.n_voltages = VSEL_MASK,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO2",
		.id = AXP173_LDO2,
		.ops = &axp173_ldo2_ops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO3",
		.id = AXP173_LDO3,
		.ops = &axp173_ldo3_ops,
		.n_voltages = VSEL_MASK,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "LDO4",
		.id = AXP173_LDO4,
		.ops = &axp173_ldo4_ops,
		.n_voltages = VSEL_MASK,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	{
		.name = "POWER",
		.id = AXP173_POWER,
		.ops = &axp173_power_ops,
		.n_voltages = VSEL_MASK,
		.owner = THIS_MODULE,
	},
	{
		.name = "EXTEN",
		.id = AXP173_EXTEN,
		.ops = &axp173_exten_ops,
		.n_voltages = VSEL_MASK,
		.owner = THIS_MODULE,
	},
};

static inline struct regulator_desc *find_desc(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(axp173_regulators); i++) {
		if (!strcmp(axp173_regulators[i].name, name))
			return &axp173_regulators[i];
	}

	return NULL;
}

static inline int reg_is_vbus(struct regulator_info *reg_info)
{
	if (!strcmp(reg_info->name, "VBUS"))
			return 1;

	return 0;
}

static __devinit int axp173_regulator_probe(struct platform_device *pdev)
{
	struct axp173 *iodev = dev_get_drvdata(pdev->dev.parent);
	struct axp173_reg *axp173_reg;
	struct pmu_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev;
	int i, ret, size;
	if (!pdata) {
		dev_err(pdev->dev.parent, "No platform init data supplied\n");
		return -ENODEV;
	}

	axp173_reg = kzalloc(sizeof(struct axp173_reg), GFP_KERNEL);
	if (!axp173_reg)
		return -ENOMEM;

	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
#ifdef CONFIG_CHARGER_ACT8600
	size++;
#endif
	axp173_reg->rdev = kzalloc(size, GFP_KERNEL);
	if (!axp173_reg->rdev) {
		kfree(axp173_reg);
		return -ENOMEM;
	}

	rdev = axp173_reg->rdev;
	axp173_reg->dev = &pdev->dev;
	axp173_reg->iodev = iodev;

	platform_set_drvdata(pdev, axp173_reg);

	for (i = 0; i < pdata->num_regulators; i++) {
		struct regulator_info *reg_info = &pdata->regulators[i];
		struct regulator_desc *desc = find_desc(reg_info->name);

		if (!desc) {
			dev_err(pdev->dev.parent,
				"WARNING: can't find regulator:%s\n", reg_info->name);
		} else {
			dev_dbg(pdev->dev.parent,
				"register regulator:%s\n", reg_info->name);
			if (reg_info->init_data) {
				rdev[i] = regulator_register(desc,
							                 axp173_reg->dev,
							    			 reg_info->init_data,
							     			 axp173_reg);
			} else {
				dev_err(axp173_reg->dev,
					"ERROR: no init_data available\n");
			}

			if (IS_ERR(rdev[i])) {
				ret = PTR_ERR(rdev[i]);
				dev_err(axp173_reg->dev,
					"regulator init failed\n");
				rdev[i] = NULL;
			}

			if (rdev[i] && desc->ops->is_enabled
					&& desc->ops->is_enabled(rdev[i])) {
				rdev[i]->use_count++;
			}
		}
	}

	return 0;
err:
	for (i = 0; i < pdata->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(axp173_reg->rdev);
	kfree(axp173_reg);

	return ret;
}

static int __devexit axp173_regulator_remove(struct platform_device *pdev)
{
	struct axp173_reg *axp173_reg = platform_get_drvdata(pdev);
	struct axp173 *iodev = dev_get_drvdata(pdev->dev.parent);
	struct pmu_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct regulator_dev **rdev = axp173_reg->rdev;
	int i;

	for (i = 0; i < pdata->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);

	kfree(axp173_reg->rdev);
	kfree(axp173_reg);

	return 0;
}

static struct platform_driver axp173_regulator_driver = {
	.driver = {
		.name = "axp173-regulator",
		.owner = THIS_MODULE,
	},
	.probe = axp173_regulator_probe,
	.remove = __devexit_p(axp173_regulator_remove),
};

static int __init axp173_regulator_init(void)
{
	return platform_driver_register(&axp173_regulator_driver);
}
subsys_initcall(axp173_regulator_init);

static void __exit axp173_regulator_cleanup(void)
{
	platform_driver_unregister(&axp173_regulator_driver);
}
module_exit(axp173_regulator_cleanup);

MODULE_DESCRIPTION("axp173 PMU regulator Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("smile <jbxu@ingenic.cn>");
