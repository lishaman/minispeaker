/* 
 * drivers/input/touchscreen/ldwzic_ts.c
 *
 * FocalTech ldwzic TouchScreen driver. 
 *
 * Copyright (c) 2010  Ingenic Semiconductor Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *	note: only support mulititouch	liaoqizhen 2010-09-01
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/tsc.h>

#include "ldwzic_ts.h"

//#undef CONFIG_LINDAWZ_DEBUG
//#define CONFIG_LINDAWZ_DEBUG

//#define PENUP_TIMEOUT_TIMER 1
//#define P2_PACKET_LEN 14
//#define P1_PACKET_LEN 9
#define POINT_READ_LEN 9 //16 //14 //9
#define REG_SUM  14 //18 //14

#define CONFIG_TOUCHSCREEN_X_FLIP 1
#define CONFIG_TOUCHSCREEN_Y_FLIP 1

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16 pen_up;
	u16	pressure;
	u8  touch_point;
};

struct ldwzic_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *reset;
};

struct ldwzic_ts_data {
	struct i2c_client *client;
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
#ifdef PENUP_TIMEOUT_TIMER
	struct timer_list penup_timeout_timer;
#endif
#ifdef CONFIG_LINDAWZ_DEBUG
	long int count;
#endif
	struct ldwzic_gpio gpio;
	struct regulator *power;
};

static int ldwzic_i2c_rxdata(struct i2c_client *client, u8 *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= client->addr,
			.flags	= 0, //|I2C_M_NOSTART,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD, //|I2C_M_NOSTART,
			.len	= length,
			.buf	= rxdata,
		},
	};
	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);	

	return ret;
}

static int ldwzic_i2c_txdata(struct i2c_client *client, u8 *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= client->addr,
			.flags	= 0, //|I2C_M_NOSTART,
			.len	= length,
			.buf	= txdata,
		},
	};

	msleep(1);
	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ldwzic_set_reg(struct ldwzic_ts_data *ts, u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;
	buf[0] = addr;
	buf[1] = para;
	ret = ldwzic_i2c_txdata(ts->client, buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}

	return 0;
}

static int ldwzic_ts_power_off(struct ldwzic_ts_data *ts)
{
	if (ts->power)
		return regulator_disable(ts->power);

	return 0;
}

static int ldwzic_ts_power_on(struct ldwzic_ts_data *ts)
{
	if (ts->power)
		return regulator_enable(ts->power);

	return 0;
}

static void ldwzic_ts_release(struct ldwzic_ts_data *ldwzic_ts)
{
#ifdef CONFIG_LDWZIC_MULTI_TOUCH	
	input_mt_sync(ldwzic_ts->input_dev);
#else
	input_report_abs(ldwzic_ts->input_dev, ABS_PRESSURE, 0);
	input_report_key(ldwzic_ts->input_dev, BTN_TOUCH, 0);
#endif
	input_sync(ldwzic_ts->input_dev);
#ifdef PENUP_TIMEOUT_TIMER
	del_timer(&(ldwzic_ts->penup_timeout_timer));
#endif
}
static void ldwzic_chip_reset(struct ldwzic_ts_data *ts)
{

	set_pin_status(ts->gpio.reset, 1);
	msleep(20);
	set_pin_status(ts->gpio.reset, 0);
	msleep(20);
	set_pin_status(ts->gpio.reset, 1);
	msleep(20);
}

static unsigned long transform_to_screen_x(unsigned long x )
{
	if (x < TP_MIN_X) 
		x = TP_MIN_X;
	if (x > TP_MAX_X) 
		x = TP_MAX_X;

	return (x - TP_MIN_X) * SCREEN_MAX_X / (TP_MAX_X - TP_MIN_X);
}

static unsigned long transform_to_screen_y(unsigned long y)
{
	if (y < TP_MIN_Y) 
		y = TP_MIN_Y;
	if (y > TP_MAX_Y) 
		y = TP_MAX_Y;

	return (y - TP_MIN_Y) * SCREEN_MAX_Y / (TP_MAX_Y - TP_MIN_Y);
}

static int ldwzic_read_data(struct ldwzic_ts_data *ldwzic_ts)
{
	struct ts_event *event = &ldwzic_ts->event;
	u8 buf[REG_SUM];
	int ret = -1;

	buf[0] = 0;// read from reg 0
	ret = ldwzic_i2c_rxdata(ldwzic_ts->client, buf, POINT_READ_LEN);
	if (ret < 0) {
		ldwzic_chip_reset(ldwzic_ts);
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}
	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[0] & REG_STATE_MASK;
	//CLIVIA DON'T KNOW WHAT DO FOR	event->pen_up = ((buf[1] >> 6)==0x01);

	if ((event->touch_point == 0)) {
		ldwzic_ts_release(ldwzic_ts);
		return 1; 
	}
#ifdef CONFIG_LDWZIC_MULTI_TOUCH
	switch (event->touch_point) {
		case 2:
			event->x1 = ((((u16)buf[5])<<8)&0x0f00) |buf[6];
			event->y1 = ((((u16)buf[7])<<8)&0x0f00) |buf[8];
			event->touch_point = 1;// ONE POINT

			break;
		case 1:
			event->x1 = ((((u16)buf[1])<<8)&0x0f00) |buf[2];
			event->y1 = ((((u16)buf[3])<<8)&0x0f00) |buf[4];

			break;
		case 3:
			event->x1 = ((((u16)buf[1])<<8)&0x0f00) |buf[2];
			event->y1 = ((((u16)buf[3])<<8)&0x0f00) |buf[4];
			event->x2 = ((((u16)buf[5])<<8)&0x0f00) |buf[6];
			event->y2 = ((((u16)buf[7])<<8)&0x0f00) |buf[8];
			event->touch_point = 2;// TWO POINT
			break;
		default:
			return -1;
	}
#ifdef CONFIG_TOUCHSCREEN_X_FLIP
	event->x1 = SCREEN_MAX_X - transform_to_screen_x(event->x1);
#else
	event->x1 = transform_to_screen_x(event->x1);
#endif

#ifdef CONFIG_TOUCHSCREEN_Y_FLIP
	event->y1 = SCREEN_MAX_Y - transform_to_screen_y(event->y1);
#else
	event->y1 = transform_to_screen_y(event->y1);
#endif

	if(event->touch_point == 2)
	{

#ifdef CONFIG_TOUCHSCREEN_X_FLIP
		event->x2 = SCREEN_MAX_X - transform_to_screen_x(event->x2);
#else
		event->x2 = transform_to_screen_x(event->x2);
#endif

#ifdef CONFIG_TOUCHSCREEN_Y_FLIP
		event->y2 = SCREEN_MAX_Y - transform_to_screen_y(event->y2);
#else
		event->y2 = transform_to_screen_y(event->y2);
#endif

	}
#else
	switch (event->touch_point) {
		case 2:
			event->x1 = ((((u16)buf[5])<<8)&0x0f00) |buf[6];
			event->y1 = ((((u16)buf[7])<<8)&0x0f00) |buf[8];
		case 1:
			event->x1 = ((((u16)buf[1])<<8)&0x0f00) |buf[2];
			event->y1 = ((((u16)buf[3])<<8)&0x0f00) |buf[4];
			break;
		default:
			return -1;
	}
#ifdef CONFIG_TOUCHSCREEN_X_FLIP
	event->x1 = SCREEN_MAX_X - transform_to_screen_x(event->x1);
#else
	event->x1 = transform_to_screen_x(event->x1);
#endif

#ifdef CONFIG_TOUCHSCREEN_Y_FLIP
	event->y1 = SCREEN_MAX_Y - transform_to_screen_y(event->y1);
#else
	event->y1 = transform_to_screen_y(event->y1);
#endif
#endif
	event->pressure = 200;
#ifdef PENUP_TIMEOUT_TIMER
	mod_timer(&(ldwzic_ts->penup_timeout_timer), jiffies+40);
#endif
	return 0;
}

static void ldwzic_report_value(struct ldwzic_ts_data *ts)
{
	struct ldwzic_ts_data *data = i2c_get_clientdata(ts->client);
	struct ts_event *event = &data->event;

#ifdef CONFIG_LDWZIC_MULTI_TOUCH
	switch(event->touch_point) {
		case 2:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
	//		printk("---x2=%d  y2=%d\n",event->x2,event->y2);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
		case 1:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
	//		printk("---x1=%d  y1=%d\n",event->x1,event->y1);
			input_mt_sync(data->input_dev);
		default:
			break;
	}
#else	/* CONFIG_LDWZIC_MULTI_TOUCH*/
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	input_report_key(data->input_dev, BTN_TOUCH, 1);
#endif	/* CONFIG_LDWZIC_MULTI_TOUCH*/
	input_sync(data->input_dev);

	dev_dbg(&ts->client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
			event->x1, event->y1, event->x2, event->y2);
}	/*end ldwzic_report_value*/

static void ldwzic_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	struct ldwzic_ts_data *ts;
#ifdef CONFIG_LINDAWZ_DEBUG
	long int count = 0;

	count = ts->count;
	printk("==ldwzic read data start(%ld)=\n", count);
#endif
	ts =  container_of(work, struct ldwzic_ts_data, pen_event_work);
	ret = ldwzic_read_data(ts);	
	if (ret == 0) {	
		ldwzic_report_value(ts);
	}
#ifdef CONFIG_LINDAWZ_DEBUG
	printk("==ldwzic report data end(%ld)=\n", count++);
#endif
	//msleep(1);
	enable_irq(ts->client->irq);
}

static irqreturn_t ldwzic_ts_interrupt(int irq, void *dev_id)
{
	struct ldwzic_ts_data *ldwzic_ts = dev_id;
#ifdef CONFIG_LINDAWZ_DEBUG
	static long int count = 0;
#endif
	disable_irq_nosync(ldwzic_ts->client->irq);

#ifdef CONFIG_LINDAWZ_DEBUG
	ldwzic_ts->count = count;	
	printk("==ctp int(%ld)=\n", count++);	
#endif

	if (!work_pending(&ldwzic_ts->pen_event_work)) {
		queue_work(ldwzic_ts->ts_workqueue, &ldwzic_ts->pen_event_work);
	}

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ldwzic_ts_suspend(struct early_suspend *handler)
{
	struct ldwzic_ts_data *ts;
	ts =  container_of(handler, struct ldwzic_ts_data, early_suspend);

	disable_irq(ts->client->irq);
	if(cancel_work_sync(&ts->pen_event_work))
		enable_irq(ts->client->irq);
	flush_workqueue(ts->ts_workqueue);
	//ldwzic_set_reg(LDWZIC_REG_CTRL_OPMODE, REG_OPMODE_POFF | REG_OPMODE_128MS_DSLEEP_5_ADC);
	//msleep(20);
}

static void ldwzic_ts_resume(struct early_suspend *handler)
{
	struct ldwzic_ts_data *ts;
	ts = container_of(handler, struct ldwzic_ts_data, early_suspend);

	ldwzic_chip_reset(ts);	//add by KM: 20111010

	ldwzic_set_reg(ts, LDWZIC_REG_CTRL_OPMODE, REG_TOUCH_USED_OPMODE);
	msleep(20);
	enable_irq(ts->client->irq);
}
#endif  /* CONFIG_HAS_EARLYSUSPEND */

static void ldwzic_gpio_init(struct ldwzic_ts_data *ts)
{
	struct device *dev = &ts->client->dev;
	struct jztsc_platform_data *pdata = dev->platform_data;;

	ts->gpio.irq = &pdata->gpio[0];
	ts->gpio.reset = &pdata->gpio[1];
		
	if (gpio_request_one(ts->gpio.irq->num,
			     GPIOF_DIR_IN, "ldwzic_irq")) {
		dev_err(dev, "no irq pin available\n");
		ts->gpio.irq->num = -EBUSY;
	}

	if (gpio_request_one(ts->gpio.reset->num,
			     ts->gpio.reset->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "ldwzic_reset")) {
		dev_err(dev, "no reset pin available\n");
		ts->gpio.reset->num = -EBUSY;
	}
}

static int ldwzic_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ldwzic_ts_data *ldwzic_ts;
	struct input_dev *input_dev;
	int err = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ldwzic_ts = kzalloc(sizeof(*ldwzic_ts), GFP_KERNEL);
	if (!ldwzic_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	ldwzic_ts->client = client;
	client->dev.init_name=client->name;
	i2c_set_clientdata(client, ldwzic_ts);

	ldwzic_ts->power = regulator_get(&client->dev, "vtsc");
	if (IS_ERR(ldwzic_ts->power)) {
		dev_warn(&client->dev, "get regulator failed\n");
	}
	ldwzic_ts_power_on(ldwzic_ts);
	ldwzic_gpio_init(ldwzic_ts);

	INIT_WORK(&ldwzic_ts->pen_event_work, ldwzic_ts_pen_irq_work);
	ldwzic_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ldwzic_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	client->irq = gpio_to_irq(ldwzic_ts->gpio.irq->num);
	err = request_irq(client->irq, ldwzic_ts_interrupt,
			  IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			  "ldwzic_ts", ldwzic_ts);

	if (err < 0) {
		dev_err(&client->dev, "ldwzic_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ldwzic_ts->input_dev = input_dev;

#ifdef CONFIG_LDWZIC_MULTI_TOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev,
			ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,
			ABS_PRESSURE, 0, PRESS_MAX, 0 , 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name		= LDWZIC_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
				"ldwzic_ts_probe: failed to register input device: %s\n",
				dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
#ifdef PENUP_TIMEOUT_TIMER
	init_timer(&(ldwzic_ts->penup_timeout_timer));
	ldwzic_ts->penup_timeout_timer.data = (unsigned long)ldwzic_ts;
	ldwzic_ts->penup_timeout_timer.function  =	(void (*)(unsigned long))ldwzic_ts_release;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ldwzic_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ldwzic_ts->early_suspend.suspend = ldwzic_ts_suspend;
	ldwzic_ts->early_suspend.resume	= ldwzic_ts_resume;
	register_early_suspend(&ldwzic_ts->early_suspend);
#endif

	ldwzic_chip_reset(ldwzic_ts);	//add by KM: 20111010
	ldwzic_set_reg(ldwzic_ts, LDWZIC_REG_CTRL_OPMODE, REG_TOUCH_USED_OPMODE); //5, 6,7,8
	msleep(40);

	enable_irq(client->irq);

	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, ldwzic_ts);
exit_irq_request_failed:
	cancel_work_sync(&ldwzic_ts->pen_event_work);
	destroy_workqueue(ldwzic_ts->ts_workqueue);
exit_create_singlethread:
	i2c_set_clientdata(client, NULL);
	kfree(ldwzic_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int __devexit ldwzic_ts_remove(struct i2c_client *client)
{
	struct ldwzic_ts_data *ldwzic_ts = i2c_get_clientdata(client);

	unregister_early_suspend(&ldwzic_ts->early_suspend);
	free_irq(client->irq, ldwzic_ts);
	input_unregister_device(ldwzic_ts->input_dev);
	kfree(ldwzic_ts);
	cancel_work_sync(&ldwzic_ts->pen_event_work);
	destroy_workqueue(ldwzic_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
	ldwzic_ts_power_off(ldwzic_ts);
	return 0;
}

static const struct i2c_device_id ldwzic_ts_id[] = {
	{ LDWZIC_NAME, 0 },
};
MODULE_DEVICE_TABLE(i2c, ldwzic_ts_id);

static struct i2c_driver ldwzic_ts_driver = {
	.probe		= ldwzic_ts_probe,
	.remove		= __devexit_p(ldwzic_ts_remove),
	.id_table	= ldwzic_ts_id,
	.driver	= {
		.name	= LDWZIC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init ldwzic_ts_init(void)
{
	return i2c_add_driver(&ldwzic_ts_driver);
}

static void __exit ldwzic_ts_exit(void)
{
	i2c_del_driver(&ldwzic_ts_driver);
}

module_init(ldwzic_ts_init);
module_exit(ldwzic_ts_exit);

MODULE_AUTHOR("<clivia_cui@163.com>");
MODULE_DESCRIPTION("linda unknown test ic TouchScreen driver");
MODULE_LICENSE("GPL");
