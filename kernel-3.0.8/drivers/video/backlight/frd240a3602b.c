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

struct frd240a3602b_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct lcd_platform_data *ctrl;
};

static int frd240a3602b_set_power(struct lcd_device *lcd, int power)
{
	struct frd240a3602b_data *dev= lcd_get_data(lcd);

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

static int frd240a3602b_get_power(struct lcd_device *lcd)
{
	struct frd240a3602b_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int frd240a3602b_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops frd240a3602b_ops = {
	.set_power = frd240a3602b_set_power,
	.get_power = frd240a3602b_get_power,
	.set_mode = frd240a3602b_set_mode,
};

static int __devinit frd240a3602b_probe(struct platform_device *pdev)
{
	int ret;
	struct frd240a3602b_data *dev;

	dev = kzalloc(sizeof(struct frd240a3602b_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->ctrl = pdev->dev.platform_data;
	if (dev->ctrl == NULL) {
		dev_info(&pdev->dev, "no platform data!");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd = lcd_device_register("frd240a3602b_slcd", &pdev->dev,
				       dev, &frd240a3602b_ops);
	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device(frd240a3602b) register success\n");
	}

	if (dev->ctrl->power_on) {
		dev->ctrl->power_on(dev->lcd, 1);
		dev->lcd_power = FB_BLANK_UNBLANK;
	}

	return 0;
}

static int __devinit frd240a3602b_remove(struct platform_device *pdev)
{
	struct frd240a3602b_data *dev = dev_get_drvdata(&pdev->dev);

	if (dev->lcd_power)
		dev->ctrl->power_on(dev->lcd, 0);

	lcd_device_unregister(dev->lcd);
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int frd240a3602b_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int frd240a3602b_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define frd240a3602b_suspend	NULL
#define frd240a3602b_resume	NULL
#endif

static struct platform_driver frd240a3602b_driver = {
	.driver		= {
		.name	= "frd240a3602b_slcd",
		.owner	= THIS_MODULE,
	},
	.probe		= frd240a3602b_probe,
	.remove		= frd240a3602b_remove,
	.suspend	= frd240a3602b_suspend,
	.resume		= frd240a3602b_resume,
};

static int __init frd240a3602b_init(void)
{
	return platform_driver_register(&frd240a3602b_driver);
}
rootfs_initcall(frd240a3602b_init);

static void __exit frd240a3602b_exit(void)
{
	platform_driver_unregister(&frd240a3602b_driver);
}
module_exit(frd240a3602b_exit);

MODULE_DESCRIPTION("frd240a3602b lcd panel driver");
MODULE_LICENSE("GPL");
