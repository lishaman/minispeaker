/*
 * linux/drivers/video/backlight/pwm_bl.c
 *
 * simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
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
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#include "../jz_fb_v12/jz_fb.h"

struct pwm_bl_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
	unsigned int		lth_brightness;
	unsigned int		cur_brightness;
    unsigned int        max_brightness;
	unsigned int		last_brightness;
	unsigned int		suspend;
	unsigned int		first_boot;
	unsigned int		is_suspend;
    struct mutex        pwm_lock;
    struct notifier_block nb;
	int			(*notify)(struct device *,
					  int brightness);
	int			(*check_fb)(struct device *, struct fb_info *);
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend bk_early_suspend;
#endif
};

#ifdef CONFIG_HAS_EARLYSUSPEND
/*
 * Backlight control program at the system wake up
 * need to wait until after the end of the pwm resume.
 */
static int wait_bk_l_resume(struct pwm_bl_data *pb)
{
	int wait_timeout = 2000;
	suspend_state_t suspend_state = get_suspend_state();

	if(pb->first_boot)
		return 0;

	if(suspend_state == PM_SUSPEND_MEM)
		return 1;

	while(pb->suspend && wait_timeout) {
		msleep(1);
		wait_timeout--;
		if(get_suspend_state() == PM_SUSPEND_MEM)
			return 1;
	}
	if(!wait_timeout) {
		printk("pwm wait_bk_l_resume is time out\n");
		return 1;
	}
	return 0;
}

/*
 * Check the backlight settings are in the system wake-up phase.
 */
static inline int backlight_is_suspend(struct pwm_bl_data *pb, int brightness)
{
	if(pb->is_suspend && (brightness < pb->last_brightness)) {
		pb->last_brightness = brightness;
		return 1;
	} else {
		pb->last_brightness = brightness;
		pb->is_suspend = 0;
	}
	return 0;
}
#endif // CONFIG_HAS_EARLYSUSPEND

static int pwm_backlight_update_status(struct backlight_device *bl)
{
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);
	int brightness = bl->props.brightness;
	int max = bl->props.max_brightness;

#ifdef CONFIG_HAS_EARLYSUSPEND
	if(wait_bk_l_resume(pb)) {
		/*pb->last_brightness = brightness;*/
		return 0;
	}
#endif

    mutex_lock(&pb->pwm_lock);

#ifdef CONFIG_HAS_EARLYSUSPEND
	if(backlight_is_suspend(pb, brightness)) {
		mutex_unlock(&pb->pwm_lock);
		return 0;
	}
#endif

	if (bl->props.power != FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;

	if (pb->notify) {
		brightness = pb->notify(pb->dev, brightness);
	}

#if 0
    if (pb->suspend && (brightness > 0)) {
		pb->cur_brightness = brightness;
        //printk("pb->suspend, brightness =%d\n", brightness);
        mutex_unlock(&pb->pwm_lock);
        return 0;
    }
#else
    /*
     * if backlight has been suspended we shouldn't update brightness
     * until backlight has been resumed.
     */
    if (pb->suspend) {
        mutex_unlock(&pb->pwm_lock);
        return 0;
    }
#endif

#ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	brightness = 120;
#endif
    pb->cur_brightness = brightness;
	if (brightness == 0) {
		pwm_config(pb->pwm, 0, pb->period);
		pwm_disable(pb->pwm);
        mdelay(50);
	} else {
		pwm_disable(pb->pwm);
		brightness = pb->lth_brightness +
			(brightness * (pb->period - pb->lth_brightness) / max);
		pwm_config(pb->pwm, brightness, pb->period);
		pwm_enable(pb->pwm);
	}
    mutex_unlock(&pb->pwm_lock);
	return 0;
}

static int pwm_backlight_get_brightness(struct backlight_device *bl)
{
	return bl->props.brightness;
}

static const struct backlight_ops pwm_backlight_ops = {
	.update_status	= pwm_backlight_update_status,
	.get_brightness	= pwm_backlight_get_brightness,
};

static int pwm_bl_shutdown_notify(struct notifier_block *rnb,
			   unsigned long unused2, void *unused3)
{
	struct pwm_bl_data *pb = container_of(rnb,
            struct pwm_bl_data, nb);

	mutex_lock(&pb->pwm_lock);
    pwm_config(pb->pwm, 0, pb->period);
    pwm_disable(pb->pwm);
    mutex_unlock(&pb->pwm_lock);

	return NOTIFY_DONE;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bk_e_suspend(struct early_suspend *h)
{
	struct pwm_bl_data *pb = container_of(h,
            struct pwm_bl_data, bk_early_suspend);

	mutex_lock(&pb->pwm_lock);
    pb->suspend = 1;
	pb->first_boot = 0;
	pb->is_suspend = 1;
	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);
    mutex_unlock(&pb->pwm_lock);
}

static void bk_l_resume(struct early_suspend *h)
{
#if 0
	int brightness;
#endif
	struct pwm_bl_data *pb = container_of(h,
            struct pwm_bl_data, bk_early_suspend);

	mutex_lock(&pb->pwm_lock);
#if 0
	pwm_disable(pb->pwm);
	brightness = pb->cur_brightness;
	brightness = pb->lth_brightness +
		(brightness * (pb->period - pb->lth_brightness) / pb->max_brightness);
	pwm_config(pb->pwm, brightness, pb->period);
	pwm_enable(pb->pwm);
#endif
    pb->suspend = 0;
	pb->first_boot = 0;
    mutex_unlock(&pb->pwm_lock);
}
#endif

static int pwm_backlight_probe(struct platform_device *pdev)
{
	struct backlight_properties props;
	struct platform_pwm_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl;
	struct pwm_bl_data *pb;
	int ret;

	if (!data) {
		dev_err(&pdev->dev, "failed to find platform data\n");
		return -EINVAL;
	}

	if (data->init) {
		ret = data->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}

	pb = kzalloc(sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	pb->period = data->pwm_period_ns;
	pb->notify = data->notify;
	pb->check_fb = data->check_fb;
	pb->lth_brightness = data->lth_brightness *
		(data->pwm_period_ns / data->max_brightness);
	pb->dev = &pdev->dev;
	pb->first_boot = 1;

	pb->pwm = pwm_request(data->pwm_id, "backlight");

	if (IS_ERR(pb->pwm)) {
		dev_err(&pdev->dev, "unable to request PWM for backlight\n");
		ret = PTR_ERR(pb->pwm);
		goto err_pwm;
	} else
		dev_dbg(&pdev->dev, "got pwm for backlight\n");

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = data->max_brightness;
    pb->max_brightness = data->max_brightness;
	bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, pb,
				       &pwm_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_bl;
	}

    mutex_init(&pb->pwm_lock);
	bl->props.brightness = data->dft_brightness;
	if ( ! lcd_display_inited_by_uboot() ) {
		backlight_update_status(bl);
	}

	platform_set_drvdata(pdev, bl);

    pb->nb.notifier_call = pwm_bl_shutdown_notify;
	register_reboot_notifier(&pb->nb);

#if 0
//#ifdef CONFIG_HAS_EARLYSUSPEND
	printk("!!!!!!!!!!!!!!!!!!!!!!!!!!early %s,%d\n",__func__,__LINE__);
    pb->bk_early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;//EARLY_SUSPEND_LEVEL_STOP_DRAWING;
    pb->bk_early_suspend.suspend = bk_e_suspend;
    pb->bk_early_suspend.resume = bk_l_resume;
#ifndef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
    register_early_suspend(&pb->bk_early_suspend);
#endif
#endif

	return 0;

err_bl:
	pwm_free(pb->pwm);
err_pwm:
	kfree(pb);
err_alloc:
	if (data->exit)
		data->exit(&pdev->dev);
	return ret;
}

static int pwm_backlight_remove(struct platform_device *pdev)
{
	struct platform_pwm_backlight_data *data = pdev->dev.platform_data;
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);

	backlight_device_unregister(bl);
	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);
	pwm_free(pb->pwm);
	kfree(pb);
	if (data->exit)
		data->exit(&pdev->dev);

	unregister_reboot_notifier(&pb->nb);
    unregister_early_suspend(&pb->bk_early_suspend);

	return 0;
}

#ifdef CONFIG_PM
static int pwm_backlight_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	/*
	 * Android suspend and wake-up controlled by the earlysuspend
	 * here do not do anything.
	 */
//#ifndef CONFIG_HAS_EARLYSUSPEND
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = dev_get_drvdata(&bl->dev);

	if (pb->notify)
		pb->notify(pb->dev, 0);
	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);
//#endif
	return 0;
}

static int pwm_backlight_resume(struct platform_device *pdev)
{
//#ifndef CONFIG_HAS_EARLYSUSPEND
	struct backlight_device *bl = platform_get_drvdata(pdev);

	backlight_update_status(bl);
//#endif
	return 0;
}
#else
#define pwm_backlight_suspend	NULL
#define pwm_backlight_resume	NULL
#endif

static struct platform_driver pwm_backlight_driver = {
	.driver		= {
		.name	= "pwm-backlight",
		.owner	= THIS_MODULE,
	},
	.probe		= pwm_backlight_probe,
	.remove		= pwm_backlight_remove,
	.suspend	= pwm_backlight_suspend,
	.resume		= pwm_backlight_resume,
};

static int __init pwm_backlight_init(void)
{
	return platform_driver_register(&pwm_backlight_driver);
}
module_init(pwm_backlight_init);

static void __exit pwm_backlight_exit(void)
{
	platform_driver_unregister(&pwm_backlight_driver);
}
module_exit(pwm_backlight_exit);

MODULE_DESCRIPTION("PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-backlight");

