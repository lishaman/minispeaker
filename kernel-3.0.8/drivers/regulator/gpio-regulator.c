/*
 * gpio-regulator.c  --  Regulator driver for the Ingenic Platform
 *
 * Author: ztyan<ztyan@ingenic.cn>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <linux/regulator/machine.h>

#include "gpio-regulator.h"

struct gpio_pwc {
	int flags;
	int gpio;
	struct regulator_dev *regulator;
	struct regulator_desc gpio_pwc_desc;
};

int gpio_pwc_enable(struct regulator_dev *dev)
{
	struct gpio_pwc *pwc = rdev_get_drvdata(dev);
	gpio_direction_output(pwc->gpio,pwc->flags == 0);
	return 0;
}

int gpio_pwc_disable(struct regulator_dev *dev)
{
	struct gpio_pwc *pwc = rdev_get_drvdata(dev);
	gpio_direction_output(pwc->gpio,pwc->flags != 0);
	return 0;
}

int gpio_pwc_is_enabled(struct regulator_dev *dev)
{
	struct gpio_pwc *pwc = rdev_get_drvdata(dev);
	if(pwc->flags)
		return !gpio_get_value(pwc->gpio);
	return gpio_get_value(pwc->gpio);
}

static struct regulator_ops gpio_pwc_ops = {
	.enable = gpio_pwc_enable,
	.disable = gpio_pwc_disable,
	.is_enabled = gpio_pwc_is_enabled,
};

static struct regulator_init_data init_data = {
	.supply_regulator = NULL,
	.constraints.min_uV = 3300000,
	.constraints.max_uV = 3300000,
	.constraints.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	.constraints.valid_modes_mask = REGULATOR_MODE_NORMAL,
	.num_consumer_supplies = 0,
	.consumer_supplies = NULL,
};

void gpio_pwc_fill_desc(struct gpio_pwc *pwc,struct gpio_pwc_data *data)
{
	static int i = 0;
	pwc->flags = data->flags;
	pwc->gpio = data->gpio;
	pwc->gpio_pwc_desc.name = data->name;

	pwc->gpio_pwc_desc.id = i++;
	pwc->gpio_pwc_desc.type = REGULATOR_CURRENT;
	pwc->gpio_pwc_desc.n_voltages = 1;
	pwc->gpio_pwc_desc.ops = &gpio_pwc_ops;
	pwc->gpio_pwc_desc.owner = THIS_MODULE;

	init_data.supply_regulator = data->parent;
}

static __devinit int gpio_pwc_probe(struct platform_device *pdev)
{
	int i,ret;
	struct gpio_pwc *pwc;
	struct gpio_pwc_platform_data *pdata = pdev->dev.platform_data;

	if(pdata == NULL) {
		dev_err(&pdev->dev, "Platform data CAN NOT be empty.\n");
		return -ENODEV;
	}

	pwc = kzalloc((sizeof(struct gpio_pwc) * pdev->num_resources), GFP_KERNEL);
	if (pwc == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data.\n");
		return -ENOMEM;
	}

	for(i=0;i<pdata->count;i++) {
		if(!gpio_is_valid(pdata->data[i].gpio)) {
			dev_err(&pdev->dev,"Failed to register GPIO PWC-%s,gpio is invalid.\n",
					pdev->resource[i].name);
			continue;
		}

		if(gpio_request(pwc[i].gpio,pwc[i].gpio_pwc_desc.name)) {
			dev_err(&pdev->dev,"Failed to register GPIO PWC-%s,request gpio failed.\n",
					pdev->resource[i].name);
			continue;
		}

		gpio_direction_output(pwc[i].gpio,pwc[i].flags != 0);

		gpio_pwc_fill_desc(&pwc[i],&pdata->data[i]);

		pwc[i].regulator = regulator_register(&pwc[i].gpio_pwc_desc,&pdev->dev,&init_data,&pwc[i]);

		if (IS_ERR(pwc[i].regulator)) {
			ret = PTR_ERR(pwc[i].regulator);
			dev_err(&pdev->dev,"Failed to register GPIO PWC-%s %d\n",pdev->resource[i].name,ret);
			while(--i >= 0) {
				regulator_unregister(pwc[i].regulator);
			}
		}

		dev_dbg(&pdev->dev,"register regulator - %s , flags = %d\n",pdev->resource[i].name,pwc[i].flags != 0);
	}

	platform_set_drvdata(pdev, pwc);

	return 0;
}

static __devexit int gpio_pwc_remove(struct platform_device *pdev)
{
	int i;
	struct gpio_pwc *pwc = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	for(i=0;i<pdev->num_resources;i++) {
		regulator_unregister(pwc[i].regulator);
	}
	kfree(pwc);
	return 0;
}

static struct platform_driver gpio_pwc_driver = {
	.probe = gpio_pwc_probe,
	.remove = __devexit_p(gpio_pwc_remove),
	.driver		= {
		.name	= "gpio_pwc",
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_pwc_init(void)
{
	int ret;

	ret = platform_driver_register(&gpio_pwc_driver);
	if (ret != 0)
		pr_err("Failed to register GPIO PWC driver: %d\n", ret);

	return ret;
}
subsys_initcall(gpio_pwc_init);

static void __exit gpio_pwc_exit(void)
{
	platform_driver_unregister(&gpio_pwc_driver);
}
module_exit(gpio_pwc_exit);

MODULE_AUTHOR("ztyan<ztyan@ingenic.cn>");
MODULE_DESCRIPTION("ingenic power ctrl driver based on gpio");
MODULE_LICENSE("GPL");


