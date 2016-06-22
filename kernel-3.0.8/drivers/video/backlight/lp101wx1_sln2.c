/*
 *  LCD control code for HSD101PWW1
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

#include <linux/lp101wx1_sln2.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

#define DRIVER_NAME "jz4770-lcd"

#define eprint(f, arg...)		pr_err(DRIVER_NAME ": " f , ## arg)
#define wprint(f, arg...)		pr_warning(DRIVER_NAME ": " f , ## arg)
#define kprint(f, arg...)		pr_info(DRIVER_NAME ": " f , ## arg)
#define dprint(f, arg...)		pr_debug(DRIVER_NAME ": " f , ## arg)

static int first_boot = 1;
struct lp101wx1_sln2_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_lp101wx1_sln2_data *pdata;
	struct regulator *lcd_vcc_reg;
	struct regulator *lcd_bklight_reg;
};

void enable_lcd_and_bklight(struct lp101wx1_sln2_data *dev)
{
	regulator_enable(dev->lcd_vcc_reg);
	msleep(300);
	regulator_enable(dev->lcd_bklight_reg);
	msleep(100);
}

void disable_lcd_and_bklight(struct lp101wx1_sln2_data *dev)
{
	regulator_disable(dev->lcd_bklight_reg);
	msleep(300);
	regulator_disable(dev->lcd_vcc_reg);
	msleep(100);
}

static void lp101wx1_sln2_on(struct lp101wx1_sln2_data *dev)
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

static void lp101wx1_sln2_off(struct lp101wx1_sln2_data *dev)
{
	dev->lcd_power = 0;

	disable_lcd_and_bklight(dev);
}

static int lp101wx1_sln2_set_power(struct lcd_device *lcd, int power)
{
	struct lp101wx1_sln2_data *dev= lcd_get_data(lcd);

	if (POWER_IS_ON(power) && !POWER_IS_ON(dev->lcd_power))
		lp101wx1_sln2_on(dev);

	if (!POWER_IS_ON(power) && POWER_IS_ON(dev->lcd_power))
		lp101wx1_sln2_off(dev);

	dev->lcd_power = power;

	return 0;
}

static int lp101wx1_sln2_get_power(struct lcd_device *lcd)
{
	struct lp101wx1_sln2_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int lp101wx1_sln2_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops lp101wx1_sln2_ops = {
	.set_power = lp101wx1_sln2_set_power,
	.get_power = lp101wx1_sln2_get_power,
	.set_mode = lp101wx1_sln2_set_mode,
};

static int __devinit lp101wx1_sln2_probe(struct platform_device *pdev)
{
	int ret;
	struct lp101wx1_sln2_data *dev;

	eprint("%s", __func__);

	dev = kzalloc(sizeof(struct lp101wx1_sln2_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd_vcc_reg = regulator_get(NULL, "vlcd");
	if (IS_ERR(dev->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(dev->lcd_vcc_reg);
	}

	dev->lcd_bklight_reg = regulator_get(NULL, "vbklight");
	if (IS_ERR(dev->lcd_bklight_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vbklight\n");
		return PTR_ERR(dev->lcd_bklight_reg);
	}

	if (dev->pdata->gpio_rest)
		gpio_request(dev->pdata->gpio_rest, "reset");

	dev->lcd = lcd_device_register("lp101wx1_sln2-lcd", &pdev->dev,
				       dev, &lp101wx1_sln2_ops);

	dev->lcd_power =FB_BLANK_UNBLANK;
	lp101wx1_sln2_set_power(dev->lcd, FB_BLANK_UNBLANK);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit lp101wx1_sln2_remove(struct platform_device *pdev)
{
	struct lp101wx1_sln2_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	lp101wx1_sln2_off(dev);

	regulator_put(dev->lcd_vcc_reg);
	regulator_put(dev->lcd_bklight_reg);

	if (dev->pdata->gpio_rest)
		gpio_free(dev->pdata->gpio_rest);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int lp101wx1_sln2_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int lp101wx1_sln2_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define lp101wx1_sln2_suspend	NULL
#define lp101wx1_sln2_resume	NULL
#endif

static struct platform_driver lp101wx1_sln2_driver = {
	.driver		= {
		.name	= "lp101wx1_sln2-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= lp101wx1_sln2_probe,
	.remove		= lp101wx1_sln2_remove,
	.suspend	= lp101wx1_sln2_suspend,
	.resume		= lp101wx1_sln2_resume,
};

static int __init lp101wx1_sln2_init(void)
{
	return platform_driver_register(&lp101wx1_sln2_driver);
}
module_init(lp101wx1_sln2_init);

static void __exit lp101wx1_sln2_exit(void)
{
	platform_driver_unregister(&lp101wx1_sln2_driver);
}
module_exit(lp101wx1_sln2_exit);

MODULE_DESCRIPTION("lp101wx1_sln2 lcd panel driver");
MODULE_LICENSE("GPL");

