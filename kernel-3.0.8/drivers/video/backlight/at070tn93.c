/*
 *  LCD / Backlight control code for AT070TN93
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

struct platform_at070tn93_data {
	int gpio_power;
	int gpio_vsync;
	int gpio_hsync;
	int gpio_reset;
};

struct at070tn93_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_at070tn93_data *pdata;
};

static void at070tn93_on(struct at070tn93_data *dev)
{
	gpio_direction_output(dev->pdata->gpio_power,1);
	msleep(30);

	gpio_direction_output(dev->pdata->gpio_vsync,1);
	gpio_direction_output(dev->pdata->gpio_hsync,1);
}

static void at070tn93_off(struct at070tn93_data *dev)
{
	gpio_direction_output(dev->pdata->gpio_power,0);
}


static int at070tn93_set_power(struct lcd_device *lcd, int power)
{
	struct at070tn93_data *dev= lcd_get_data(lcd);

	if (POWER_IS_ON(power) && !POWER_IS_ON(dev->lcd_power))
		at070tn93_on(dev);

	if (!POWER_IS_ON(power) && POWER_IS_ON(dev->lcd_power))
		at070tn93_off(dev);

	dev->lcd_power = power;
	return 0;
}

static int at070tn93_get_power(struct lcd_device *lcd)
{
	struct at070tn93_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int at070tn93_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops at070tn93_ops = {
	.set_power = at070tn93_set_power,
	.get_power = at070tn93_get_power,
	.set_mode = at070tn93_set_mode,
};

static int at070tn93_probe(struct platform_device *pdev)
{
	struct at070tn93_data *dev;

	dev = kzalloc(sizeof(struct at070tn93_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	gpio_request(dev->pdata->gpio_power, "power");
	gpio_request(dev->pdata->gpio_vsync, "vsync");
	gpio_request(dev->pdata->gpio_hsync, "hsync");
	gpio_request(dev->pdata->gpio_reset, "reset");

	at070tn93_on(dev);

	dev->lcd = lcd_device_register("at070tn93-lcd", &pdev->dev, dev, &at070tn93_ops);

	return 0;
}

static int at070tn93_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int at070tn93_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int at070tn93_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define at070tn93_suspend	NULL
#define at070tn93_resume	NULL
#endif

static struct platform_driver at070tn93_driver = {
	.driver		= {
		.name	= "at070tn93-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= at070tn93_probe,
	.remove		= at070tn93_remove,
	.suspend	= at070tn93_suspend,
	.resume		= at070tn93_resume,
};

static int __init at070tn93_init(void)
{
	return platform_driver_register(&at070tn93_driver);
}
module_init(at070tn93_init);

static void __exit at070tn93_exit(void)
{
	platform_driver_unregister(&at070tn93_driver);
}
module_exit(at070tn93_exit);

MODULE_DESCRIPTION("AT070TN93 lcd panel driver");
MODULE_LICENSE("GPL");

