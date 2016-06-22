/* drivers/mfd/jz_adc.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *      http://www.ingenic.com
 *      Sun Jiwei<jwsun@ingenic.cn>
 * JZ4780 SOC ADC device core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This driver is designed to control the usage of the ADC block between
 * the touchscreen and any other drivers that may need to use it, such as
 * the hwmon driver.
 */

#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/delay.h>
#include <linux/mfd/core.h>

#include <linux/mfd/axp173-private.h>
#include <linux/jz-axp173-adc.h>
#include <linux/mfd/pmu-common.h>

struct axp173_adc {
	struct device	*dev;
};

static struct mfd_cell axp173_adc_cells[] = {
	{
		.id 	= 1,
                .name 	= "jz-current-battery",
	},
};

static int __devinit axp173_adc_probe(struct platform_device *pdev)
{
	int ret;
	int i;
	struct axp173_adc *adc;
	struct jz_current_battery_info *battery_info;
	struct axp173_adc_platform_data *pdata = pdev->dev.platform_data;
	if (!pdata)
	{
		dev_err(&pdev->dev,"no platform data,can't attach\n");
		return -EINVAL;
	}
	battery_info = pdata->battery_info;
	adc = kmalloc(sizeof(*adc), GFP_KERNEL);
	if (!adc) {
		dev_err(&pdev->dev, "Failed to allocate driver structre\n");
		return -ENOMEM;
	}

	adc->dev = &pdev->dev;

	platform_set_drvdata(pdev, adc);

	for(i = 0;i < ARRAY_SIZE(axp173_adc_cells);i++)
	{
		switch(axp173_adc_cells[i].id)
		{
			case 1:
				axp173_adc_cells[i].platform_data = battery_info;
				break;
			case 2:
				break;
			default:
				break;
		}
	}
	ret = mfd_add_devices(&pdev->dev, 0, axp173_adc_cells,
			ARRAY_SIZE(axp173_adc_cells), NULL, 0);
	if (ret < 0) {
		goto err_free;
	}

	return 0;
err_free:
        kfree(adc);
}

static int __devexit axp173_adc_remove(struct platform_device *pdev)
{
	struct axp173_adc *adc = platform_get_drvdata(pdev);

	mfd_remove_devices(&pdev->dev);

	platform_set_drvdata(pdev, NULL);

	kfree(adc);

	return 0;
}

struct platform_driver axp173_adc_driver = {
	.probe	= axp173_adc_probe,
	.remove	= __devexit_p(axp173_adc_remove),
	.driver = {
		.name	= "axp173-adc",
		.owner	= THIS_MODULE,
	},
};

static int __init axp173_adc_init(void)
{
	return platform_driver_register(&axp173_adc_driver);
}
module_init(axp173_adc_init);

static void __exit axp173_adc_exit(void)
{
	platform_driver_unregister(&axp173_adc_driver);
}
module_exit(axp173_adc_exit);

MODULE_DESCRIPTION("axp173 SOC ADC driver");
MODULE_AUTHOR("Hu Xiaohui<xiaohui.hu@ingenic.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:axp173-adc");
