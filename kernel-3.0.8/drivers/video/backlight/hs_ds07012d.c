/*
 * kernel/drivers/video/panel/jz_hs_ds07012d.c -- Ingenic LCD panel device
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

#include <linux/hs_ds07012d.h>

//static struct lcd_board_info *lcd_board;
//static struct lcd_soc_info *lcd_soc;
struct hs_ds07012d_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_hs_ds07012d_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void hs_ds07012d_on(struct hs_ds07012d_data *dev) {
	dev->lcd_power = 1;
	regulator_enable(dev->lcd_vcc_reg);
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

static void hs_ds07012d_off(struct hs_ds07012d_data *dev)
{
	dev->lcd_power = 0;
	gpio_direction_output(dev->pdata->gpio_lcd_disp, 0);
	mdelay(2);
	regulator_disable(dev->lcd_vcc_reg);
	mdelay(10);
}

static int hs_ds07012d_set_power(struct lcd_device *lcd, int power)
{
	struct hs_ds07012d_data *dev = lcd_get_data(lcd);

	if (!power && !(dev->lcd_power)) {
                hs_ds07012d_on(dev);
        } else if (power && (dev->lcd_power)) {
                hs_ds07012d_off(dev);
        }
	return 0;
}

static int hs_ds07012d_get_power(struct lcd_device *lcd)
{
	struct hs_ds07012d_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int hs_ds07012d_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops hs_ds07012d_ops = {
	.set_power = hs_ds07012d_set_power,
	.get_power = hs_ds07012d_get_power,
	.set_mode = hs_ds07012d_set_mode,
};

static int hs_ds07012d_probe(struct platform_device *pdev)
{
	/* check the parameters from lcd_driver */
	int ret;
	struct hs_ds07012d_data *dev;

	dev = kzalloc(sizeof(struct hs_ds07012d_data), GFP_KERNEL);
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

	hs_ds07012d_on(dev);

	dev->lcd = lcd_device_register("d50g2_40nm_a2on-lcd", &pdev->dev,
				       dev, &hs_ds07012d_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit hs_ds07012d_remove(struct platform_device *pdev)
{
	struct hs_ds07012d_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	hs_ds07012d_off(dev);

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
static int hs_ds07012d_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int hs_ds07012d_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define hs_ds07012d_suspend	NULL
#define hs_ds07012d_resume	NULL
#endif

static struct platform_driver hs_ds07012d_driver = {
	.driver		= {
		.name	= "hs_ds07012d-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= hs_ds07012d_probe,
	.remove		= hs_ds07012d_remove,
	.suspend	= hs_ds07012d_suspend,
	.resume		= hs_ds07012d_resume,
};

static int __init hs_ds07012d_init(void)
{
	// register the panel with lcd drivers
	return platform_driver_register(&hs_ds07012d_driver);;
}
module_init(hs_ds07012d_init);

static void __exit hs_ds07012d_exit(void)
{
	platform_driver_unregister(&hs_ds07012d_driver);
}
module_exit(hs_ds07012d_exit);

MODULE_DESCRIPTION("HS_DS07012D lcd driver");
MODULE_LICENSE("GPL");
