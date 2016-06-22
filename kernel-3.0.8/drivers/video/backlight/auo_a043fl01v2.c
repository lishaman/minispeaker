/*
 *  LCD control code for AUO_A043FL01V2
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/fb.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct auo_a043fl01v2_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_auo_a043fl01v2_data *pdata;
};

static void auo_a043fl01v2_on(struct auo_a043fl01v2_data *dev)
{
}

static void auo_a043fl01v2_off(struct auo_a043fl01v2_data *dev)
{
}


static int auo_a043fl01v2_set_power(struct lcd_device *lcd, int power)
{
	struct auo_a043fl01v2_data *dev= lcd_get_data(lcd);

	if (POWER_IS_ON(power) && !POWER_IS_ON(dev->lcd_power))
		auo_a043fl01v2_on(dev);

	if (!POWER_IS_ON(power) && POWER_IS_ON(dev->lcd_power))
		auo_a043fl01v2_off(dev);

	dev->lcd_power = power;
	return 0;
}

static int auo_a043fl01v2_get_power(struct lcd_device *lcd)
{
	struct auo_a043fl01v2_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int auo_a043fl01v2_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops auo_a043fl01v2_ops = {
	.set_power = auo_a043fl01v2_set_power,
	.get_power = auo_a043fl01v2_get_power,
	.set_mode = auo_a043fl01v2_set_mode,
};

static int auo_a043fl01v2_probe(struct platform_device *pdev)
{
	struct auo_a043fl01v2_data *dev;

	dev = kzalloc(sizeof(struct auo_a043fl01v2_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	auo_a043fl01v2_on(dev);

	dev->lcd = lcd_device_register("auo_a043fl01v2-lcd", &pdev->dev, dev, &auo_a043fl01v2_ops);

	return 0;
}

static int auo_a043fl01v2_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int auo_a043fl01v2_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int auo_a043fl01v2_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define auo_a043fl01v2_suspend	NULL
#define auo_a043fl01v2_resume	NULL
#endif

static struct platform_driver auo_a043fl01v2_driver = {
	.driver		= {
		.name	= "auo_a043fl01v2-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= auo_a043fl01v2_probe,
	.remove		= auo_a043fl01v2_remove,
	.suspend	= auo_a043fl01v2_suspend,
	.resume		= auo_a043fl01v2_resume,
};

static int __init auo_a043fl01v2_init(void)
{
	return platform_driver_register(&auo_a043fl01v2_driver);
}
module_init(auo_a043fl01v2_init);

static void __exit auo_a043fl01v2_exit(void)
{
	platform_driver_unregister(&auo_a043fl01v2_driver);
}
module_exit(auo_a043fl01v2_exit);

MODULE_DESCRIPTION("AUO_A043FL01V2 lcd panel driver");
MODULE_LICENSE("GPL");
