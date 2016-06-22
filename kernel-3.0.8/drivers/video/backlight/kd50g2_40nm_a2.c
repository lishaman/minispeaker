/*
 * kernel/drivers/video/panel/jz_kd50g2_40nm_a2.c -- Ingenic LCD panel device
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

//#include <linux/init.h>
//#include <linux/platform_device.h>
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

#include <linux/kd50g2_40nm_a2.h>
//static struct lcd_board_info *lcd_board;
//static struct lcd_soc_info *lcd_soc;
struct kd50g2_40nm_a2_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_kd50g2_40nm_a2_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void kd50g2_40nm_a2_on(struct kd50g2_40nm_a2_data *dev) {
	dev->lcd_power = 1;
	if(!IS_ERR(dev->lcd_vcc_reg)) {
		regulator_enable(dev->lcd_vcc_reg);
		}
	if (dev->pdata->gpio_lcd_disp) {
		/*
		gpio_direction_output(dev->pdata->gpio_lcd_disp, 0);
		*/
		mdelay(2);
		gpio_direction_output(dev->pdata->gpio_lcd_disp, 1);
	}
	if (dev->pdata->gpio_lcd_de) /* set data mode*/
		gpio_direction_output(dev->pdata->gpio_lcd_de, 0);

	if (dev->pdata->gpio_lcd_hsync) /*sync mode*/
		gpio_direction_output(dev->pdata->gpio_lcd_hsync, 1);
	if (dev->pdata->gpio_lcd_vsync) 
		gpio_direction_output(dev->pdata->gpio_lcd_vsync, 1);
}

static void kd50g2_40nm_a2_off(struct kd50g2_40nm_a2_data *dev)
{
	dev->lcd_power = 0;
	gpio_direction_output(dev->pdata->gpio_lcd_disp, 0);
	mdelay(2);
	if(regulator_is_enabled(dev->lcd_vcc_reg))
	regulator_disable(dev->lcd_vcc_reg);
	mdelay(10);
}

static int kd50g2_40nm_a2_set_power(struct lcd_device *lcd, int power)
{
	struct kd50g2_40nm_a2_data *dev = lcd_get_data(lcd);
	if (!power && !(dev->lcd_power)) {
		kd50g2_40nm_a2_on(dev);
	} else if (power&& (dev->lcd_power)) {
		kd50g2_40nm_a2_off(dev);
	}
	return 0;
}

static int kd50g2_40nm_a2_get_power(struct lcd_device *lcd)
{
	struct kd50g2_40nm_a2_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int kd50g2_40nm_a2_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops kd50g2_40nm_a2_ops = {
	.set_power = kd50g2_40nm_a2_set_power,
	.get_power = kd50g2_40nm_a2_get_power,
	.set_mode = kd50g2_40nm_a2_set_mode,
};

static int kd50g2_40nm_a2_probe(struct platform_device *pdev)
{
	/* check the parameters from lcd_driver */
	int ret;
	struct kd50g2_40nm_a2_data *dev;

	dev = kzalloc(sizeof(struct kd50g2_40nm_a2_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd_vcc_reg = regulator_get(NULL, "vlcd");
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

	kd50g2_40nm_a2_on(dev);

	dev->lcd = lcd_device_register("d50g2_40nm_a2on-lcd", &pdev->dev,
				       dev, &kd50g2_40nm_a2_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit kd50g2_40nm_a2_remove(struct platform_device *pdev)
{
	struct kd50g2_40nm_a2_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	kd50g2_40nm_a2_off(dev);

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
static int kd50g2_40nm_a2_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int kd50g2_40nm_a2_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define kd50g2_40nm_a2_suspend	NULL
#define kd50g2_40nm_a2_resume	NULL
#endif

static struct platform_driver kd50g2_40nm_a2_driver = {
	.driver		= {
		.name	= "kd50g2_40nm_a2-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= kd50g2_40nm_a2_probe,
	.remove		= kd50g2_40nm_a2_remove,
	.suspend	= kd50g2_40nm_a2_suspend,
	.resume		= kd50g2_40nm_a2_resume,
};

static int __init kd50g2_40nm_a2_init(void)
{
	// register the panel with lcd drivers
	return platform_driver_register(&kd50g2_40nm_a2_driver);;
}
module_init(kd50g2_40nm_a2_init);

static void __exit kd50g2_40nm_a2_exit(void)
{
	platform_driver_unregister(&kd50g2_40nm_a2_driver);
}
module_exit(kd50g2_40nm_a2_exit);

MODULE_DESCRIPTION("KD50G2_40NM_A2 lcd driver");
MODULE_LICENSE("GPL");
