/*
 *  LCD control code for TM080TDH01
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

#include <linux/tm080tdh01.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

#define DRIVER_NAME "jz4780-lcd"

#define eprint(f, arg...)		pr_err(DRIVER_NAME ": " f , ## arg)
#define wprint(f, arg...)		pr_warning(DRIVER_NAME ": " f , ## arg)
#define kprint(f, arg...)		pr_info(DRIVER_NAME ": " f , ## arg)
#define dprint(f, arg...)		pr_debug(DRIVER_NAME ": " f , ## arg)

static int first_boot = 1;
struct tm080tdh01_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_tm080tdh01_data *pdata;
	struct regulator *lcd_vcc_reg;
	struct regulator *lcd_bklight_reg;
};

void enable_lcd_and_bklight(struct tm080tdh01_data *dev)
{
	regulator_enable(dev->lcd_vcc_reg);
	msleep(300);
	/* printk("^^^^^^^^enable_lcd_and_bklight\n"); */
	/* gpio_direction_output(4*32, 1);// */

	//regulator_enable(dev->lcd_bklight_reg);
	//msleep(100);
}

void disable_lcd_and_bklight(struct tm080tdh01_data *dev)
{
	//regulator_disable(dev->lcd_bklight_reg);
//	msleep(300);
	regulator_disable(dev->lcd_vcc_reg);
	/* printk("^^^^^^^^^^^^disable_lcd_and_bklight\n"); */
	/* gpio_direction_output(4*32, 0);// */
	msleep(100);
}

static void tm080tdh01_on(struct tm080tdh01_data *dev)
{
	//regulator_enable(dev->lcd_vcc_reg);
	enable_lcd_and_bklight(dev);
	if(!first_boot) {
		if (dev->pdata->gpio_rest) {
			gpio_direction_output(dev->pdata->gpio_rest, 0);
			msleep(100);
			gpio_direction_output(dev->pdata->gpio_rest, 1);
		    	first_boot = 0;
		}
	}
}

static void tm080tdh01_off(struct tm080tdh01_data *dev)
{
	dev->lcd_power = 0;

	disable_lcd_and_bklight(dev);
}

static int tm080tdh01_set_power(struct lcd_device *lcd, int power)
{
	struct tm080tdh01_data *dev= lcd_get_data(lcd);
	if (POWER_IS_ON(power)) 
		printk("POWER_IS_ON");
	if (!POWER_IS_ON(dev->lcd_power))
		printk("!POWER_IS_ON(dev->lcd_power)");
	if (POWER_IS_ON(power) && !POWER_IS_ON(dev->lcd_power))
	{
		tm080tdh01_on(dev);
	}

	if (!POWER_IS_ON(power) && POWER_IS_ON(dev->lcd_power))
		tm080tdh01_off(dev);

	dev->lcd_power = power;

	return 0;
}

static int tm080tdh01_get_power(struct lcd_device *lcd)
{
	struct tm080tdh01_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int tm080tdh01_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops tm080tdh01_ops = {
	.set_power = tm080tdh01_set_power,
	.get_power = tm080tdh01_get_power,
	.set_mode = tm080tdh01_set_mode,
};

static int __devinit tm080tdh01_probe(struct platform_device *pdev)
{
	int ret;
	struct tm080tdh01_data *dev;

	printk("==================%s, %d", __func__, __LINE__);

	dev = kzalloc(sizeof(struct tm080tdh01_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);
#if 1
	dev->lcd_vcc_reg = regulator_get(NULL, "vlcd");
	if (IS_ERR(dev->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(dev->lcd_vcc_reg);
	}
	printk("======================%s, %d", __func__, __LINE__);

	/* dev->lcd_bklight_reg = regulator_get(NULL, "vbklight"); */
	/* if (IS_ERR(dev->lcd_bklight_reg)) { */
	/* 	dev_err(&pdev->dev, "failed to get regulator vbklight\n"); */
	/* 	return PTR_ERR(dev->lcd_bklight_reg); */
	/* } */
#endif
	if (dev->pdata->gpio_rest)
		gpio_request(dev->pdata->gpio_rest, "reset");

	dev->lcd = lcd_device_register("tm080tdh01-lcd", &pdev->dev,
				       dev, &tm080tdh01_ops);

	dev->lcd_power =FB_BLANK_UNBLANK;
	tm080tdh01_set_power(dev->lcd, FB_BLANK_UNBLANK);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}
	printk("================%s, %d", __func__, __LINE__);

	return 0;
}

static int __devinit tm080tdh01_remove(struct platform_device *pdev)
{
	struct tm080tdh01_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	tm080tdh01_off(dev);

	regulator_put(dev->lcd_vcc_reg);
	regulator_put(dev->lcd_bklight_reg);

	if (dev->pdata->gpio_rest)
		gpio_free(dev->pdata->gpio_rest);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int tm080tdh01_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int tm080tdh01_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define tm080tdh01_suspend	NULL
#define tm080tdh01_resume	NULL
#endif

static struct platform_driver tm080tdh01_driver = {
	.driver		= {
		.name	= "tm080tdh01-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= tm080tdh01_probe,
	.remove		= tm080tdh01_remove,
	.suspend	= tm080tdh01_suspend,
	.resume		= tm080tdh01_resume,
};

static int __init tm080tdh01_init(void)
{
	return platform_driver_register(&tm080tdh01_driver);
}
module_init(tm080tdh01_init);

static void __exit tm080tdh01_exit(void)
{
	platform_driver_unregister(&tm080tdh01_driver);
}
module_exit(tm080tdh01_exit);

MODULE_DESCRIPTION("tm080tdh01 lcd panel driver");
MODULE_LICENSE("GPL");

