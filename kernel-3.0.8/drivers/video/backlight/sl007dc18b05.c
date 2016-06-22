/*
 *  LCD control code for SL007DC18B05
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

#include <linux/sl007dc18b05.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct sl007dc18b05_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_sl007dc18b05_data *pdata;
	struct regulator *lcd_vcc_reg;
};

static void sl007dc18b05_on(struct sl007dc18b05_data *dev)
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

static void sl007dc18b05_off(struct sl007dc18b05_data *dev)
{
        dev->lcd_power = 0;
        while(regulator_is_enabled(dev->lcd_vcc_reg)) {
            regulator_disable(dev->lcd_vcc_reg);
            printk("%s----%d\n",__func__,__LINE__);
        }

	mdelay(30);
}

static int sl007dc18b05_set_power(struct lcd_device *lcd, int power)
{
	struct sl007dc18b05_data *dev= lcd_get_data(lcd);

	if (!power && !(dev->lcd_power)) {
                sl007dc18b05_on(dev);
        } else if (power && (dev->lcd_power)) {
                sl007dc18b05_off(dev);
        }
	return 0;
}

static int sl007dc18b05_get_power(struct lcd_device *lcd)
{
	struct sl007dc18b05_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int sl007dc18b05_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops sl007dc18b05_ops = {
	.set_power = sl007dc18b05_set_power,
	.get_power = sl007dc18b05_get_power,
	.set_mode = sl007dc18b05_set_mode,
};

static int __devinit sl007dc18b05_probe(struct platform_device *pdev)
{
	int ret;
	struct sl007dc18b05_data *dev;

	dev = kzalloc(sizeof(struct sl007dc18b05_data), GFP_KERNEL);
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

	sl007dc18b05_on(dev);

	printk("register lcd panel : sl007dc18b05-lcd\n");
	dev->lcd = lcd_device_register("sl007dc18b05-lcd", &pdev->dev,
				       dev, &sl007dc18b05_ops);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int __devinit sl007dc18b05_remove(struct platform_device *pdev)
{
	struct sl007dc18b05_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	sl007dc18b05_off(dev);

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
static int sl007dc18b05_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int sl007dc18b05_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define sl007dc18b05_suspend	NULL
#define sl007dc18b05_resume	NULL
#endif

static struct platform_driver sl007dc18b05_driver = {
	.driver		= {
		.name	= "sl007dc18b05-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= sl007dc18b05_probe,
	.remove		= sl007dc18b05_remove,
	.suspend	= sl007dc18b05_suspend,
	.resume		= sl007dc18b05_resume,
};

static int __init sl007dc18b05_init(void)
{
	return platform_driver_register(&sl007dc18b05_driver);
}
module_init(sl007dc18b05_init);

static void __exit sl007dc18b05_exit(void)
{
	platform_driver_unregister(&sl007dc18b05_driver);
}
module_exit(sl007dc18b05_exit);

MODULE_DESCRIPTION("SL007DC18B05 lcd panel driver");
MODULE_LICENSE("GPL");
