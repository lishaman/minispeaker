/*
 *  LCD control code for KR070LA0S_270
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

#include <linux/kr070la0s_270.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct kr070la0s_270_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_kr070la0s_270_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void kr070la0s_270_on(struct kr070la0s_270_data *dev)
{
        dev->lcd_power = 1;
        regulator_enable(dev->lcd_vcc_reg);

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
	if (dev->pdata->gpio_selb) {
		if ( dev->pdata->six_bit_mode) {
			gpio_direction_output(dev->pdata->gpio_selb, 1);
		} else {
			gpio_direction_output(dev->pdata->gpio_selb, 0);
		}
	}
	if (dev->pdata->gpio_stbyb) {
		gpio_direction_output(dev->pdata->gpio_stbyb, 1);
	}
	if (dev->pdata->gpio_rest) {
		gpio_direction_output(dev->pdata->gpio_rest, 0);
		mdelay(100);
		gpio_direction_output(dev->pdata->gpio_rest, 1);
	}

	mdelay(80);
}

static void kr070la0s_270_off(struct kr070la0s_270_data *dev)
{
        dev->lcd_power = 0;
        regulator_disable(dev->lcd_vcc_reg);

	mdelay(30);
}

static int kr070la0s_270_set_power(struct lcd_device *lcd, int power)
{
	struct kr070la0s_270_data *dev= lcd_get_data(lcd);

	if (!power && !(dev->lcd_power)) {
                kr070la0s_270_on(dev);
        } else if (power && (dev->lcd_power)) {
                kr070la0s_270_off(dev);
        }
	return 0;
}

static int kr070la0s_270_get_power(struct lcd_device *lcd)
{
	struct kr070la0s_270_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int kr070la0s_270_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops kr070la0s_270_ops = {
	.set_power = kr070la0s_270_set_power,
	.get_power = kr070la0s_270_get_power,
	.set_mode = kr070la0s_270_set_mode,
};

static int __devinit kr070la0s_270_probe(struct platform_device *pdev)
{
	int ret;
	struct kr070la0s_270_data *dev;

	dev = kzalloc(sizeof(struct kr070la0s_270_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd_vcc_reg = regulator_get(NULL, "vlcd");
	if (IS_ERR(dev->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(dev->lcd_vcc_reg);
	}

	if (dev->pdata->gpio_lr)
		gpio_request(dev->pdata->gpio_lr, "left_to_right ");
	if (dev->pdata->gpio_ud)
		gpio_request(dev->pdata->gpio_ud, "up_to_down");
	if (dev->pdata->gpio_selb)
		gpio_request(dev->pdata->gpio_selb, "mode_select");
	if (dev->pdata->gpio_stbyb)
		gpio_request(dev->pdata->gpio_stbyb, "standby_mode");
	if (dev->pdata->gpio_rest)
		gpio_request(dev->pdata->gpio_rest, "reset");

	kr070la0s_270_on(dev);

	dev->lcd = lcd_device_register("kr070la0s_270-lcd", &pdev->dev,
				       dev, &kr070la0s_270_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit kr070la0s_270_remove(struct platform_device *pdev)
{
	struct kr070la0s_270_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	kr070la0s_270_off(dev);

	regulator_put(dev->lcd_vcc_reg);

	if (dev->pdata->gpio_lr)
		gpio_free(dev->pdata->gpio_lr);
	if (dev->pdata->gpio_ud)
		gpio_free(dev->pdata->gpio_ud);
	if (dev->pdata->gpio_selb)
		gpio_free(dev->pdata->gpio_selb);
	if (dev->pdata->gpio_stbyb)
		gpio_free(dev->pdata->gpio_stbyb);
	if (dev->pdata->gpio_rest)
		gpio_free(dev->pdata->gpio_rest);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int kr070la0s_270_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int kr070la0s_270_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define kr070la0s_270_suspend	NULL
#define kr070la0s_270_resume	NULL
#endif

static struct platform_driver kr070la0s_270_driver = {
	.driver		= {
		.name	= "kr070la0s_270-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= kr070la0s_270_probe,
	.remove		= kr070la0s_270_remove,
	.suspend	= kr070la0s_270_suspend,
	.resume		= kr070la0s_270_resume,
};

static int __init kr070la0s_270_init(void)
{
	return platform_driver_register(&kr070la0s_270_driver);
}
module_init(kr070la0s_270_init);

static void __exit kr070la0s_270_exit(void)
{
	platform_driver_unregister(&kr070la0s_270_driver);
}
module_exit(kr070la0s_270_exit);

MODULE_DESCRIPTION("KR070LA0S_270 lcd panel driver");
MODULE_LICENSE("GPL");

