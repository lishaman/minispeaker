/*
 *  LCD control code for truly
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
#include <linux/regulator/consumer.h>

struct xrm2002903_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct lcd_platform_data *ctrl;
};

static int xrm2002903_set_power(struct lcd_device *lcd, int power)
{
	struct xrm2002903_data *dev= lcd_get_data(lcd);

	if (!power && dev->lcd_power) {
		dev->ctrl->power_on(lcd, 1);
	} else if (power && !dev->lcd_power) {
		if (dev->ctrl->reset) {
			dev->ctrl->reset(lcd);
		}
		dev->ctrl->power_on(lcd, 0);
	}
	dev->lcd_power = power;
	return 0;
}

static int xrm2002903_get_power(struct lcd_device *lcd)
{
	struct xrm2002903_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int xrm2002903_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops xrm2002903_ops = {
	.set_power = xrm2002903_set_power,
	.get_power = xrm2002903_get_power,
	.set_mode = xrm2002903_set_mode,
};

static int __devinit xrm2002903_probe(struct platform_device *pdev)
{
	int ret;
	struct xrm2002903_data *dev;

	dev = kzalloc(sizeof(struct xrm2002903_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->ctrl = pdev->dev.platform_data;
	if (dev->ctrl == NULL) {
		dev_info(&pdev->dev, "no platform data!");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd = lcd_device_register("xrm2002903_slcd", &pdev->dev,
				       dev, &xrm2002903_ops);
	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device(XRM2002903) register success\n");
	}

	if (dev->ctrl->power_on) {
		dev->ctrl->power_on(dev->lcd, 1);
		dev->lcd_power = FB_BLANK_UNBLANK;
	}

	return 0;
}

static int __devinit xrm2002903_remove(struct platform_device *pdev)
{
	struct xrm2002903_data *dev = dev_get_drvdata(&pdev->dev);

	if (dev->lcd_power)
		dev->ctrl->power_on(dev->lcd, 0);

	lcd_device_unregister(dev->lcd);
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int xrm2002903_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int xrm2002903_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define xrm2002903_suspend	NULL
#define xrm2002903_resume	NULL
#endif

static struct platform_driver xrm2002903_driver = {
	.driver		= {
		.name	= "xrm2002903_slcd",
		.owner	= THIS_MODULE,
	},
	.probe		= xrm2002903_probe,
	.remove		= xrm2002903_remove,
	.suspend	= xrm2002903_suspend,
	.resume		= xrm2002903_resume,
};

static int __init xrm2002903_init(void)
{
	return platform_driver_register(&xrm2002903_driver);
}
rootfs_initcall(xrm2002903_init);

static void __exit xrm2002903_exit(void)
{
	platform_driver_unregister(&xrm2002903_driver);
}
module_exit(xrm2002903_exit);

MODULE_DESCRIPTION("XRM2002903 lcd panel driver");
MODULE_LICENSE("GPL");
