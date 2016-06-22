/*
 *
 * drivers/input/touchscreen/gwtc9xxxb_ts.c
 *
 * FocalTech gwtc9xxxb TouchScreen driver.
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
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/tsc.h>

#include "gwtc9xxxb_ts.h"

#define DEBUG_LCD_VCC_ALWAYS_ON
#define PENUP_TIMEOUT_TIMER 1
#define P2_PACKET_LEN 11
#define P1_PACKET_LEN 5
//#define P1_PACKET_LEN 11
#define DATABASE 0
struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	gesture_code;
	u16	sleep_mode;
	u16 	pressure;
	u8	touch_point;
};

struct gwtc9xxxb_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *reset;
};

struct gwtc9xxxb_ts_data {
	struct i2c_client	*client;
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct gwtc9xxxb_gpio	gpio;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
	struct regulator	*power;
	struct mutex lock;
#ifdef PENUP_TIMEOUT_TIMER
	struct timer_list penup_timeout_timer;
#endif
#ifdef CONFIG_GWTC9XXXB_DEBUG
	long int count;
#endif
};

static int gwtc9xxxb_ts_power_off(struct gwtc9xxxb_ts_data *ts);
static int gwtc9xxxb_ts_power_on(struct gwtc9xxxb_ts_data *ts);
static void gwtc9xxxb_ts_reset(struct gwtc9xxxb_ts_data *ts);

static int gwtc9xxxb_i2c_rxdata(struct gwtc9xxxb_ts_data *ts, char *rxdata, int length)
{
	int ret;
	struct i2c_msg msg1[] = {
		{
			.addr	= ts->client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
	};

	struct i2c_msg msg2[] = {
		{
			.addr	= ts->client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

	mutex_lock(&ts->lock);
	ret = i2c_transfer(ts->client->adapter, msg1, 1);
	if (ret < 0){
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
		return ret;
	}

	ret = i2c_transfer(ts->client->adapter, msg2, 1);
	mutex_unlock(&ts->lock);
	if (ret < 0){
		pr_err("msg %s i2c read error: %d\n", __func__, ret);
		return ret;
	}

	return ret;
}

static int gwtc9xxxb_i2c_txdata(struct gwtc9xxxb_ts_data *ts, char *txdata, int length)
{
	int ret;
	struct i2c_msg msg[] = {
		{
			.addr	= ts->client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	msleep(1);
	mutex_lock(&ts->lock);
	ret = i2c_transfer(ts->client->adapter, msg, 1);
	mutex_unlock(&ts->lock);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int gwtc9xxxb_set_reg(struct gwtc9xxxb_ts_data *ts, u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = gwtc9xxxb_i2c_txdata(ts, buf, 2);
    if (ret < 0) {
	pr_err("write reg failed! %#x ret: %d", buf[0], ret);
	return -1;
    }
    return 0;
}

static void gwtc9xxxb_ts_release(struct gwtc9xxxb_ts_data *gwtc9xxxb_ts)
{
#ifdef CONFIG_GWTC9XXXB_MULTITOUCH
//	input_report_abs(gwtc9xxxb_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
	input_mt_sync(gwtc9xxxb_ts->input_dev);
#else
	input_report_abs(gwtc9xxxb_ts->input_dev, ABS_PRESSURE, 0);
	input_report_key(gwtc9xxxb_ts->input_dev, BTN_TOUCH, 0);
#endif
	input_sync(gwtc9xxxb_ts->input_dev);
#ifdef PENUP_TIMEOUT_TIMER
	del_timer(&(gwtc9xxxb_ts->penup_timeout_timer));
#endif
}

static int gwtc9xxxb_read_data(struct gwtc9xxxb_ts_data *ts)
{
	struct ts_event *event = &ts->event;
	u8 buf[P2_PACKET_LEN+1] = {0};
	int ret = -1;

	buf[0] = DATABASE;
#ifdef CONFIG_GWTC9XXXB_MULTITOUCH
	ret = gwtc9xxxb_i2c_rxdata(ts, buf, P2_PACKET_LEN);
#else
     	ret = gwtc9xxxb_i2c_rxdata(ts, buf, P1_PACKET_LEN);
#endif
	if (ret < 0) {
		gwtc9xxxb_ts_release(ts);
		gwtc9xxxb_ts_reset(ts);
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

#if 1
	if((buf[1]==0xff) && (buf[2]==0xff) && (buf[3]==0xff) && (buf[4]==0xff)) {
		gwtc9xxxb_ts_release(ts);
		printk("----read buf error----\n");
		return 1;
	}

#endif
//	 printk(KERN_INFO "buf[0] = %d buf[1] = %d buf[2] = %d buf[3] = %d buf[4] = %d buf[5] = %d buf[6] = %d buf[7] = %d buf[8] = %d\n",
//	buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);
	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[0] & 0x0f;
	event->gesture_code = buf[9];
	event->sleep_mode = buf[10];

	if ((event->touch_point == 0)) {
		gwtc9xxxb_ts_release(ts);
		return 1;
	}

#ifdef CONFIG_GWTC9XXXB_MULTITOUCH
    switch (event->touch_point) {
		case 2:
			event->x2 = ((((u16)buf[5])<<8)&0x0f00) |buf[6];
			event->y2 = ((((u16)buf[7])<<8)&0x0f00) |buf[8];
		case 1:
			event->x1 = ((((u16)buf[1])<<8)&0x0f00) |buf[2];
			event->y1 = ((((u16)buf[3])<<8)&0x0f00) |buf[4];
	    break;
		default:
		    return -1;
	}
#else
	if (event->touch_point == 1) {
		event->x1 = ((((u16)buf[1])<<8)&0x0f00) |buf[2];
		event->y1 = ((((u16)buf[3])<<8)&0x0f00) |buf[4];
	}
#endif

    	event->pressure = 200;
#ifdef PENUP_TIMEOUT_TIMER
	mod_timer(&(ts->penup_timeout_timer), jiffies + 40);
#endif
//	printk("%d , (%d, %d), (%d, %d)\n", event->touch_point, event->x1, event->y1, event->x2, event->y2);
	return 0;
}

static void gwtc9xxxb_report_value(struct gwtc9xxxb_ts_data *data)
{
	struct ts_event *event = &data->event;

	//printk("(%d, %d), (%d, %d)\n", event->x1, event->y1, event->x2, event->y2);
#ifdef CONFIG_GWTC9XXXB_MULTITOUCH
	switch(event->touch_point) {
		case 2:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x2);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y2);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
		case 1:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x1);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y1);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
		default:
			break;
	}
#else	/* CONFIG_GWTC9XXXB_MULTITOUCH*/
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	input_report_key(data->input_dev, BTN_TOUCH, 1);
#endif	/* CONFIG_GWTC9XXXB_MULTITOUCH*/
	input_sync(data->input_dev);

	dev_dbg(&data->client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
		event->x1, event->y1, event->x2, event->y2);

}	/*end gwtc9xxxb_report_value*/


static void gwtc9xxxb_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	struct gwtc9xxxb_ts_data *ts =
		container_of(work, struct gwtc9xxxb_ts_data, pen_event_work);

	ret = gwtc9xxxb_read_data(ts);

	if (ret == 0) {
		gwtc9xxxb_report_value(ts);
	}

	enable_irq(ts->client->irq);

}
static irqreturn_t gwtc9xxxb_ts_interrupt(int irq, void *dev_id)
{
	struct gwtc9xxxb_ts_data *gwtc9xxxb_ts = dev_id;
	disable_irq_nosync(gwtc9xxxb_ts->client->irq);

	if (!work_pending(&gwtc9xxxb_ts->pen_event_work)) {
		queue_work(gwtc9xxxb_ts->ts_workqueue, &gwtc9xxxb_ts->pen_event_work);
	} else {
		enable_irq(gwtc9xxxb_ts->client->irq);
	}

	return IRQ_HANDLED;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void gwtc9xxxb_ts_suspend(struct early_suspend *handler)
{

	struct gwtc9xxxb_ts_data *ts;
	ts =  container_of(handler, struct gwtc9xxxb_ts_data, early_suspend);

#ifdef CONFIG_GWTC9XXXB_DEBUG
	printk("==gwtc9xxxb_ts_suspend=\n");
#endif
	disable_irq(ts->client->irq);
	if(cancel_work_sync(&ts->pen_event_work))
		enable_irq(ts->client->irq);
	flush_workqueue(ts->ts_workqueue);
	gwtc9xxxb_set_reg(ts, GWTC9XXXB_REG_SLEEP, SLEEP_MODE);
	gwtc9xxxb_ts_power_off(ts);
}

static void gwtc9xxxb_ts_resume(struct early_suspend *handler)
{
	struct gwtc9xxxb_ts_data *ts;
	ts =  container_of(handler, struct gwtc9xxxb_ts_data, early_suspend);

	gwtc9xxxb_ts_power_on(ts);
#ifdef CONFIG_GWTC9XXXB_DEBUG
	printk("==gwtc9xxxb_ts_resume=\n");
#endif
	gwtc9xxxb_ts_reset(ts);

	enable_irq(ts->client->irq);
}
#endif  //CONFIG_HAS_EARLYSUSPEND

static int gwtc9xxxb_ts_power_off(struct gwtc9xxxb_ts_data *ts)
{
#ifndef DEBUG_LCD_VCC_ALWAYS_ON
	if (!IS_ERR(ts->power)) {
		if (regulator_is_enabled(ts->power))
			return regulator_disable(ts->power);
	}
#endif
	return 0;
}

static int gwtc9xxxb_ts_power_on(struct gwtc9xxxb_ts_data *ts)
{
#ifndef DEBUG_LCD_VCC_ALWAYS_ON
	if (!IS_ERR(ts->power)) {
			return  regulator_enable(ts->power);
	}
#endif
	return 0;
}

static void gwtc9xxxb_ts_reset(struct gwtc9xxxb_ts_data *ts)
{
	set_pin_status(ts->gpio.reset, 1);
	msleep(50);
	set_pin_status(ts->gpio.reset, 0);
	msleep(50);
	set_pin_status(ts->gpio.reset, 1);
	msleep(50);
}

static void gwtc9xxxb_gpio_init(struct gwtc9xxxb_ts_data *ts)
{
	struct device *dev = &ts->client->dev;
	struct jztsc_platform_data *pdata = dev->platform_data;;

	ts->gpio.irq = &pdata->gpio[0];
	ts->gpio.reset = &pdata->gpio[1];

	if (gpio_request_one(ts->gpio.irq->num,
			     GPIOF_DIR_IN, "gwtc9xxxb_irq")) {
		dev_err(dev, "no irq pin available\n");
		ts->gpio.irq->num = -EBUSY;
	}

	if (gpio_request_one(ts->gpio.reset->num,
			     ts->gpio.reset->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "gwtc9xxxb_reset")) {
		dev_err(dev, "no reset pin available\n");
		ts->gpio.reset->num = -EBUSY;
	}
}

static int gwtc9xxxb_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct gwtc9xxxb_ts_data *gwtc9xxxb_ts;
	struct input_dev *input_dev;
	int err = 0;

	printk("-----gwtc9xxxb_ts probe start-----\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	gwtc9xxxb_ts = kzalloc(sizeof(*gwtc9xxxb_ts), GFP_KERNEL);
	if (!gwtc9xxxb_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	mutex_init(&gwtc9xxxb_ts->lock);
	gwtc9xxxb_ts->client = client;
	i2c_set_clientdata(client, gwtc9xxxb_ts);
 //	i2c_jz_setclk(client, 100*1000);

	gwtc9xxxb_gpio_init(gwtc9xxxb_ts);

#ifndef DEBUG_LCD_VCC_ALWAYS_ON
	gwtc9xxxb_ts->power = regulator_get(NULL, "vlcd");
	if (IS_ERR(gwtc9xxxb_ts->power)) {
		dev_warn(&client->dev, "get regulator power failed\n");
		gwtc9xxxb_ts->power = NULL;
	}
	gwtc9xxxb_ts_power_on(gwtc9xxxb_ts);
#endif
	INIT_WORK(&gwtc9xxxb_ts->pen_event_work, gwtc9xxxb_ts_pen_irq_work);
	gwtc9xxxb_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!gwtc9xxxb_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	gwtc9xxxb_ts->input_dev = input_dev;

	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#ifdef CONFIG_GWTC9XXXB_MULTITOUCH
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

	input_dev->name		= GWTC9XXXB_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"gwtc9xxxb_ts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

	client->irq = gpio_to_irq(gwtc9xxxb_ts->gpio.irq->num);
	err = request_irq(client->irq, gwtc9xxxb_ts_interrupt,
			  IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			 "gwtc9xxxb_ts", gwtc9xxxb_ts);
	if (err < 0) {
		dev_err(&client->dev, "request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(gwtc9xxxb_ts->client->irq);


#ifdef PENUP_TIMEOUT_TIMER
	init_timer(&(gwtc9xxxb_ts->penup_timeout_timer));
	gwtc9xxxb_ts->penup_timeout_timer.data = (unsigned long)gwtc9xxxb_ts;
	gwtc9xxxb_ts->penup_timeout_timer.function  =	(void (*)(unsigned long))gwtc9xxxb_ts_release;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	gwtc9xxxb_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	gwtc9xxxb_ts->early_suspend.suspend = gwtc9xxxb_ts_suspend;
	gwtc9xxxb_ts->early_suspend.resume = gwtc9xxxb_ts_resume;
	register_early_suspend(&gwtc9xxxb_ts->early_suspend);
#endif

	gwtc9xxxb_ts_reset(gwtc9xxxb_ts);
	mdelay(10);
	enable_irq(gwtc9xxxb_ts->client->irq);
	printk("-----gwtc9xxxb_ts probed -----\n");

	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, gwtc9xxxb_ts);
exit_irq_request_failed:
	cancel_work_sync(&gwtc9xxxb_ts->pen_event_work);
	destroy_workqueue(gwtc9xxxb_ts->ts_workqueue);
exit_create_singlethread:
	i2c_set_clientdata(client, NULL);
	kfree(gwtc9xxxb_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int __devexit gwtc9xxxb_ts_remove(struct i2c_client *client)
{
	struct gwtc9xxxb_ts_data *gwtc9xxxb_ts = i2c_get_clientdata(client);
	unregister_early_suspend(&gwtc9xxxb_ts->early_suspend);
	free_irq(client->irq, gwtc9xxxb_ts);
	input_unregister_device(gwtc9xxxb_ts->input_dev);
	kfree(gwtc9xxxb_ts);
	cancel_work_sync(&gwtc9xxxb_ts->pen_event_work);
	destroy_workqueue(gwtc9xxxb_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id gwtc9xxxb_ts_id[] = {
	{ GWTC9XXXB_NAME, 0 },
};
MODULE_DEVICE_TABLE(i2c, gwtc9xxxb_ts_id);

static struct i2c_driver gwtc9xxxb_ts_driver = {
	.probe		= gwtc9xxxb_ts_probe,
	.remove		= __devexit_p(gwtc9xxxb_ts_remove),
	.id_table	= gwtc9xxxb_ts_id,
	.driver	= {
		.name	= GWTC9XXXB_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init gwtc9xxxb_ts_init(void)
{
	return i2c_add_driver(&gwtc9xxxb_ts_driver);
}

static void __exit gwtc9xxxb_ts_exit(void)
{
	i2c_del_driver(&gwtc9xxxb_ts_driver);
}

module_init(gwtc9xxxb_ts_init);
module_exit(gwtc9xxxb_ts_exit);

MODULE_AUTHOR("<bcjia@ingenic.cn>");
MODULE_DESCRIPTION("FocalTech gwtc9xxxb TouchScreen driver");
MODULE_LICENSE("GPL");
