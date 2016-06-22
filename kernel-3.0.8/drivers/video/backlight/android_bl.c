/*
 *  android bl no flash
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
#include <linux/earlysuspend.h>

#include <linux/android_bl.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct android_bl_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct android_bl_platform_data *pdata;
	struct regulator *lcd_vcc_reg;
	struct regulator *lcd_bklight_reg;

	#ifdef CONFIG_HAS_EARLYSUSPEND
		struct early_suspend bk_early_suspend;
	#endif
};

static void android_bl_on(struct android_bl_data *dev) {
	static int is_bootup = 1;

	if (is_bootup && !dev->pdata->bootloader_unblank) {
		if (dev->lcd_bklight_reg)
			regulator_disable(dev->lcd_bklight_reg);

		regulator_enable(dev->lcd_vcc_reg);

		if (dev->pdata->gpio_reset) {
			gpio_direction_output(dev->pdata->gpio_reset, 0);
			mdelay(dev->pdata->delay_reset);
			gpio_direction_output(dev->pdata->gpio_reset, 1);
		}

		mdelay(dev->pdata->delay_before_bkon);
		if (dev->lcd_bklight_reg)
			regulator_enable(dev->lcd_bklight_reg);

	}

	if (is_bootup) {
		if (dev->lcd_bklight_reg &&
				!regulator_is_enabled(dev->lcd_bklight_reg))
			regulator_enable(dev->lcd_bklight_reg);

		if (!regulator_is_enabled(dev->lcd_vcc_reg))
			regulator_enable(dev->lcd_vcc_reg);

		if (dev->pdata->notify_on)
			dev->pdata->notify_on(1);

		is_bootup = 0;
	}
}

static void android_bl_off(struct android_bl_data *dev) {
}

static int android_bl_set_power(struct lcd_device *lcd, int power) {
	struct android_bl_data *dev = lcd_get_data(lcd);

	if (POWER_IS_ON(power) && !POWER_IS_ON(dev->lcd_power))
		android_bl_on(dev);

	if (!POWER_IS_ON(power) && POWER_IS_ON(dev->lcd_power))
		android_bl_off(dev);

	dev->lcd_power = power;

	return 0;
}

static int android_bl_get_power(struct lcd_device *lcd) {
	struct android_bl_data *dev = lcd_get_data(lcd);

	return dev->lcd_power;
}

static int android_bl_set_mode(struct lcd_device *lcd,
	struct fb_videomode *mode) {
	return 0;
}

static struct lcd_ops android_bl_ops = {
		.set_power = android_bl_set_power,
		.get_power = android_bl_get_power,
		.set_mode = android_bl_set_mode,
};


#ifdef CONFIG_HAS_EARLYSUSPEND

static void bk_e_suspend(struct early_suspend *h)
{
	struct android_bl_data *dev = container_of(h, struct android_bl_data, bk_early_suspend);
	if (dev->pdata->notify_on)
		dev->pdata->notify_on(0);

	if (dev->lcd_bklight_reg)
		regulator_disable(dev->lcd_bklight_reg);

	regulator_disable(dev->lcd_vcc_reg);
}

static void bk_l_resume(struct early_suspend *h)
{
	struct android_bl_data *dev = container_of(h, struct android_bl_data, bk_early_suspend);

	regulator_enable(dev->lcd_vcc_reg);

	if (dev->pdata->gpio_reset) {
		gpio_direction_output(dev->pdata->gpio_reset, 0);
		mdelay(dev->pdata->delay_reset);
		gpio_direction_output(dev->pdata->gpio_reset, 1);
	}

	mdelay(dev->pdata->delay_before_bkon);
	if (dev->lcd_bklight_reg)
		regulator_enable(dev->lcd_bklight_reg);

	if (dev->pdata->notify_on)
		dev->pdata->notify_on(1);
}

#endif

static int __devinit android_bl_probe(struct platform_device *pdev)
{
	int ret;
	struct android_bl_data *dev;

	dev = kzalloc(sizeof(struct android_bl_data), GFP_KERNEL);
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
		dev->lcd_bklight_reg = NULL;
	}

	if (dev->pdata->gpio_reset)
		gpio_request(dev->pdata->gpio_reset, "reset");

	dev->lcd = lcd_device_register("android_bl-lcd", &pdev->dev,
				       dev, &android_bl_ops);

	dev->lcd_power = FB_BLANK_POWERDOWN;
	android_bl_set_power(dev->lcd, FB_BLANK_UNBLANK);

	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	dev->bk_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	dev->bk_early_suspend.suspend = bk_e_suspend;
	dev->bk_early_suspend.resume = bk_l_resume;
	register_early_suspend(&dev->bk_early_suspend);
#endif

	return 0;
}

static int __devinit android_bl_remove(struct platform_device *pdev)
{
	struct android_bl_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	android_bl_off(dev);

	regulator_put(dev->lcd_vcc_reg);
	if (dev->lcd_bklight_reg)
		regulator_put(dev->lcd_bklight_reg);

	if (dev->pdata->gpio_reset)
		gpio_free(dev->pdata->gpio_reset);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&dev->bk_early_suspend);
#endif

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM

static int android_bl_suspend(struct platform_device *pdev,
pm_message_t state)
{
	return 0;
}

static int android_bl_resume(struct platform_device *pdev)
{
	return 0;
}

#else

#define android_bl_suspend	NULL
#define android_bl_resume	NULL

#endif

static struct platform_driver android_bl_driver =
{
	.driver = {
		.name = "android-bl",
		.owner = THIS_MODULE ,
	},

	.probe = android_bl_probe,
	.remove = android_bl_remove,
	.suspend = android_bl_suspend,
	.resume = android_bl_resume
};

static int __init android_bl_init(void)
{
	return platform_driver_register(&android_bl_driver);
}
module_init(android_bl_init);

static void __exit android_bl_exit(void)
{
	platform_driver_unregister(&android_bl_driver);
}
module_exit(android_bl_exit);

MODULE_DESCRIPTION("Generic lcd panel driver");
MODULE_LICENSE("GPL");

