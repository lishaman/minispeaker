/*
 * linux/drivers/video/backlight/digital_pulse_bl.c
 *
 * simple digital pulse based backlight control, board code has to setup
 * 1) pin configuration so digital pulse waveforms can output
 * 2) platform_data being correctly configured
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/digital_pulse_backlight.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/gpio.h>

struct digital_pulse_bl_data {
	struct device *dev;
	unsigned int convert_factor;
	unsigned int current_brightness;
	unsigned int pulse_num;
	struct platform_digital_pulse_backlight_data *pdata;
	unsigned int suspend;
	struct mutex digital_pulse_lock;
	struct notifier_block nb;
	int	(*notify)(struct device *, int brightness);
	int	(*check_fb)(struct device *, struct fb_info *);
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend bk_early_suspend;
#endif
};

static void send_low_pulse(struct backlight_device *bl)
{
	unsigned int i;
	struct digital_pulse_bl_data *pb = dev_get_drvdata(&bl->dev);

	for (i = pb->pulse_num; i > 0; i--)	{
		gpio_direction_output(pb->pdata->digital_pulse_gpio, 0);
		udelay(pb->pdata->low_level_delay_us);
		gpio_direction_output(pb->pdata->digital_pulse_gpio, 1);
		udelay(pb->pdata->high_level_delay_us);
	}
	gpio_direction_output(pb->pdata->digital_pulse_gpio, 1);
}

static void init_backlight(struct backlight_device *bl)
{
	struct digital_pulse_bl_data *pb = dev_get_drvdata(&bl->dev);
	pb->pulse_num = pb->pdata->dft_brightness / pb->convert_factor + 1;

	send_low_pulse(bl);
}

static void set_backlight_level(struct backlight_device *bl)
{
	struct digital_pulse_bl_data *pb = dev_get_drvdata(&bl->dev);
	unsigned int last = pb->current_brightness / pb->convert_factor + 1;
	unsigned int tmp = bl->props.brightness / pb->convert_factor + 1;

	if (tmp <= last) {
		pb->pulse_num = last - tmp;
	} else {
		pb->pulse_num = last + pb->pdata->max_brightness - tmp;
	}
	send_low_pulse(bl);
}

static void close_backlight(struct backlight_device *bl)
{
	struct digital_pulse_bl_data *pb = dev_get_drvdata(&bl->dev);
	gpio_direction_output(pb->pdata->digital_pulse_gpio, 0);
}

static int digital_pulse_backlight_update_status(struct backlight_device *bl)
{
	struct digital_pulse_bl_data *pb = dev_get_drvdata(&bl->dev);
	int brightness = bl->props.brightness;

	mutex_lock(&pb->digital_pulse_lock);
	if (bl->props.power != FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;

	if (pb->notify)
		brightness = pb->notify(pb->dev, brightness);

	if (pb->suspend && (brightness > pb->current_brightness)) {
		pb->current_brightness = brightness;
		mutex_unlock(&pb->digital_pulse_lock);

		return 0;
	}

	pb->current_brightness = brightness;
	if (brightness == 0) {
		close_backlight(bl);
		mdelay(50);
	} else {
		set_backlight_level(bl);
	}
	mutex_unlock(&pb->digital_pulse_lock);

	return 0;
}

static int digital_pulse_backlight_get_brightness(struct backlight_device *bl)
{
	return bl->props.brightness;
}

static const struct backlight_ops digital_pulse_backlight_ops = {
	.update_status	= digital_pulse_backlight_update_status,
	.get_brightness	= digital_pulse_backlight_get_brightness,
};

static int digital_pulse_bl_shutdown_notify(struct notifier_block *rnb,
		unsigned long unused2, void *unused3)
{
	struct digital_pulse_bl_data *pb = container_of(rnb,
			struct digital_pulse_bl_data, nb);

	gpio_direction_output(pb->pdata->digital_pulse_gpio, 0);

	return NOTIFY_DONE;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bk_e_suspend(struct early_suspend *h)
{
	struct digital_pulse_bl_data *pb = container_of(h,
			struct digital_pulse_bl_data, bk_early_suspend);

	pb->suspend = 1;
	gpio_direction_output(pb->pdata->digital_pulse_gpio, 0);
}

static void bk_l_resume(struct early_suspend *h)
{
	int brightness;
	unsigned int i;
	struct digital_pulse_bl_data *pb = container_of(h,
			struct digital_pulse_bl_data, bk_early_suspend);

	pb->suspend = 0;
	mutex_lock(&pb->digital_pulse_lock);
	brightness = pb->current_brightness;
	pb->pulse_num = brightness / pb->convert_factor + 1;

	gpio_direction_output(pb->pdata->digital_pulse_gpio, 1);
	udelay(30);
	for (i = pb->pulse_num; i > 0; i--)	{
		gpio_direction_output(pb->pdata->digital_pulse_gpio, 0);
		udelay(pb->pdata->low_level_delay_us);
		gpio_direction_output(pb->pdata->digital_pulse_gpio, 1);
		udelay(pb->pdata->high_level_delay_us);
	}
	mutex_unlock(&pb->digital_pulse_lock);
}
#endif

static int digital_pulse_backlight_probe(struct platform_device *pdev)
{
	struct backlight_properties props;
	struct backlight_device *bl;
	struct digital_pulse_bl_data *dp_bl;
	int ret;

	dp_bl = kzalloc(sizeof(*dp_bl), GFP_KERNEL);
	if (!dp_bl) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	dp_bl->pdata = pdev->dev.platform_data;
	if (!dp_bl->pdata) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	if (dp_bl->pdata->init) {
		ret = dp_bl->pdata->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}
	dp_bl->notify = dp_bl->pdata->notify;
	dp_bl->check_fb = dp_bl->pdata->check_fb;
	dp_bl->dev = &pdev->dev;
	dp_bl->convert_factor = dp_bl->pdata->max_brightness / dp_bl->pdata->max_brightness_step;

	if (dp_bl->pdata->digital_pulse_gpio)
		gpio_request(dp_bl->pdata->digital_pulse_gpio, "digital_pulse");

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = dp_bl->pdata->max_brightness;
	bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, dp_bl,
			&digital_pulse_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_bl;
	}

	mutex_init(&dp_bl->digital_pulse_lock);

	bl->props.brightness = dp_bl->pdata->dft_brightness;
	init_backlight(bl);

	platform_set_drvdata(pdev, bl);

	dp_bl->nb.notifier_call = digital_pulse_bl_shutdown_notify;
	register_reboot_notifier(&dp_bl->nb);

#ifdef CONFIG_HAS_EARLYSUSPEND
	dp_bl->bk_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	dp_bl->bk_early_suspend.suspend = bk_e_suspend;
	dp_bl->bk_early_suspend.resume = bk_l_resume;
	register_early_suspend(&dp_bl->bk_early_suspend);
#endif

	return 0;

err_bl:
	kfree(dp_bl);
err_alloc:

	return ret;
}

static int digital_pulse_backlight_remove(struct platform_device *pdev)
{
	struct platform_digital_pulse_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct digital_pulse_bl_data *dp_bl = dev_get_drvdata(&bl->dev);

	backlight_device_unregister(bl);
	close_backlight(bl);

	if (data->exit)
		data->exit(&pdev->dev);
	if (dp_bl->pdata->digital_pulse_gpio)
		gpio_free(dp_bl->pdata->digital_pulse_gpio);

	unregister_reboot_notifier(&dp_bl->nb);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&dp_bl->bk_early_suspend);
#endif

	kfree(dp_bl);

	return 0;
}

#if defined(CONFIG_PM) && 0 /* If no register early suspend open it */
static int digital_pulse_backlight_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct digital_pulse_bl_data *pb = dev_get_drvdata(&bl->dev);
	if (pb->notify)
		pb->notify(&bl->dev, 0);
	close_backlight(bl);

	return 0;
}

static int digital_pulse_backlight_resume(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	backlight_update_status(bl);

	return 0;
}
#else
#define digital_pulse_backlight_suspend	NULL
#define digital_pulse_backlight_resume	NULL
#endif

static struct platform_driver digital_pulse_backlight_driver = {
	.driver		= {
		.name	= "digital-pulse-backlight",
		.owner	= THIS_MODULE,
	},
	.probe		= digital_pulse_backlight_probe,
	.remove		= digital_pulse_backlight_remove,
	.suspend	= digital_pulse_backlight_suspend,
	.resume		= digital_pulse_backlight_resume,
};

static int __init digital_pulse_backlight_init(void)
{
	return platform_driver_register(&digital_pulse_backlight_driver);
}
module_init(digital_pulse_backlight_init);

static void __exit digital_pulse_backlight_exit(void)
{
	platform_driver_unregister(&digital_pulse_backlight_driver);
}
module_exit(digital_pulse_backlight_exit);

MODULE_DESCRIPTION("Digital pulse based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:digital-pulse-backlight");

