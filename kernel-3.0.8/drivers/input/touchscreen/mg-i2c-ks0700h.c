/*
 * drivers/input/touchscreen/mg-i2c-ks0700h.c
 *
 * driver for KS0700h ---- a magnetic touch screen
 *
 * Copyright (C) 2012 Fighter Sun<wanmyqawdr@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input-group.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/atomic.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
	#include <linux/earlysuspend.h>
#endif

#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/tsc.h>

#define MG_DRIVER_NAME  "mg-i2c-ks0700h-ts"

#define COORD_INTERPRET(MSB_BYTE, LSB_BYTE) \
        (MSB_BYTE << 8 | LSB_BYTE)

#define CAP_X_CORD              0x37ff
#define CAP_Y_CORD              0x1fff
#define DIG_MAX_P               0xff

#define BUF_SIZE                8

enum mg_capac_report {
	MG_MODE = 0x0,
	MG_CONTACT_ID,
	MG_STATUS,
	MG_POS_X_LOW,
	MG_POS_X_HI,
	MG_POS_Y_LOW,
	MG_POS_Y_HI,
	MG_POINTS,

	MG_MODE2,
	MG_CONTACT_ID2,
	MG_STATUS2,
	MG_POS_X_LOW2,
	MG_POS_X_HI2,
	MG_POS_Y_LOW2,
	MG_POS_Y_HI2,

	MG_POINTS2,
};

enum mg_dig_report {
	MG_DIG_MODE = 0x0,
	MG_DIG_STATUS, MG_DIG_X_LOW, MG_DIG_X_HI, MG_DIG_Y_LOW, MG_DIG_Y_HI,
	/* Z represents pressure value */
	MG_DIG_Z_LOW, MG_DIG_Z_HI,
};

enum mg_dig_state {
	MG_OUT_RANG = 0,
	MG_FLOATING = 0x10,
	MG_ON_PAD = 0x11,
	MG_FLOATING_B_DOWN = 0x12,
	MG_FLOATING_T_DOWN = 0x13,
};


struct mg_data {
	uint32_t x, y, w, p, id;
	struct i2c_client *client;
	struct input_dev *dig_dev;
	int irq;
	int gpio_irq;
	int gpio_rst;
	struct workqueue_struct *i2c_workqueue;
	struct work_struct work;
	struct regulator *power;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend mg_early_suspend;
#endif

	int suspend;

	atomic_t members_locked;
	struct input_group group;
	struct timer_list timer;
	spinlock_t lock;

	struct jztsc_platform_data *pdata;
};

static struct i2c_driver mg_driver;

static void mg_hw_reset(struct mg_data *mg)
{
	int rst;

	if (mg->gpio_rst == -1)
		return;

	rst = !(1 ^ !!mg->pdata->gpio[1].enable_level);

	gpio_set_value(mg->gpio_rst, rst);
	mdelay(100);
	gpio_set_value(mg->gpio_rst, !rst);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void mg_suspend(struct early_suspend *h)
{
	struct mg_data *mg = container_of(h, struct mg_data, mg_early_suspend);

	mg->suspend = 1;

	flush_work_sync(&mg->work);

	if (!mg->pdata->wakeup)
		disable_irq(mg->irq);

	if (mg->power && !mg->pdata->wakeup)
		regulator_disable(mg->power);
}

static void mg_resume(struct early_suspend *h)
{
	struct mg_data *mg = container_of(h, struct mg_data, mg_early_suspend);

	mg->suspend = 0;

	if (mg->power && !mg->pdata->wakeup) {
		regulator_enable(mg->power);
		mdelay(100);
	}

	mg_hw_reset(mg);

	if (!mg->pdata->wakeup)
		enable_irq(mg->irq);
}

#endif

static irqreturn_t mg_irq(int irq, void *_mg) {
	struct mg_data *mg = _mg;

	if (!work_pending(&mg->work)) {
		disable_irq_nosync(mg->irq);
		queue_work(mg->i2c_workqueue, &mg->work);
	}

	return IRQ_HANDLED;
}

static void report_value(struct mg_data *mg) {
	input_mt_slot(mg->dig_dev, 0);
	input_mt_report_slot_state(mg->dig_dev, MT_TOOL_FINGER, true);
	input_report_abs(mg->dig_dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(mg->dig_dev, ABS_MT_POSITION_X, mg->x);
	input_report_abs(mg->dig_dev, ABS_MT_POSITION_Y, mg->y);
	input_report_abs(mg->dig_dev, ABS_MT_PRESSURE, mg->p >> 2);

	input_sync(mg->dig_dev);
}

static void report_wakeup(struct mg_data *mg) {
	input_report_key(mg->dig_dev, KEY_POWER, 1);
	input_sync(mg->dig_dev);

	input_report_key(mg->dig_dev, KEY_POWER, 0);
	input_sync(mg->dig_dev);
}

static void report_t_btn_down(struct mg_data *mg) {
	input_report_key(mg->dig_dev, KEY_LEFTSHIFT, 1);
	input_sync(mg->dig_dev);
}

static void report_t_btn_up(struct mg_data *mg) {
	input_report_key(mg->dig_dev, KEY_LEFTSHIFT, 0);
	input_sync(mg->dig_dev);
}

static void report_b_btn_down(struct mg_data *mg) {
	input_report_key(mg->dig_dev, KEY_LEFTALT, 1);
	input_sync(mg->dig_dev);
}

static void report_b_btn_up(struct mg_data *mg) {
	input_report_key(mg->dig_dev, KEY_LEFTALT, 0);
	input_sync(mg->dig_dev);
}

static void report_release(struct mg_data *mg) {
	input_mt_slot(mg->dig_dev, 0);
	input_mt_report_slot_state(mg->dig_dev, MT_TOOL_FINGER, false);

	input_sync(mg->dig_dev);
}

static inline void remap_to_view_size(struct mg_data *mg) {
	mg->x = mg->x * mg->pdata->x_max / CAP_X_CORD;
	mg->y = mg->y * mg->pdata->y_max / CAP_Y_CORD;
}


static inline void mg_report(struct mg_data *mg) {
	static int prev_s = -1;
	static int current_s = -1;

	static int changed;

	static int floating_b_down;
	static int floating_t_down;

	if (!atomic_read(&mg->members_locked)) {
		input_group_lock(&mg->group);
		atomic_set(&mg->members_locked, 1);
	}

	if (atomic_read(&mg->members_locked))
		mod_timer(&mg->timer, get_jiffies_64() + HZ / 8);

	prev_s = current_s;
	current_s = mg->w;

	remap_to_view_size(mg);

	switch (current_s) {
	case MG_OUT_RANG: //0x0
		if (changed) {
			report_release(mg);
			changed = 0;

			if (mg->pdata->wakeup && mg->suspend)
				report_wakeup(mg);
		}

		if (floating_b_down) {
			floating_b_down = 0;
			report_b_btn_up(mg);
		}

		if (floating_t_down) {
			floating_t_down = 0;
			report_t_btn_up(mg);
		}

		break;

	case MG_FLOATING: //0x10
		if (prev_s == MG_FLOATING_B_DOWN &&
				floating_b_down)
			report_b_btn_down(mg);

		if (prev_s == MG_FLOATING_T_DOWN &&
				floating_t_down)
			report_t_btn_down(mg);

		if (floating_b_down ||
				floating_t_down) {
			report_value(mg);
			changed = 1;

		} else if (changed) {
			report_release(mg);
			changed = 0;

			if (mg->pdata->wakeup && mg->suspend)
				report_wakeup(mg);
		}

		break;

	case MG_ON_PAD: //0x11
		report_value(mg);
		changed = 1;

		if (floating_b_down) {
			floating_b_down = 0;
			report_b_btn_up(mg);
		}

		if (floating_t_down) {
			floating_t_down = 0;
			report_t_btn_up(mg);
		}

		break;

	case MG_FLOATING_B_DOWN: //0x12
		if (prev_s == MG_FLOATING) {
			if (!floating_b_down) {
				floating_b_down = 1;

			} else if (floating_b_down) {
				floating_b_down = 0;
				report_b_btn_up(mg);
			}
		}

		if (floating_t_down) {
			if (prev_s == MG_FLOATING) {
				floating_t_down = 0;
				report_t_btn_up(mg);

			} else if (prev_s == MG_FLOATING_T_DOWN) {
				report_t_btn_down(mg);

			} else {
				report_value(mg);
				changed = 1;
			}
		}

		break;

	case MG_FLOATING_T_DOWN: //0x13
		if (prev_s == MG_FLOATING) {
			if (!floating_t_down) {
				floating_t_down = 1;

			} else if (floating_t_down) {
				floating_t_down = 0;
				report_t_btn_up(mg);
			}
		}

		break;

	default:
		break;
	}
}

static void mg_i2c_work(struct work_struct *work) {
	int i = 0;
	struct mg_data *mg = container_of(work, struct mg_data, work);
	u_int8_t ret = 0;
	u8 read_buf[BUF_SIZE]; /* buffer for first point of multitouch */

	for (i = 0; i < BUF_SIZE; i++)
		read_buf[i] = 0;

	ret = i2c_smbus_read_i2c_block_data(mg->client, 0x0, BUF_SIZE, read_buf);

	if (ret < 0) {
		printk("Read error!!!!!\n");
		goto err_enable_irq;
	}

	if (read_buf[MG_MODE] >> 1) {
		mg->x = COORD_INTERPRET(read_buf[MG_DIG_X_HI], read_buf[MG_DIG_X_LOW]); //3,2
		mg->y = COORD_INTERPRET(read_buf[MG_DIG_Y_HI], read_buf[MG_DIG_Y_LOW]); //5,4
		mg->w = read_buf[MG_DIG_STATUS]; //1
		mg->p =
				(COORD_INTERPRET(read_buf[MG_DIG_Z_HI], read_buf[MG_DIG_Z_LOW]));

		mg_report(mg);
	}

err_enable_irq:
	enable_irq(mg->irq);
}

void mg_timer_func(unsigned long data)
{
	struct mg_data *mg = (struct mg_data *) data;

	if (atomic_read(&mg->members_locked)) {
		input_group_unlock(&mg->group);
		atomic_set(&mg->members_locked, 0);
	}
}

static int mg_probe(struct i2c_client *client, const struct i2c_device_id *ids) {
	struct jztsc_platform_data *pdata = client->dev.platform_data;
	struct mg_data *mg;
	struct input_dev *input_dig;
	int err = 0;

	mg = kzalloc(sizeof(struct mg_data), GFP_KERNEL);
	if (!mg)
		return -ENOMEM;

	mg->client = client;
	i2c_set_clientdata(client, mg);
	mg->pdata = pdata;

	if (pdata->power_name) {
		mg->power = regulator_get(&client->dev, pdata->power_name);
		if (IS_ERR(mg->power)) {
			mg->power = NULL;
			dev_err(&mg->client->dev, "Failed to request regulator\n"
					" assume a fixed power supply.\n");
		} else {
			regulator_enable(mg->power);
			mdelay(100);
		}
	} else {
		mg->power = NULL;
		dev_err(&mg->client->dev, "Haven't given a power name.\n"
				" assume a fixed power supply.\n");
	}

	if (pdata->gpio[1].num != -1) {
		mg->gpio_rst = pdata->gpio[1].num;
		err = gpio_request(mg->gpio_rst, "mg_ts_rst");
		if (err) {
			err = -EIO;
			dev_err(&mg->client->dev, "Failed to request GPIO: %d\n", mg->gpio_rst);
			goto err_kfree;
		}
		gpio_direction_output(mg->gpio_rst, pdata->gpio[1].enable_level);
	} else {
		mg->gpio_rst = -1;
		dev_err(&mg->client->dev, "Haven't given a reset GPIO.\n"
				" assume powerup HW reset.\n");
	}

	mg->gpio_irq = pdata->gpio[0].num;
	err = gpio_request(mg->gpio_irq, "mg_ts_int");
	if (err) {
		err = -EIO;
		dev_err(&mg->client->dev, "Failed to request GPIO: %d\n", mg->gpio_irq);
		goto err_gpio_free_rst;
	}
	gpio_direction_input(mg->gpio_irq);

	mg->client = client;
	i2c_set_clientdata(client, mg);

	input_dig = input_allocate_device();
	if (!input_dig) {
		err = -ENOMEM;
		dev_err(&mg->client->dev, "Failed to allocate input device.\n");
		goto err_gpio_free_irq;
	}

	set_bit(EV_ABS, input_dig->evbit);
	set_bit(INPUT_PROP_DIRECT, input_dig->propbit);

	/* assigned with 2 even we only got 1 point, this may be a bug of input subsys */
	input_mt_init_slots(input_dig, 2);
	input_set_abs_params(input_dig, ABS_MT_POSITION_X, 0, pdata->x_max, 0, 0);
	input_set_abs_params(input_dig, ABS_MT_POSITION_Y, 0, pdata->y_max, 0, 0);
	input_set_abs_params(input_dig, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dig, ABS_MT_PRESSURE, 0, DIG_MAX_P, 0, 0);

	set_bit(EV_KEY, input_dig->evbit);
	set_bit(KEY_MENU, input_dig->keybit);
	set_bit(KEY_BACK, input_dig->keybit);
	set_bit(KEY_POWER, input_dig->keybit);
	set_bit(KEY_LEFTSHIFT, input_dig->keybit);
	set_bit(KEY_LEFTALT, input_dig->keybit);

	input_dig->name = MG_DRIVER_NAME;
	input_dig->id.bustype = BUS_I2C;

	err = input_register_device(input_dig);
	if (err) {
		dev_err(&mg->client->dev, "Failed to register input device.\n");
		goto err_regulator_put;
	}

	mg->dig_dev = input_dig;

	mg->group.master = input_dig;
	err = input_register_group(&mg->group);
	if (err) {
		dev_err(&mg->client->dev, "Failed to register a input group.\n");
		goto err_input_unregister;
	}

	INIT_WORK(&mg->work, mg_i2c_work);
	mg->i2c_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!mg->i2c_workqueue) {
		err = -ESRCH;
		goto err_input_unregister_group;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	mg->mg_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	mg->mg_early_suspend.suspend = mg_suspend;
	mg->mg_early_suspend.resume = mg_resume;
	register_early_suspend(&mg->mg_early_suspend);
#endif

	mg_hw_reset(mg);

	setup_timer(&mg->timer, mg_timer_func, (unsigned long) mg);
	atomic_set(&mg->members_locked, 0);

	mg->irq = gpio_to_irq(mg->gpio_irq);
	err = request_irq(mg->irq, mg_irq, IRQF_TRIGGER_FALLING, MG_DRIVER_NAME, mg);
	if (err) {
		dev_err(&mg->client->dev, "Failed to request irq.\n");
		goto err_destroy_workqueue;
	}

	if (pdata->wakeup)
		enable_irq_wake(mg->irq);

	return 0;

err_destroy_workqueue:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mg->mg_early_suspend);
#endif

	destroy_workqueue(mg->i2c_workqueue);

err_input_unregister_group:
	input_unregister_group(&mg->group);

err_input_unregister:
	input_unregister_device(mg->dig_dev);

err_regulator_put:
	regulator_put(mg->power);

err_gpio_free_rst:
	gpio_free(mg->gpio_rst);

err_gpio_free_irq:
	gpio_free(mg->gpio_irq);

err_kfree:
	kfree(mg);

	return err;
}

static int __devexit mg_remove(struct i2c_client *client) {
	struct mg_data *mg = i2c_get_clientdata(client);

	disable_irq(mg->irq);
	flush_work_sync(&mg->work);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mg->mg_early_suspend);
#endif

	del_timer_sync(&mg->timer);
	destroy_workqueue(mg->i2c_workqueue);
	free_irq(mg->irq, mg);
	input_unregister_group(&mg->group);
	input_unregister_device(mg->dig_dev);
	gpio_free(mg->gpio_irq);
	gpio_free(mg->gpio_rst);
	regulator_put(mg->power);
	kfree(mg);

	return 0;
}

static struct i2c_device_id mg_id_table[] = {
	{ MG_DRIVER_NAME, 0 },
	{ }
};

static struct i2c_driver mg_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = MG_DRIVER_NAME,
	},

	.id_table = mg_id_table,
	.probe = mg_probe,
	.remove = mg_remove,
};

static int __init mg_init(void) {
	return i2c_add_driver(&mg_driver);
}

static void __exit mg_exit(void) {
	i2c_del_driver(&mg_driver);
}

module_init(mg_init);
module_exit(mg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fighter Sun <wanmyqawdr@126.com>");
MODULE_DESCRIPTION("Driver for KS0700h a magnetic touch screen");
MODULE_ALIAS("i2c:mg-i2c-ks0700h-ts");
