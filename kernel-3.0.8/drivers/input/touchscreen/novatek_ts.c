#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <soc/gpio.h>
#include <gpio.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>
#include <linux/tsc.h>

#include "novatek_ts.h"

struct novatek_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *reset;
};

struct novatek_ts_data
{
	uint16_t addr;
	int      use_irq;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	struct novatek_platform_data *pdata;
	struct novatek_gpio	gpio;
	unsigned int abs_x_max;
	unsigned int abs_y_max;
	unsigned int max_touch_num;
	struct regulator	*power;
	struct mutex lock;
	//unsigned int irq;
	struct hrtimer timer;
	char	phys[32];
};
static int i2c_read_bytes(struct novatek_ts_data *ts, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret = -1;
	int retries = 0;

	msgs[0].flags = 0;
	msgs[0].addr =ts->client->addr;
	msgs[0].len = 1;
	msgs[0].buf = &buf[0];

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr =ts->client->addr;
	msgs[1].len = len - 1;
	msgs[1].buf = &buf[1];

	while(retries < 5)
	{
		ret = i2c_transfer(ts->client->adapter, msgs, 2);
		if(ret == 2) break;
		retries++;
	}	
	/*ret = i2c_transfer(client->adapter, msgs, 2);
	if(ret != 2)
	{
		printk(KERN_ALERT "output ret with err-----%d\n", ret);
	}*/
	return ret;
}
#if 0
static int i2c_read_bytes(struct novatek_ts_data *ts, char *rxdata, int length)
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
		pr_err("msg1 %s i2c read error: %d\n", __func__, ret);
		return ret;
	}

	ret = i2c_transfer(ts->client->adapter, msg2, 1);
	mutex_unlock(&ts->lock);
	if (ret < 0){
		pr_err("msg2 %s i2c read error: %d\n", __func__, ret);
		return ret;
	}

	return ret;
}
#endif

static enum hrtimer_restart novatek_ts_timer_func(struct hrtimer *timer)
{
	struct novatek_ts_data *ts = container_of(timer, struct novatek_ts_data, timer);
	queue_work(novatek_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, (POLL_TIME + 6) * 1000000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}
static void novatek_ts_release(struct novatek_ts_data *data)
{
	/* input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0); */
	input_mt_sync(data->input_dev);
	input_sync(data->input_dev);
}

static void novatek_ts_work_func(struct work_struct *work)
{
	int ret = -1;
	//int tmp = 0;
	uint8_t point_data[1 + IIC_BYTENUM * MAX_FINGER_NUM] = {0};
	//uint8_t check_sum = 0;
	//uint16_t finger_current = 0;
	//uint16_t finger_bit = 0;
	//unsigned int count = 0;
	//unsigned int point_count = 0;
	unsigned int position = 0;
	uint8_t track_id[MAX_FINGER_NUM] = {0};
	unsigned int input_x = 0;
	unsigned int input_y = 0;
	unsigned int input_w = 0;
	unsigned char index = 0;
	unsigned char touch_num = 0;
	//unsigned char touch_check = 0;
	unsigned int rnum = 0;

	struct novatek_ts_data *ts = container_of(work, struct novatek_ts_data, work);
	point_data[0] = READ_COOR_ADDR;

	ret = i2c_read_bytes(ts, point_data, sizeof(point_data) / sizeof(point_data[0]));

	touch_num = MAX_FINGER_NUM;

#if 0
	printk(KERN_ALERT "touch number = %d\n", touch_num);
	for(rnum = 0; rnum < 21; rnum++)
	{
		printk(KERN_ALERT "output point_data = %d\n", point_data[rnum]);
	}
#endif
	for(index = 0; index < MAX_FINGER_NUM; index++)
	{
		position = 1 + IIC_BYTENUM * index;
		if((point_data[position] & 0x03) == 0x03)
		{
			touch_num--;
		}
	}



	if(touch_num)
	{
		for(index = 0; index < touch_num; index++)
		{
			position = 1 + IIC_BYTENUM * index;
			track_id[index] = (unsigned int)(point_data[position] >> 3) - 1;
			input_x = (unsigned int)(point_data[position + 1] << 4) + \
			          (unsigned int)(point_data[position + 3] >> 4);
			input_y = (unsigned int)(point_data[position + 2] << 4) + \
			          (unsigned int)(point_data[position + 3] & 0x0f);
			input_w = (unsigned int)(point_data[position + 4]) + 127;
			printk("%d, %d\n", input_x, input_y);

			//input_x = input_x * SCREEN_MAX_WIDTH / (TOUCH_MAX_WIDTH);
			//input_y = input_y * SCREEN_MAX_HEIGHT / (TOUCH_MAX_HEIGHT);

			if((input_x > ts->abs_x_max) || (input_y > ts->abs_y_max))
			{
				continue;
			}
#if 0
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, track_id[index]);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, TOOL_PRESSURE);
#endif

#if 1
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
					 input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
					 input_y);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID,
					 track_id[index]);
			input_report_abs(ts->input_dev,ABS_MT_TOUCH_MAJOR,
					 1);
			input_report_abs(ts->input_dev,ABS_MT_WIDTH_MAJOR,
					 1);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, TOOL_PRESSURE);

#endif
			input_mt_sync(ts->input_dev);
		}
		input_sync(ts->input_dev);
	}
	else
	{
		printk("has not touch point\n");
		novatek_ts_release(ts);
		//input_report_key(ts->input_dev, BTN_TOUCH, 0);
		//input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		//input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		//input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		//input_mt_sync(ts->input_dev);
		//input_sync(ts->input_dev);
	}

	/*if(ts->use_irq)
	{
		enable_irq(ts->client->irq);
	}*/
	//printk(KERN_ALERT "issue interrupt from novatek tp--------\n");
}

static irqreturn_t novatek_ts_irq_handler(int irq, void *dev_id)
{
	struct novatek_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(novatek_wq, &ts->work);
	enable_irq(ts->client->irq);
	return IRQ_HANDLED;
}

static int novatek_ts_power_off(struct novatek_ts_data *ts)
{
	if (ts->power) {
		printk("novatek power off\n");
		return regulator_disable(ts->power);
	}
	return 0;
}

static int novatek_ts_power_on(struct novatek_ts_data *ts)
{
	if (ts->power) {
		printk("novatek power on\n");
		return regulator_enable(ts->power);
	}
	return 0;
}

static void novatek_ts_reset(struct novatek_ts_data *ts)
{
	set_pin_status(ts->gpio.reset, 1);
	msleep(50);
	set_pin_status(ts->gpio.reset, 0);
	msleep(50);
	set_pin_status(ts->gpio.reset, 1);
	msleep(50);
}

static void novatek_gpio_init(struct novatek_ts_data *ts)
{
	struct device *dev = &ts->client->dev;
	struct novatek_platform_data *pdata = dev->platform_data;;

	ts->gpio.irq = &pdata->gpio[0];
	ts->gpio.reset = &pdata->gpio[1];

	if (gpio_request_one(ts->gpio.irq->num,
			     GPIOF_DIR_IN, "novatek_irq")) {
		dev_err(dev, "no irq pin available\n");
		ts->gpio.irq->num = -EBUSY;
	}

	if (gpio_request_one(ts->gpio.reset->num,
			     ts->gpio.reset->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "novatek_reset")) {
		dev_err(dev, "no reset pin available\n");
		ts->gpio.reset->num = -EBUSY;
	}
}

static int novatek_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	int retry = 0;
	int err = 0;
	uint8_t test_data[7] = {0x00,};
	struct novatek_ts_data *novatek_ts;
	struct novatek_platform_data *pdata = client->dev.platform_data;

	printk(KERN_ALERT "novatek probe function start.-------------------\n");

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		printk("Must have I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	printk(KERN_ALERT "allocate ts space-------\n");

	novatek_ts = kzalloc(sizeof(struct novatek_ts_data), GFP_KERNEL);
	if(novatek_ts == NULL)
	{
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	mutex_init(&novatek_ts->lock);
	novatek_ts->client = client;
	i2c_set_clientdata(client, novatek_ts);

	novatek_gpio_init(novatek_ts);
	printk(KERN_INFO "successfully initial\n");

#if 1
	novatek_ts->power = regulator_get(&client->dev, "vtsc");
	if (IS_ERR(novatek_ts->power)) {
		dev_warn(&client->dev, "get regulator power failed\n");
		novatek_ts->power = NULL;
	}
	novatek_ts_power_on(novatek_ts);
#endif

	client->irq = gpio_to_irq(novatek_ts->gpio.irq->num);
	err = request_irq(client->irq, novatek_ts_irq_handler,
			  IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			 client->name, novatek_ts);
	if (err < 0) {
		dev_err(&client->dev, "request irq failed\n");
		goto exit_irq_request_failed;
	}

	disable_irq(novatek_ts->client->irq);
	novatek_ts->use_irq = 1;

	INIT_WORK(&novatek_ts->work, novatek_ts_work_func);
	novatek_ts->client = client;
	novatek_ts->pdata = pdata;
	novatek_ts->abs_x_max = pdata->x_max - 1;
	novatek_ts->abs_y_max = pdata->y_max - 1;

	printk("novatek_ts->abs_x_max:%d\n",novatek_ts->abs_x_max);
	printk("novatek_ts->abs_y_max:%d\n",novatek_ts->abs_y_max);

	novatek_ts->input_dev = input_allocate_device();
	if(novatek_ts->input_dev == NULL)
	{
		ret = -ENOMEM;
		printk("Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
#if 0
	novatek_ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	novatek_ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	novatek_ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);

	input_set_abs_params(novatek_ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(novatek_ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(novatek_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(novatek_ts->input_dev, ABS_MT_POSITION_X, 0, novatek_ts->abs_x_max, 0, 0);
	input_set_abs_params(novatek_ts->input_dev, ABS_MT_POSITION_Y, 0, novatek_ts->abs_y_max, 0, 0);
	input_set_abs_params(novatek_ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(novatek_ts->input_dev, ABS_MT_PRESSURE, 0, TOOL_PRESSURE, 0, 0);

#endif

#if 1
	set_bit(INPUT_PROP_DIRECT, novatek_ts->input_dev->propbit);
	set_bit(ABS_MT_POSITION_X, novatek_ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, novatek_ts->input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID,novatek_ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, novatek_ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, novatek_ts->input_dev->absbit);

	input_set_abs_params(novatek_ts->input_dev,
			ABS_MT_POSITION_X, 0, novatek_ts->abs_x_max, 0, 0);
	input_set_abs_params(novatek_ts->input_dev,
			ABS_MT_POSITION_Y, 0, novatek_ts->abs_y_max, 0, 0);
	input_set_abs_params(novatek_ts->input_dev,
			ABS_MT_TOUCH_MAJOR, 0, 250, 0, 0);
	input_set_abs_params(novatek_ts->input_dev,
			ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	set_bit(EV_KEY, novatek_ts->input_dev->evbit);
	set_bit(EV_ABS, novatek_ts->input_dev->evbit);

#endif 
	novatek_ts->input_dev->name = novatek_ts_name;
#if 0
	sprintf(novatek_ts->phys, "input/novatek_ts");
	novatek_ts->input_dev->name = novatek_ts_name;
	novatek_ts->input_dev->phys = novatek_ts->phys;
	novatek_ts->input_dev->id.bustype = BUS_I2C;
	novatek_ts->input_dev->id.vendor = 0xDEAD;
	novatek_ts->input_dev->id.product = 0xBEEF;
	novatek_ts->input_dev->id.version = 111028;
#endif
	ret = input_register_device(novatek_ts->input_dev);
	if(ret)
	{
		printk("Probe: unabe to register %s input device\n", novatek_ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	if(!novatek_ts->use_irq)
	{
		printk("can not be here\n");
		hrtimer_init(&novatek_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		novatek_ts->timer.function = novatek_ts_timer_func;
		hrtimer_start(&novatek_ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	novatek_ts_reset(novatek_ts);
	mdelay(10);
	enable_irq(client->irq);
	printk("-----novatek_ts probed -----\n");
	return 0;

err_input_register_device_failed:
	input_free_device(novatek_ts->input_dev);
	
err_input_dev_alloc_failed:
	free_irq(client->irq, novatek_ts);
exit_irq_request_failed:
	kfree(novatek_ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static void novatek_ts_remove(struct i2c_client *client)
{
	printk(KERN_ALERT "novatek remove function start.\n");
	struct novatek_ts_data *ts = i2c_get_clientdata(client);

	if(ts && ts->use_irq)
	{
		free_irq(client->irq, ts);
	}
	else if(ts)
	{
		hrtimer_cancel(&ts->timer);
	}

	dev_notice(&client->dev, "The driver is removing....\n");
	i2c_set_clientdata(client, NULL);
	input_unregister_device(ts->input_dev);
	kfree(ts);
}

static const struct i2c_device_id novatek_ts_id[] =
{
	{NOVATEK_I2C_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, novatek_ts_id);

static struct i2c_driver novatek_ts_driver =
{
	.probe = novatek_ts_probe,
	.remove = __devexit_p(novatek_ts_remove),
	.id_table = novatek_ts_id,
	.driver =
	 {
		.name = NOVATEK_I2C_NAME,
		.owner = THIS_MODULE,
	 },
};

//**************************************************************************************//
//  function 	:	module initial for novatek
//  reuturn     :	0: failed
//**************************************************************************************//
static int __init novatek_ts_init(void)
{
	int ret;
	
	novatek_wq = create_workqueue("novatek_wq");
	if(!novatek_wq)
	{
		printk(KERN_ALERT "create workqueue failed\n");
		return -ENOMEM;
	}

	ret = i2c_add_driver(&novatek_ts_driver);
	if(ret)
	{
		printk(KERN_ALERT "adding novatek driver failed.-----\n");
	}
	else
	{
		printk(KERN_ALERT "novatek touchscreen inited--------\n");
	}

	return ret;
}

//**************************************************************************************//
//  function 	:	module exit for novatek
//**************************************************************************************//
static void __exit novatek_ts_exit(void)
{
	printk(KERN_ALERT "Touchscreen driver of novatek exited.\n");
	i2c_del_driver(&novatek_ts_driver);
	if(novatek_wq)
	{
		destroy_workqueue(novatek_wq);
	}
}

module_init(novatek_ts_init);
module_exit(novatek_ts_exit);
MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");

