/* drivers/input/touchscreen/zet6221_ts.c
 *
 * FocalTech zet6221 TouchScreen driver.
 *
 * Copyright (c) 2010  Focal tech Ltd.
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
#include "zet6221_ts.h"
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/tsc.h>

#include <linux/interrupt.h>



extern int __init zet622x_downloader( struct i2c_client *client );
u8 pc[8];

struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- contact; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u8 weight[CFG_MAX_TOUCH_POINTS];	/*touch weight */
	u16 pressure;
	u8 touch_point;
	u16 touch_num;
};

struct zet6221_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *wake;
};

struct zet6221_ts_data {
	unsigned int irq;
	u16 x_max;
	u16 y_max;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_event event;
	struct jztsc_platform_data *pdata;
	struct zet6221_gpio gpio;
	struct regulator *power;
	struct early_suspend early_suspend;
	struct work_struct  work;
	struct delayed_work release_work;
	struct workqueue_struct *workqueue;
};

static void zet6221_ts_release(struct zet6221_ts_data *data);
static void zet6221_ts_reset(struct zet6221_ts_data *ts);

s32 zet6221_i2c_read_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = I2C_M_RD;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
}

s32 zet6221_i2c_write_tsdata(struct i2c_client *client, u8 *data, u8 length)
{
	struct i2c_msg msg;
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = length;
	msg.buf = data;
	return i2c_transfer(client->adapter,&msg, 1);
}

/*release the point*/
static void zet6221_ts_release(struct zet6221_ts_data *data)
{
	/* input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0); */
	input_mt_sync(data->input_dev);
	input_sync(data->input_dev);
}


static void tsc_release(struct work_struct *work)
{
    struct zet6221_ts_data *ts = 
        container_of(work, struct zet6221_ts_data, release_work.work);
    zet6221_ts_release(ts);
}

/*Read touch point information when the interrupt  is asserted.*/
static int zet6221_read_touchdata(struct zet6221_ts_data *data)
{
	struct ts_event *event = &data->event;
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1, i = 0;

#if 0
	if (delayed_work_pending(&data->release_work)) {
		cancel_delayed_work_sync(&data->release_work);
	}
#endif
	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = 0;
	ret = zet6221_i2c_read_tsdata(data->client, buf, POINT_READ_BUF);
	if (ret < 0) {
		//schedule_delayed_work(&data->release_work, 0);
		zet6221_ts_release(data);
		zet6221_ts_reset(data);
		return ret;
	}

	if (buf[0] != 0x3c) {
		//schedule_delayed_work(&data->release_work, 0);
		zet6221_ts_release(data);
		return 1;
	}

	event->touch_num = (buf[1] << 8) | buf[2];
	if (event->touch_num == 0){
		zet6221_ts_release(data);
		return 1;
	}

	event->touch_point = event->touch_num;

	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		event->au16_x[i] = ((buf[3+4*i] >> 4) << 8) | (buf[(3+4*i)+1]);
		event->au16_y[i] = ((buf[3+4*i] & 0xf) << 8) | (buf[(3+4*i)+2]);
		event->au8_finger_id[i] = i;
		event->weight[i] = 250;
	}
	event->pressure = FT_PRESS;

	return 0;
}
/*
 *report the point information
 */
static int zet6221_report_value(struct zet6221_ts_data *data)
{
	struct ts_event *event = &data->event;
	int i = 0;
	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {

		if(event->au16_x[i] > data->x_max || event->au16_y[i] > data->y_max)
			continue;

#ifdef CONFIG_TSC_SWAP_XY
		tsc_swap_xy(&(event->au16_x[i]),&(event->au16_y[i]));
#endif

#ifdef CONFIG_TSC_SWAP_X
		tsc_swap_x(&(event->au16_x[i]),data->x_max);
#endif

#ifdef CONFIG_TSC_SWAP_Y
		tsc_swap_y(&(event->au16_y[i]),data->y_max);
#endif
#if 0
        printk("event->au16_x[%d], event->au16_y[%d] = (%d, %d)\n",
                i, i, event->au16_x[i], event->au16_y[i]);
#endif
		input_report_abs(data->input_dev, ABS_MT_POSITION_X,
				event->au16_x[i]);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
				event->au16_y[i]);
		 input_report_abs(data->input_dev, ABS_MT_TRACKING_ID,
				event->au8_finger_id[i]);
		input_report_abs(data->input_dev,ABS_MT_TOUCH_MAJOR,
				event->weight[i]);
		input_report_abs(data->input_dev,ABS_MT_WIDTH_MAJOR,
				event->weight[i]);
		input_mt_sync(data->input_dev);
	}

    input_sync(data->input_dev);

	return 0;
}

static void tsc_work_handler(struct work_struct *work)
{
	struct zet6221_ts_data *zet6221_ts = container_of(work, struct zet6221_ts_data,work);

	if (likely(gpio_get_value(zet6221_ts->gpio.irq->num) == 0))
		if (likely(zet6221_read_touchdata(zet6221_ts) == 0))
				zet6221_report_value(zet6221_ts);

	enable_irq(zet6221_ts->irq);
}
/*The zet6221 device will signal the host about TRIGGER_FALLING.
*Processed when the interrupt is asserted.
*/
static irqreturn_t zet6221_ts_interrupt(int irq, void *dev_id)
{
	struct zet6221_ts_data *zet6221_ts = dev_id;

	disable_irq_nosync(zet6221_ts->irq);
	queue_work(zet6221_ts->workqueue, &zet6221_ts->work);

	return IRQ_HANDLED;
}

static void zet6221_gpio_init(struct zet6221_ts_data *ts, struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct jztsc_platform_data *pdata = dev->platform_data;

	ts->gpio.irq = &pdata->gpio[0];
	ts->gpio.wake = &pdata->gpio[1];

	if (gpio_request_one(ts->gpio.irq->num,
			     GPIOF_DIR_IN, "zet6221_irq")) {
		dev_err(dev, "no irq pin available\n");
		ts->gpio.irq->num = -EBUSY;
	}
	if (gpio_request_one(ts->gpio.wake->num,
			     ts->gpio.wake->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "zet6221_shutdown")) {
		dev_err(dev, "no shutdown pin available\n");
		ts->gpio.wake->num = -EBUSY;
	}
}


static int zet6221_ts_power_on(struct zet6221_ts_data *ts)
{
	if (ts->power)
		return regulator_enable(ts->power);

	return 0;
}

static int zet6221_ts_power_off(struct zet6221_ts_data *ts)
{
	if (ts->power)
		return regulator_disable(ts->power);

	return 0;
}

static void zet6221_ts_reset(struct zet6221_ts_data *ts)
{
	set_pin_status(ts->gpio.wake, 1);
	msleep(1);
	set_pin_status(ts->gpio.wake, 0);
	msleep(10);
	set_pin_status(ts->gpio.wake, 1);
	msleep(20);
	printk("reset ts\n");
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zet6221_ts_resume(struct early_suspend *handler);
static void zet6221_ts_suspend(struct early_suspend *handler);
#endif

u8 zet6221_ts_get_report_mode_t(struct i2c_client *client)
{
	u8 ts_report_cmd[1] = {0xb2};
	u8 ts_in_data[17] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	int ret;
	int i;
	int ResolutionY, ResolutionX, xyExchange,FingerNum, bufLength, KeyNum;
	int inChargerMode;
	
	ret=zet6221_i2c_write_tsdata(client, ts_report_cmd, 1);

	if (ret > 0)
	{
			//udelay(10);
			msleep(10);
			printk ("=============== zet6221_ts_get_report_mode_t ===============\n");
			ret=zet6221_i2c_read_tsdata(client, ts_in_data, 17);
			
			if(ret > 0)
			{
				
				for(i=0;i<8;i++)
				{
					pc[i]=ts_in_data[i] & 0xff;
					printk("0x%x ", pc[i]);
				}
				printk("--------------------------------------\n");
				
				if(pc[3] != 0x08)
				{
					printk ("=============== zet6221_ts_get_report_mode_t report error ===============\n");
					return 0;
				} else {

					return 1;
					printk("--------------------------------------\n");

				}

				xyExchange = (ts_in_data[16] & 0x8) >> 3;
				if(xyExchange == 1)
				{
					ResolutionY= ts_in_data[9] & 0xff;
					ResolutionY= (ResolutionY << 8)|(ts_in_data[8] & 0xff);
					ResolutionX= ts_in_data[11] & 0xff;
					ResolutionX= (ResolutionX << 8) | (ts_in_data[10] & 0xff);
				}
				else
				{
					ResolutionX = ts_in_data[9] & 0xff;
					ResolutionX = (ResolutionX << 8)|(ts_in_data[8] & 0xff);
					ResolutionY = ts_in_data[11] & 0xff;
					ResolutionY = (ResolutionY << 8) | (ts_in_data[10] & 0xff);
				}
					
				FingerNum = (ts_in_data[15] & 0x7f);
				KeyNum = (ts_in_data[15] & 0x80);
				inChargerMode = (ts_in_data[16] & 0x2) >> 1;

				if(KeyNum==0)
					bufLength  = 3+4*FingerNum;
				else
					bufLength  = 3+4*FingerNum+1;
				
			}else
			{
				printk ("=============== zet6221_ts_get_report_mode_t READ ERROR ===============\n");
				return 0;
			}
							
	}else
	{
		printk ("=============== zet6221_ts_get_report_mode_t WRITE ERROR ===============\n");
		return 0;
	}
	return 1;
}



static int zet6221_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct jztsc_platform_data *pdata =
	    (struct jztsc_platform_data *)client->dev.platform_data;
	struct zet6221_ts_data *zet6221_ts;
	struct input_dev *input_dev;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	zet6221_ts = kzalloc(sizeof(struct zet6221_ts_data), GFP_KERNEL);

	if (!zet6221_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, zet6221_ts);

	zet6221_gpio_init(zet6221_ts, client);

	client->dev.init_name=client->name;
	zet6221_ts->power = regulator_get(&client->dev, "vtsc");
	if (IS_ERR(zet6221_ts->power)) {
		dev_warn(&client->dev, "get regulator failed\n");
	}
	zet6221_ts_power_on(zet6221_ts);

	INIT_WORK(&zet6221_ts->work, tsc_work_handler);
	//INIT_DELAYED_WORK(&zet6221_ts->release_work, tsc_release);
	zet6221_ts->workqueue = create_singlethread_workqueue("zet6221_tsc");

	client->irq = gpio_to_irq(zet6221_ts->gpio.irq->num);
	err = request_irq(client->irq, zet6221_ts_interrupt,
			    IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			  "zet6221_ts", zet6221_ts);
	if (err < 0) {
		dev_err(&client->dev, "zet6221_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	zet6221_ts->irq = client->irq;
	zet6221_ts->client = client;
	zet6221_ts->pdata = pdata;
	zet6221_ts->x_max = pdata->x_max - 1;
	zet6221_ts->y_max = pdata->y_max - 1;

	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	zet6221_ts->input_dev = input_dev;
	
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID,input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev,
			ABS_MT_POSITION_X, 0, zet6221_ts->x_max, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_POSITION_Y, 0, zet6221_ts->y_max, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_TOUCH_MAJOR, 0, 250, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
    input_set_abs_params(input_dev, 
            ABS_MT_TRACKING_ID, 0, 4, 0, 0);
    input_set_abs_params(input_dev, 
            ABS_PRESSURE, 0, 256, 0, 0);


	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);

	input_dev->name = ZET6621_tsc_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"zet6221_ts_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	
	zet6221_ts_reset(zet6221_ts);

	if(zet622x_downloader(client)<=0) {
		printk("zet6221_downloader failed\n");
		printk("zet6221_downloader failed\n");
		zet6221_ts_reset(zet6221_ts);
		zet6221_ts_get_report_mode_t(client);
	}

	printk("zet6221_downloader ok\n");
	printk("zet6221_downloader ok\n");

	/*make sure CTP already finish startup process */
	zet6221_ts_reset(zet6221_ts);
	zet6221_ts_get_report_mode_t(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	zet6221_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	zet6221_ts->early_suspend.suspend = zet6221_ts_suspend;
	zet6221_ts->early_suspend.resume	= zet6221_ts_resume;
	register_early_suspend(&zet6221_ts->early_suspend);
#endif
	
	enable_irq(client->irq);

	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, zet6221_ts);
	gpio_free(zet6221_ts->gpio.wake->num);

exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(zet6221_ts);
	zet6221_ts_power_off(zet6221_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zet6221_ts_suspend(struct early_suspend *handler)
{
	struct zet6221_ts_data *ts;

	ts = container_of(handler, struct zet6221_ts_data,
						early_suspend);
	disable_irq_nosync(ts->client->irq);

	flush_workqueue(ts->workqueue);
	cancel_work_sync(&ts->work);
	cancel_delayed_work_sync(&ts->release_work);

	zet6221_ts_power_off(ts);
	set_pin_status(ts->gpio.wake, 0);
	dev_info(&ts->client->dev, "zet6221 suspend\n");
}

static void zet6221_ts_resume(struct early_suspend *handler)
{
	struct zet6221_ts_data *ts;

    ts = container_of(handler, struct zet6221_ts_data,
						early_suspend);

    zet6221_ts_power_on(ts);
    msleep(5);
    zet6221_ts_reset(ts);
    msleep(10);

    enable_irq(ts->client->irq);
}
#endif

static int __devexit zet6221_ts_remove(struct i2c_client *client)
{
	struct zet6221_ts_data *zet6221_ts;
	zet6221_ts = i2c_get_clientdata(client);
	input_unregister_device(zet6221_ts->input_dev);
	gpio_free(zet6221_ts->gpio.wake->num);
	free_irq(client->irq, zet6221_ts);
	i2c_set_clientdata(client, NULL);
	zet6221_ts_power_off(zet6221_ts);
	regulator_put(zet6221_ts->power);
	kfree(zet6221_ts);
	return 0;
}

static const struct i2c_device_id zet6221_ts_id[] = {
	{ZET6621_tsc_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, zet6221_ts_id);

static struct i2c_driver zet6221_ts_driver = {
	.probe = zet6221_ts_probe,
	.remove = __devexit_p(zet6221_ts_remove),
	.id_table = zet6221_ts_id,
	.driver = {
		   .name = ZET6621_tsc_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init zet6221_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&zet6221_ts_driver);
	if (ret) {
		printk(KERN_WARNING "Adding zet6221 driver failed "
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			zet6221_ts_driver.driver.name);
	}
	return ret;
}

static void __exit zet6221_ts_exit(void)
{
	i2c_del_driver(&zet6221_ts_driver);
}

module_init(zet6221_ts_init);
module_exit(zet6221_ts_exit);

MODULE_AUTHOR("<hlguo>");
MODULE_DESCRIPTION("FocalTech zet6221 TouchScreen driver");
MODULE_LICENSE("GPL");
