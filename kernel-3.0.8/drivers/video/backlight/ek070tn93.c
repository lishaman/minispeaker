/*
 *  LCD / Backlight control code for EK070TN93
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

#include <linux/ek070tn93.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct ek070tn93_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_ek070tn93_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void ek070tn93_on(struct ek070tn93_data *dev)
{
        dev->lcd_power = 1;
	regulator_enable(dev->lcd_vcc_reg);

	if (dev->pdata->de_mode) { /* DE mode */
		if (dev->pdata->gpio_mode)
			gpio_direction_output(dev->pdata->gpio_mode, 1);
		if (dev->pdata->gpio_vs)
			gpio_direction_output(dev->pdata->gpio_vs, 1);
		if (dev->pdata->gpio_hs)
			gpio_direction_output(dev->pdata->gpio_hs, 1);
	} else { /* SYNC mode */
		if (dev->pdata->gpio_mode)
			gpio_direction_output(dev->pdata->gpio_mode, 0);
		if (dev->pdata->gpio_de)
			gpio_direction_output(dev->pdata->gpio_de, 0);
	}
	if (dev->pdata->gpio_lr) {
		if (dev->pdata->left_to_right_scan) {
			gpio_direction_output(dev->pdata->gpio_lr, 1);
		} else {
			gpio_direction_output(dev->pdata->gpio_lr, 0);
		}
	}
	if (dev->pdata->gpio_ud) {
		if (dev->pdata->bottom_to_top_scan) {
			gpio_direction_output(dev->pdata->gpio_ud, 1);
		} else {
			gpio_direction_output(dev->pdata->gpio_ud, 0);
		}
	}
	if (dev->pdata->gpio_reset) {
		gpio_direction_output(dev->pdata->gpio_reset, 0);
		mdelay(2);
		gpio_direction_output(dev->pdata->gpio_reset, 1);
	}
	if (dev->pdata->gpio_dithb) {
		if (dev->pdata->dither_enable) {
			gpio_direction_output(dev->pdata->gpio_dithb, 0);
		} else {
			gpio_direction_output(dev->pdata->gpio_dithb, 1);
		}
	}

	msleep(60);
}

static void ek070tn93_off(struct ek070tn93_data *dev)
{
        dev->lcd_power = 0;
        regulator_disable(dev->lcd_vcc_reg);
	msleep(60);
}


static int ek070tn93_set_power(struct lcd_device *lcd, int power)
{
	struct ek070tn93_data *dev= lcd_get_data(lcd);

        if (!power && !(dev->lcd_power)) {
                ek070tn93_on(dev);
        } else if (power && (dev->lcd_power)) {
                ek070tn93_off(dev);
        }

	return 0;
}

static int ek070tn93_get_power(struct lcd_device *lcd)
{
	struct ek070tn93_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int ek070tn93_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops ek070tn93_ops = {
	.set_power = ek070tn93_set_power,
	.get_power = ek070tn93_get_power,
	.set_mode = ek070tn93_set_mode,
};

static int __devinit ek070tn93_probe(struct platform_device *pdev)
{
	int ret;
	struct ek070tn93_data *dev;

	dev = kzalloc(sizeof(struct ek070tn93_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd_vcc_reg = regulator_get(NULL, "vlcd");
	if (IS_ERR(dev->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(dev->lcd_vcc_reg);
	}

	if (dev->pdata->gpio_mode)
		gpio_request(dev->pdata->gpio_mode, "mode");
	if (dev->pdata->gpio_de)
		gpio_request(dev->pdata->gpio_de, "data enable");
	if (dev->pdata->gpio_vs)
		gpio_request(dev->pdata->gpio_vs, "vsync");
	if (dev->pdata->gpio_hs)
		gpio_request(dev->pdata->gpio_hs, "hsync");
	if (dev->pdata->gpio_lr)
		gpio_request(dev->pdata->gpio_lr, "left_to_right ");
	if (dev->pdata->gpio_ud)
		gpio_request(dev->pdata->gpio_ud, "up_to_down");
	if (dev->pdata->gpio_reset)
		gpio_request(dev->pdata->gpio_reset, "reset");
	if (dev->pdata->gpio_dithb)
		gpio_request(dev->pdata->gpio_dithb, "dither");

	ek070tn93_on(dev);

	dev->lcd = lcd_device_register("ek070tn93-lcd", &pdev->dev,
				       dev, &ek070tn93_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit ek070tn93_remove(struct platform_device *pdev)
{
	struct ek070tn93_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	ek070tn93_off(dev);
	regulator_put(dev->lcd_vcc_reg);

	if (dev->pdata->gpio_mode)
		gpio_free(dev->pdata->gpio_mode);
	if (dev->pdata->gpio_de)
		gpio_free(dev->pdata->gpio_de);
	if (dev->pdata->gpio_vs)
		gpio_free(dev->pdata->gpio_vs);
	if (dev->pdata->gpio_hs)
		gpio_free(dev->pdata->gpio_hs);
	if (dev->pdata->gpio_lr)
		gpio_free(dev->pdata->gpio_lr);
	if (dev->pdata->gpio_ud)
		gpio_free(dev->pdata->gpio_ud);
	if (dev->pdata->gpio_reset)
		gpio_free(dev->pdata->gpio_reset);
	if (dev->pdata->gpio_dithb)
		gpio_free(dev->pdata->gpio_dithb);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int ek070tn93_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int ek070tn93_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define ek070tn93_suspend	NULL
#define ek070tn93_resume	NULL
#endif

static struct platform_driver ek070tn93_driver = {
	.driver		= {
		.name	= "ek070tn93-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= ek070tn93_probe,
	.remove		= ek070tn93_remove,
	.suspend	= ek070tn93_suspend,
	.resume		= ek070tn93_resume,
};

static int __init ek070tn93_init(void)
{
	return platform_driver_register(&ek070tn93_driver);
}
module_init(ek070tn93_init);

static void __exit ek070tn93_exit(void)
{
	platform_driver_unregister(&ek070tn93_driver);
}
module_exit(ek070tn93_exit);

MODULE_DESCRIPTION("EK070TN93 lcd panel driver");
MODULE_LICENSE("GPL");

