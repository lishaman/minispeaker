/* 
 * drivers/input/touchscreen/ft5x0x_ts.c
 *
 * FocalTech ft5x0x TouchScreen driver. 
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

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/tsc.h>

#include "ft5x0x_ts.h"

#define P2_PACKET_LEN 13
#define P1_PACKET_LEN 7


struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
#if defined(CONFIG_FT5X0X_MULTITOUCH_FIVE_POINT)
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
#endif
	u16 pen_up;
	u16	pressure;
	u8  touch_point;
};

struct ft5x0x_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *wake;
};

struct ft5x0x_ts_data {
	struct i2c_client *client;
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
	int is_suspend;
#ifdef PENUP_TIMEOUT_TIMER
	struct timer_list penup_timeout_timer;
#endif
#ifdef CONFIG_FT5X0X_DEBUG
	long int count;
#endif
	struct ft5x0x_gpio gpio;
};

static int ft5x0x_i2c_rxdata(struct ft5x0x_ts_data *ts, char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= ts->client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= ts->client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};

	ret = i2c_transfer(ts->client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	return ret;
}

static int ft5x0x_i2c_txdata(struct ft5x0x_ts_data *ts, char *txdata, int length)
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
	ret = i2c_transfer(ts->client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ft5x0x_set_reg(struct ft5x0x_ts_data *ts, u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = ft5x0x_i2c_txdata(ts, buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}

	return 0;
}

static void ft5x0x_ts_release(struct ft5x0x_ts_data *ft5x0x_ts)
{
#ifdef CONFIG_FT5X0X_MULTITOUCH	
	input_mt_sync(ft5x0x_ts->input_dev);
#else
	input_report_abs(ft5x0x_ts->input_dev, ABS_PRESSURE, 0);
	input_report_key(ft5x0x_ts->input_dev, BTN_TOUCH, 0);
#endif
	input_sync(ft5x0x_ts->input_dev);
#ifdef PENUP_TIMEOUT_TIMER
	del_timer(&(ft5x0x_ts->penup_timeout_timer));
#endif
}

static void ft5x0x_chip_reset(struct ft5x0x_ts_data *ts)
{
	set_pin_status(ts->gpio.wake, 1);
	msleep(10);
	set_pin_status(ts->gpio.wake, 0);
	msleep(20);
	set_pin_status(ts->gpio.wake, 1);
}


static int ft5x0x_read_data(struct ft5x0x_ts_data *ts)
{
	struct ts_event *event = &ts->event;
	u8 buf[14] = {0};
	int ret = -1;

#ifdef CONFIG_FT5X0X_MULTITOUCH
	ret = ft5x0x_i2c_rxdata(ts, buf, P2_PACKET_LEN);
#else
	ret = ft5x0x_i2c_rxdata(ts, buf, P1_PACKET_LEN);
#endif
	if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x03;

	if (event->touch_point == 0) {
		ft5x0x_ts_release(ts);
		return 1; 
	}

#ifdef CONFIG_FT5X0X_MULTITOUCH
	switch (event->touch_point) {
		case 2:
			event->x2 = (s16)(buf[9] & 0x0F)<<8 | (s16)buf[10];
			event->y2 = (s16)(buf[11] & 0x0F)<<8 | (s16)buf[12];
		case 1:
			event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
			event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
			break;
		default:
			return -1;
	}
#else
	if (event->touch_point == 1) {
		event->x1 = (s16)(buf[3] & 0x0F)<<8 | (s16)buf[4];
		event->y1 = (s16)(buf[5] & 0x0F)<<8 | (s16)buf[6];
	}
#endif
	event->pressure = 200;

	dev_dbg(&ts->client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
			event->x1, event->y1, event->x2, event->y2);
	return 0;
}

static void ft5x0x_report_value(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;

#ifdef CONFIG_FT5X0X_MULTITOUCH
	switch(event->touch_point) {
#if defined(CONFIG_FT5X0X_MULTITOUCH_FIVE_POINT)
		case 5:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x5);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y5);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
		case 4:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x4);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y4);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
		case 3:
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->x3);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->y3);
			input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(data->input_dev);
#endif
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
#else	/* CONFIG_FT5X0X_MULTITOUCH*/
	if (event->touch_point == 1) {
		input_report_abs(data->input_dev, ABS_X, event->x1);
		input_report_abs(data->input_dev, ABS_Y, event->y1);
		input_report_abs(data->input_dev, ABS_PRESSURE, event->pressure);
	}
	input_report_key(data->input_dev, BTN_TOUCH, 1);
#endif	/* CONFIG_FT5X0X_MULTITOUCH*/
	input_sync(data->input_dev);

	dev_dbg(&data->client->dev, "%s: 1:%d %d 2:%d %d \n", __func__,
			event->x1, event->y1, event->x2, event->y2);
}	/*end ft5x0x_report_value*/

static void ft5x0x_ts_pen_irq_work(struct work_struct *work)
{
	int ret = -1;
	struct ft5x0x_ts_data *ts;
	ts =  container_of(work, struct ft5x0x_ts_data, pen_event_work);

#ifdef CONFIG_FT5X0X_DEBUG
	long int count = 0;
	struct ft5x0x_ts_data *ts;
	ts =  container_of(work, struct ft5x0x_ts_data, pen_event_work);
	count = ts->count;
	printk("==ft5306 read data start(%ld)=\n", count);
#endif

	ret = ft5x0x_read_data(ts);	
	if (ret == 0) {	
		ft5x0x_report_value(ts);
	}
#ifdef CONFIG_FT5X0X_DEBUG
	printk("==ft5306 report data end(%ld)=\n", count++);
#endif
	//msleep(1);
	enable_irq(ts->client->irq);
}

static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x0x_ts_data *ft5x0x_ts = dev_id;
#ifdef CONFIG_FT5X0X_DEBUG
	static long int count = 0;
#endif
	disable_irq_nosync(ft5x0x_ts->client->irq);
	if(ft5x0x_ts->is_suspend == 1)
		return IRQ_HANDLED;

#ifdef CONFIG_FT5X0X_DEBUG
	ft5x0x_ts->count = count;	
	printk("==ctp int(%ld)=\n", count++);	
#endif

	if (!work_pending(&ft5x0x_ts->pen_event_work)) {
		queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts;
	ts =  container_of(handler, struct ft5x0x_ts_data, early_suspend);

#ifdef CONFIG_FT5X0X_DEBUG
	printk("==ft5x0x_ts_suspend=\n");
#endif
	ts->is_suspend = 1;
	disable_irq(ts->client->irq);
	flush_workqueue(ts->ts_workqueue);
	cancel_work_sync(&ts->pen_event_work);
	ft5x0x_set_reg(ts, FT5X0X_REG_PMODE, PMODE_HIBERNATE);
}

static void ft5x0x_ts_resume(struct early_suspend *handler)
{

	struct ft5x0x_ts_data *ts;
	ts = container_of(handler, struct ft5x0x_ts_data, early_suspend); 
#ifdef CONFIG_FT5X0X_DEBUG
	printk("==ft5x0x_ts_resume=\n");
#endif
	ft5x0x_chip_reset(ts);
	ts->is_suspend = 0;
	enable_irq(ts->client->irq);
}
#endif  //CONFIG_HAS_EARLYSUSPEND

static void ft5x0x_gpio_init(struct ft5x0x_ts_data *ts)
{
	struct device *dev = &ts->client->dev;
	struct jztsc_platform_data *pdata = dev->platform_data;;

	ts->gpio.irq = &pdata->gpio[0];
	ts->gpio.wake = &pdata->gpio[1];
		
	if (gpio_request_one(ts->gpio.irq->num,
			     GPIOF_DIR_IN, "ft5x0x_irq")) {
		dev_err(dev, "no irq pin available\n");
		ts->gpio.irq->num = -EBUSY;
	}
	if (gpio_request_one(ts->gpio.wake->num,
			     ts->gpio.wake->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "ft5x0x_wake")) {
		dev_err(dev, "no wake pin available\n");
		ts->gpio.wake->num = -EBUSY;
	}
}

static int ft5x0x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	struct input_dev *input_dev;
	int err = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft5x0x_ts = kzalloc(sizeof(*ft5x0x_ts), GFP_KERNEL);
	if (!ft5x0x_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	ft5x0x_ts->client = client;
	i2c_set_clientdata(client, ft5x0x_ts);
	//i2c_jz_setclk(client, 400*1000);

	ft5x0x_gpio_init(ft5x0x_ts);

	INIT_WORK(&ft5x0x_ts->pen_event_work, ft5x0x_ts_pen_irq_work);
	ft5x0x_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft5x0x_ts->ts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}

	client->irq = gpio_to_irq(ft5x0x_ts->gpio.irq->num);
	err = request_irq(client->irq, ft5x0x_ts_interrupt,
			  IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			  "ft5x0x_ts", ft5x0x_ts);
	if (err < 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(ft5x0x_ts->client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft5x0x_ts->input_dev = input_dev;

#ifdef CONFIG_FT5X0X_MULTITOUCH
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

	input_dev->name		= FT5X0X_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
				"ft5x0x_ts_probe: failed to register input device: %s\n",
				dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef PENUP_TIMEOUT_TIMER
	init_timer(&(ft5x0x_ts->penup_timeout_timer));
	ft5x0x_ts->penup_timeout_timer.data = (unsigned long)ft5x0x_ts;
	ft5x0x_ts->penup_timeout_timer.function  =	(void (*)(unsigned long))ft5x0x_ts_release;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ft5x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft5x0x_ts->early_suspend.suspend = ft5x0x_ts_suspend;
	ft5x0x_ts->early_suspend.resume	= ft5x0x_ts_resume;
	register_early_suspend(&ft5x0x_ts->early_suspend);
#endif

	ft5x0x_chip_reset(ft5x0x_ts);
/*
#if defined(CONFIG_FT5X0X_MULTITOUCH_FIVE_POINT)
	ft5x0x_set_reg(0x88, 0x09); //3-12 报点数
	ft5x0x_set_reg(0x80, 40); //灵敏度
#else

	ft5x0x_set_reg(0x88, 0x06); //5, 6,7,8
	ft5x0x_set_reg(0x80, 40);
#endif
*/
	msleep(10);

	enable_irq(ft5x0x_ts->client->irq);
	printk("End ft5x0x_ts_probe\n");
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, ft5x0x_ts);
exit_irq_request_failed:
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
exit_create_singlethread:
	i2c_set_clientdata(client, NULL);
	kfree(ft5x0x_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	printk("ft5x0x_ts_probe is error\n");
	return err;
}

static int __devexit ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ft5x0x_ts->early_suspend);
	free_irq(client->irq, ft5x0x_ts);
	input_unregister_device(ft5x0x_ts->input_dev);
	kfree(ft5x0x_ts);
	cancel_work_sync(&ft5x0x_ts->pen_event_work);
	destroy_workqueue(ft5x0x_ts->ts_workqueue);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{ FT5X0X_NAME, 0 },
};
MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);

static struct i2c_driver ft5x0x_ts_driver = {
	.probe		= ft5x0x_ts_probe,
	.remove		= __devexit_p(ft5x0x_ts_remove),
	.id_table	= ft5x0x_ts_id,
	.driver	= {
		.name	= FT5X0X_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init ft5x0x_ts_init(void)
{
	printk("-------------------111-----------ft5x0x_ts_init\n");
	return i2c_add_driver(&ft5x0x_ts_driver);
}

static void __exit ft5x0x_ts_exit(void)
{
	i2c_del_driver(&ft5x0x_ts_driver);
}

module_init(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);

MODULE_AUTHOR("<hsgou@ingenic.cn>");
MODULE_DESCRIPTION("FocalTech ft5x0x TouchScreen driver");
MODULE_LICENSE("GPL");
