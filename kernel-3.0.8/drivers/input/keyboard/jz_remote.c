/* drivers/input/keyboard/jz_remote.c
 *
 * Infrared sensor code for Ingenic JZ SoC
 *
 * Copyright(C)2012 Ingenic Semiconductor Co., LTD.
 *	http://www.ingenic.cn
 *	Sun Jiwei <jwsun@ingenic.cddn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/sched.h>

#include <linux/gpio.h>
#include <linux/input/remote.h>



struct jz_remote {
	struct platform_device *pdev;
	struct timer_list timer;
	struct input_dev *input_dev;

	struct jz_remote_board_data *jz_remote_board_data;

	int irq;

	struct wake_lock work_wake_lock;

	unsigned int long_press_state;
	unsigned int long_press_count;

	unsigned char current_data;
	unsigned char keycodes[11];
	unsigned char key;

	struct timeval tv, tv_old;
};
static  long  dur=0,dur_old,start=0 ;
static int  num=0;
uint8_t frame[4];

#define KEY_PHYS_NAME	"jz_remote_button/input0"



static int jz_remote_ctrl_open(struct input_dev *dev)
{
	return 0;
}

static void jz_remote_ctrl_close(struct input_dev *dev)
{
}

void jz_remote_send_key(struct jz_remote *jz_remote) {
	uint8_t send_key = 0;
	send_key = jz_remote->jz_remote_board_data->key_maps[jz_remote->current_data];
	input_report_key(jz_remote->input_dev, send_key, 1);
	input_sync(jz_remote->input_dev);
	input_report_key(jz_remote->input_dev, send_key, 0);
	input_sync(jz_remote->input_dev);
}


static irqreturn_t jz_remote_irq_handler(s32 irq, void *data) {
	struct jz_remote *jz_remote = data;
	int i;
	jz_remote->tv_old.tv_sec = jz_remote->tv.tv_sec;
	jz_remote->tv_old.tv_usec = jz_remote->tv.tv_usec;
	do_gettimeofday(&jz_remote->tv);

	dur_old = dur;
	if (jz_remote->tv.tv_sec == jz_remote->tv_old.tv_sec)
		dur = (jz_remote->tv.tv_usec - jz_remote->tv_old.tv_usec);
	else
		dur = (jz_remote->tv.tv_usec - jz_remote->tv_old.tv_usec) + 1000000;
	if (dur < 12000 && dur > 11000 && dur_old > 90000 && dur_old < 100000) {
		jz_remote_send_key(jz_remote);
		start = 0;
		return IRQ_HANDLED;;
	}
	if (dur < 15000 && dur > 13000) {
		start = 1;
		num = 0;
	}
	if (start) {

		if (num) {
			if (dur > 1700 && dur < 3300) {
				frame[(num - 1) / 8] = frame[(num - 1) / 8] >> 1;
				frame[(num - 1) / 8] = frame[(num - 1) / 8] | 0x80;

			} else if (dur < 1500 && dur > 800) {
				frame[(num - 1) / 8] = frame[(num - 1) / 8] >> 1;
				frame[(num - 1) / 8] = frame[(num - 1) / 8] | 0x00;

			} else if (num > 1 && num < 33) {
				start = 0;
				return IRQ_HANDLED;;
			}
		}
		num++;

		if (num == 33) {

			jz_remote->current_data = frame[2];
			if ((uint8_t) frame[2] == (uint8_t) ~frame[3]) {
				jz_remote_send_key(jz_remote);

			} else {
				printk("wrong check value %x,%x\n", (uint8_t) frame[2],
						(uint8_t) ~frame[3]);
			}
			for (i = 0; i < 4; i++)
				frame[i] = 0;
			start = 0;
		}

	}
	return IRQ_HANDLED;
}

static int __devinit jz_remote_probe(struct platform_device *pdev)
{


	struct jz_remote *jz_remote;
	struct input_dev *input_dev;
	int  i, ret;

	jz_remote = kzalloc(sizeof(struct jz_remote), GFP_KERNEL);
	if (!jz_remote) {
		dev_err(&pdev->dev, "failed to allocate input device for remote ctrl \n");
		return -ENOMEM;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "failed to allocate input device for remote ctrl \n");
		return -ENOMEM;
	}

	jz_remote->jz_remote_board_data = pdev->dev.platform_data;
	if(jz_remote->jz_remote_board_data->init) jz_remote->jz_remote_board_data->init(NULL);

	if (gpio_request_one(jz_remote->jz_remote_board_data->gpio,
				GPIOF_DIR_IN, "jz_remote")) {
		dev_err(&pdev->dev, "The gpio is used by other driver\n");
		jz_remote->jz_remote_board_data->gpio = -EBUSY;
		ret = -ENODEV;
		goto err_free;
	}

	jz_remote->irq = gpio_to_irq(jz_remote->jz_remote_board_data->gpio);
	if (jz_remote->irq < 0) {
		ret = jz_remote->irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto err_free_gpio;
	}



	ret = request_irq(jz_remote->irq, jz_remote_irq_handler,
			IRQF_TRIGGER_FALLING | IRQF_DISABLED | IRQF_NO_SUSPEND, "jz-remote", jz_remote);
	if (ret < 0) {
		printk(KERN_CRIT "Can't request IRQ for jz_remote__irq\n");
		return ret;
	}

	input_dev->name = pdev->name;
	input_dev->open = jz_remote_ctrl_open;
	input_dev->close = jz_remote_ctrl_close;
	input_dev->dev.parent = &pdev->dev;
	input_dev->phys = KEY_PHYS_NAME;
	input_dev->id.vendor = 0x0001;

	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	input_dev->keycodesize = sizeof(unsigned char);

	for (i = 0; i < 256; i++) {
		if (jz_remote->jz_remote_board_data->key_maps[i] != -1)
			set_bit(jz_remote->jz_remote_board_data->key_maps[i],
					input_dev->keybit);
	}


	clear_bit(0, input_dev->keybit);

	jz_remote->input_dev = input_dev;

	input_set_drvdata(input_dev, jz_remote);

	wake_lock_init(&jz_remote->work_wake_lock, WAKE_LOCK_SUSPEND, "jz-remote");

	input_dev->evbit[0] = BIT_MASK(EV_KEY) ;

	platform_set_drvdata(pdev, jz_remote);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register input jz_remote  device\n");
		goto err_unregister;
	}
	printk("The remote driver registe success\n");
	return 0;
err_unregister:
	input_unregister_device(jz_remote->input_dev);
	platform_set_drvdata(pdev, NULL);
err_free_gpio:
	gpio_free(jz_remote->jz_remote_board_data->gpio);
err_free:
	input_free_device(input_dev);
	kfree(jz_remote);
	return ret;
}

static int __devexit jz_remote__remove(struct platform_device *pdev)
{
	struct jz_remote *jz_remote = platform_get_drvdata(pdev);

	input_unregister_device(jz_remote->input_dev);
	input_free_device(jz_remote->input_dev);
	kfree(jz_remote);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int jz_remote_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jz_remote *ddata = platform_get_drvdata(pdev);

	enable_irq_wake(ddata->irq);

	return 0;
}

static int jz_remote_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jz_remote *ddata = platform_get_drvdata(pdev);

	disable_irq_wake(ddata->irq);

	return 0;
}

static const struct dev_pm_ops jz_remote_pm_ops = {
	.suspend	= jz_remote_suspend,
	.resume		= jz_remote_resume,
};
#endif
static struct platform_driver jz_remote_driver = {
	.probe		= jz_remote_probe,
	.remove 	= __devexit_p(jz_remote__remove),
	.driver 	= {
		.name	= "jz-remote",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &jz_remote_pm_ops,
#endif
	},
};

 int __init jz_remote_init(void)
{
	return platform_driver_register(&jz_remote_driver);
}

static void __exit jz_remote_exit(void)
{
	platform_driver_unregister(&jz_remote_driver);
}

module_init(jz_remote_init);
module_exit(jz_remote_exit);

MODULE_DESCRIPTION("jz_remote_irq Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sun Jiwei<jwsun@ingenic.cn>");
