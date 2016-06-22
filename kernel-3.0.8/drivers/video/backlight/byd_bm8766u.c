/*
 * kernel/drivers/video/panel/jz_byd_bm8766u.c -- Ingenic LCD panel device
 *
 * Copyright (C) 2005-2010, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
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
#include <soc/gpio.h>

#include <linux/byd_bm8766u.h>

struct byd_bm8766u_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_byd_bm8766u_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void byd_bm8766u_on(struct byd_bm8766u_data *dev) {
	dev->lcd_power = 1;
	regulator_enable(dev->lcd_vcc_reg);
	if (dev->pdata->gpio_lcd_disp)
		gpio_direction_output(dev->pdata->gpio_lcd_disp, 1);
	if (dev->pdata->gpio_lcd_de) /* set data mode*/
		gpio_direction_output(dev->pdata->gpio_lcd_de, 0);

	if (dev->pdata->gpio_lcd_hsync) /*sync mode*/
		gpio_direction_output(dev->pdata->gpio_lcd_hsync, 1);
	if (dev->pdata->gpio_lcd_vsync)
		gpio_direction_output(dev->pdata->gpio_lcd_vsync, 1);
}

static void byd_bm8766u_off(struct byd_bm8766u_data *dev)
{
	dev->lcd_power = 0;
	if (dev->pdata->gpio_lcd_disp)
		gpio_direction_output(dev->pdata->gpio_lcd_disp, 0);
	mdelay(2);
	regulator_disable(dev->lcd_vcc_reg);
	mdelay(10);
}

static int byd_bm8766u_set_power(struct lcd_device *lcd, int power)
{
	struct byd_bm8766u_data *dev = lcd_get_data(lcd);

	if (!power && !(dev->lcd_power)) {
                byd_bm8766u_on(dev);
        } else if (power && (dev->lcd_power)) {
                byd_bm8766u_off(dev);
        }
	return 0;
}

static int byd_bm8766u_get_power(struct lcd_device *lcd)
{
	struct byd_bm8766u_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int byd_bm8766u_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops byd_bm8766u_ops = {
	.set_power = byd_bm8766u_set_power,
	.get_power = byd_bm8766u_get_power,
	.set_mode = byd_bm8766u_set_mode,
};

static int byd_bm8766u_probe(struct platform_device *pdev)
{
	/* check the parameters from lcd_driver */
	int ret;
	struct byd_bm8766u_data *dev;

	dev = kzalloc(sizeof(struct byd_bm8766u_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd_vcc_reg = regulator_get(NULL, "lcd_1v8");
	if (IS_ERR(dev->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(dev->lcd_vcc_reg);
	}

	if (dev->pdata->gpio_lcd_disp)
		gpio_request(dev->pdata->gpio_lcd_disp, "display on");
	if (dev->pdata->gpio_lcd_de)
		gpio_request(dev->pdata->gpio_lcd_de, "data enable");
	if (dev->pdata->gpio_lcd_hsync)
		gpio_request(dev->pdata->gpio_lcd_hsync, "hsync");
	if (dev->pdata->gpio_lcd_vsync)
		gpio_request(dev->pdata->gpio_lcd_vsync, "vsync");

	byd_bm8766u_on(dev);

	dev->lcd = lcd_device_register("byd_bm8766u-lcd", &pdev->dev,
				       dev, &byd_bm8766u_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit byd_bm8766u_remove(struct platform_device *pdev)
{
	struct byd_bm8766u_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	byd_bm8766u_off(dev);

	regulator_put(dev->lcd_vcc_reg);

	if (dev->pdata->gpio_lcd_disp)
		gpio_free(dev->pdata->gpio_lcd_disp);
	if (dev->pdata->gpio_lcd_de)
		gpio_free(dev->pdata->gpio_lcd_de);
	if (dev->pdata->gpio_lcd_hsync)
		gpio_free(dev->pdata->gpio_lcd_hsync);
	if (dev->pdata->gpio_lcd_vsync)
		gpio_free(dev->pdata->gpio_lcd_vsync);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int byd_bm8766u_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int byd_bm8766u_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define byd_bm8766u_suspend	NULL
#define byd_bm8766u_resume	NULL
#endif

static struct platform_driver byd_bm8766u_driver = {
	.driver		= {
		.name	= "byd_bm8766u-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= byd_bm8766u_probe,
	.remove		= byd_bm8766u_remove,
	.suspend	= byd_bm8766u_suspend,
	.resume		= byd_bm8766u_resume,
};

static int __init byd_bm8766u_init(void)
{
	// register the panel with lcd drivers
	return platform_driver_register(&byd_bm8766u_driver);;
}
module_init(byd_bm8766u_init);

static void __exit byd_bm8766u_exit(void)
{
	platform_driver_unregister(&byd_bm8766u_driver);
}
module_exit(byd_bm8766u_exit);

MODULE_DESCRIPTION("BYD_BM8766U lcd driver");
MODULE_LICENSE("GPL");
