/*
 *  LCD / Backlight control code for JZLCD
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

#include <linux/jzlcd_pdata.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct jzlcd_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct jzlcd_platform_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void jzlcd_on(struct jzlcd_data *dev)
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

static void jzlcd_off(struct jzlcd_data *dev)
{
        dev->lcd_power = 0;
        regulator_disable(dev->lcd_vcc_reg);
	msleep(60);
}


static int jzlcd_set_power(struct lcd_device *lcd, int power)
{
	struct jzlcd_data *dev= lcd_get_data(lcd);

        if (!power && !(dev->lcd_power)) {
                jzlcd_on(dev);
        } else if (power && (dev->lcd_power)) {
                jzlcd_off(dev);
        }

	return 0;
}

static int jzlcd_get_power(struct lcd_device *lcd)
{
	struct jzlcd_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int jzlcd_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops jzlcd_ops = {
	.set_power = jzlcd_set_power,
	.get_power = jzlcd_get_power,
	.set_mode = jzlcd_set_mode,
};

static int __devinit jzlcd_probe(struct platform_device *pdev)
{
	int ret;
	struct jzlcd_data *dev;

	dev = kzalloc(sizeof(struct jzlcd_data), GFP_KERNEL);
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

	jzlcd_on(dev);

	dev->lcd = lcd_device_register("jzlcd-lcd", &pdev->dev,
				       dev, &jzlcd_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit jzlcd_remove(struct platform_device *pdev)
{
	struct jzlcd_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	jzlcd_off(dev);
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
static int jzlcd_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int jzlcd_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define jzlcd_suspend	NULL
#define jzlcd_resume	NULL
#endif

static struct platform_driver jzlcd_driver = {
	.driver		= {
		.name	= "jz-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= jzlcd_probe,
	.remove		= jzlcd_remove,
	.suspend	= jzlcd_suspend,
	.resume		= jzlcd_resume,
};

static int __init jzlcd_init(void)
{
	return platform_driver_register(&jzlcd_driver);
}
module_init(jzlcd_init);

static void __exit jzlcd_exit(void)
{
	platform_driver_unregister(&jzlcd_driver);
}
module_exit(jzlcd_exit);

MODULE_DESCRIPTION("JZLCD lcd panel driver");
MODULE_LICENSE("GPL");

