/**
 * drivers/input/touchscreen/ft6x0x_ts.c
 *
 * FocalTech ft6x0x TouchScreen driver.
 *
 * Copyright (c) 2013 Ingenic Semiconductor.
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
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/interrupt.h>
#include <linux/i2c/ft6x0x_ts.h>

#ifdef CONFIG_KEY_SPECIAL_POWER_KEY
extern unsigned int bkl_flag;
extern int set_backlight_light(int on);

#endif
#define CFG_MAX_TOUCH_POINTS	5

#define PRESS_MAX	0xFF
#define FT_PRESS	0x7F

#define FTS_NAME	"ft6x0x_tsc"

#define FT_MAX_ID	      0x0F
#define FT_TOUCH_STEP	      6
#define FT_TOUCH_X_H_POS      3
#define FT_TOUCH_X_L_POS      4
#define FT_TOUCH_Y_H_POS      5
#define FT_TOUCH_Y_L_POS      6
#define FT_TOUCH_EVENT_POS    3
#define FT_TOUCH_ID_POS       5
#define FT_TOUCH_WEIGHT	      7

#define POINT_READ_BUF	(3 + FT_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)

/*register address*/
#define FT6X0X_REG_FW_VER	0xA6
#define FT6X0X_REG_POINT_RATE	0x88
#define FT6X0X_REG_THGROUP	0x80
#define FT6X0X_REG_PMODE	0xA5	/* Power Consume Mode */
#define PMODE_HIBERNATE		0x03
#define PMODE_ACTIVE		0x00
#define PMODE_MONITOR		0x01

#define FTTP_THREAD_MODE
#ifdef  FTTP_THREAD_MODE
static DECLARE_WAIT_QUEUE_HEAD(waiter);
#define RTPM_PRIO_TPD  		(97)
#endif

static bool finger_pos_print = false;
module_param_named(ts_debug, finger_pos_print, bool, S_IRUGO | S_IWUSR);

#define pr_pos(fmt, ...) \
	do { \
		if (finger_pos_print) { \
			printk(fmt,##__VA_ARGS__); \
		} \
	} while (0)

struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- contact; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u8 weight[CFG_MAX_TOUCH_POINTS];	/*touch weight */
	u16 pressure;
	u8 touch_point;
	u8 touch_num;
};

struct ft6x0x_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *wake;
};

struct ft6x0x_ts_data {
	unsigned int irq;
	u16 x_max;
	u16 y_max;
	atomic_t regulator_enabled;
	int is_suspend;
	struct i2c_client *client;
	struct mutex lock;
	struct mutex rwlock;
	struct input_dev *input_dev;
	struct ts_event event;
	struct ft6x0x_platform_data *pdata;
	struct ft6x0x_gpio gpio;
	struct regulator *power;
	struct early_suspend early_suspend;
#ifdef FTTP_THREAD_MODE
	int tpd_flag;
	struct task_struct *thread;
#else
	struct work_struct  work;
	struct workqueue_struct *workqueue;
#endif
#ifdef 	CONFIG_KEY_SPECIAL_POWER_KEY
	struct timer_list tp_blk_delay;
#endif
};

static struct ft6x0x_ts_data *ft6x0x_ts = NULL;

struct touch_key_info {
	u16	x_min_search;
	u16	x_max_search;
	u16	y_min_search;
	u16	y_max_search;
	u16	x_min_back;
	u16	x_max_back;
	u16	y_min_back;
	u16	y_max_back;
	u16	x_min_home;
	u16	x_max_home;
	u16	y_min_home;
	u16	y_max_home;
	u16	x_min_menu;
	u16	x_max_menu;
	u16	y_min_menu;
	u16	y_max_menu;
};

static struct touch_key_info tk_info = {
	.x_min_search = 0,
	.x_max_search = 0,
	.y_min_search = 0,
	.y_max_search = 0,
	.x_min_home = 0,
	.x_max_home = 0,
	.y_min_home = 0,
	.y_max_home = 0,
	.x_min_menu = 0,
	.x_max_menu = 0,
	.y_min_menu = 0,
	.y_max_menu = 0,
	.x_min_back = 120,
	.x_max_back = 120,
	.y_min_back = 260,
	.y_max_back = 260,
};

static int tp_key_menu_down = 0;
static int tp_key_home_down = 0;
static int tp_key_back_down = 0;
static int tp_key_search_down = 0;
static int finger_up = true;

static int ft6x0x_touchket_detec(int x, int y)
{
	if(x >= tk_info.x_min_back && x <= tk_info.x_max_back &&
			y >=tk_info.y_min_back && y <= tk_info.y_max_back){
		tp_key_back_down = 1;
		return 0;
	}
	if(x >=tk_info.x_min_home && x <= tk_info.x_max_home &&
			y >=tk_info.y_min_home && y <= tk_info.y_max_home){
		tp_key_home_down = 1;
		return 0;
	}
	if(x >=tk_info.x_min_menu && x <= tk_info.x_max_menu &&
			y >=tk_info.y_min_menu && y <= tk_info.y_max_menu){
		tp_key_menu_down = 1;
		return 0;
	}
	if(x >=tk_info.x_min_search && x <= tk_info.x_max_search &&
			y >=tk_info.y_min_search && y <= tk_info.y_max_search){
		tp_key_search_down = 1;
		return 0;
	}

	return 1;
}

static void ft6x0x_touchkey_report(struct input_dev *dev)
{
	if(tp_key_search_down) {
		input_report_key(dev, KEY_SEARCH, 1);
		input_sync(dev);
	} else if(tp_key_back_down) {
		input_report_key(dev, KEY_BACK, 1);
		input_sync(dev);
	} else if(tp_key_home_down) {
		input_report_key(dev, KEY_HOME, 1);
		input_sync(dev);
	} else if(tp_key_menu_down) {
		input_report_key(dev, KEY_MENU, 1);
		input_sync(dev);
	}
}

static int ft6x0x_touchkey_release(struct input_dev *dev)
{
	if(tp_key_menu_down) {
		tp_key_menu_down = 0;
		input_report_key(dev, KEY_MENU, 0);
		input_sync(dev);
	}

	if(tp_key_home_down) {
		tp_key_home_down = 0;
		input_report_key(dev, KEY_HOME, 0);
		input_sync(dev);
	}

	if(tp_key_search_down) {
		tp_key_search_down = 0;
		input_report_key(dev, KEY_SEARCH, 0);
		input_sync(dev);
	}

	if(tp_key_back_down) {
		tp_key_back_down = 0;
		input_report_key(dev, KEY_BACK, 0);
		input_sync(dev);
	}

	return 0;
}

/*
*ft6x0x_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
static void ft6x0x_ts_release(struct ft6x0x_ts_data *data);
static void ft6x0x_ts_reset(struct ft6x0x_ts_data *ft6x0x_ts);

int ft6x0x_i2c_Read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	struct ft6x0x_ts_data *ft6x0x_ts = i2c_get_clientdata(client);
	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0){
			dev_err(&client->dev, "%s: -i2c read error.\n",
				__func__);
			ft6x0x_ts_release(ft6x0x_ts);
			ft6x0x_ts_reset(ft6x0x_ts);
		}
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0){
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
			ft6x0x_ts_reset(ft6x0x_ts);
		}
	}
	return ret;
}
/*write data by i2c*/
int ft6x0x_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct ft6x0x_ts_data *ft6x0x_ts = i2c_get_clientdata(client);
	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};
	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0){
		dev_err(&client->dev, "%s i2c write error.\n", __func__);
		ft6x0x_ts_release(ft6x0x_ts);
		ft6x0x_ts_reset(ft6x0x_ts);
	}
	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static int ft6x0x_set_reg(struct ft6x0x_ts_data *ft6x0x_ts, u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	mutex_lock(&ft6x0x_ts->rwlock);
	buf[0] = addr;
	buf[1] = para;
	ret = ft6x0x_i2c_Write(ft6x0x_ts->client, buf, 2);
	mutex_unlock(&ft6x0x_ts->rwlock);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}
	return 0;
}
#endif
/*release the point*/
static void ft6x0x_ts_release(struct ft6x0x_ts_data *data)
{
	finger_up = true;
	if (data != NULL && data->input_dev != NULL) {
		ft6x0x_touchkey_release(data->input_dev);
		/* input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0); */
		input_mt_sync(data->input_dev);
		input_sync(data->input_dev);
	}
}

/*Read touch point information when the interrupt  is asserted.*/
static int ft6x0x_read_Touchdata(struct ft6x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	int ret = -1, i = 0;

	uint8_t buf[POINT_READ_BUF] = { 0 };
	uint8_t finger_regs[] = {0x03,0x09,0x0F,0x15,0x1B};

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = 0;
	buf[0] = 0;
	ret = ft6x0x_i2c_Read(data->client, buf, 1, buf, 3);
	if (ret < 0) {
		return ret;
	}
	event->touch_point = buf[2] & 0x0f;

	if (0 == event->touch_point) {
		ft6x0x_ts_release(data);
		return 1;
	}

	if (event->touch_point > sizeof(finger_regs)) {
		if (event->touch_point == 0x0F)
			return 1;
		else {
			dev_info(&data->client->dev, "ERROR : %s --> %d check finger_regs!\n", __func__, __LINE__);
			return 1;
		}
	}

	for (i = 0; i < event->touch_point; i ++) {
		buf[0] = finger_regs[i];
		ft6x0x_i2c_Read(data->client,buf,1,buf,6);
		event->au16_x[i] = (s16) (buf[0] & 0x0F) << 8 | (s16) buf[1];
		event->au16_y[i] = (s16) (buf[2] & 0x0F) << 8 | (s16) buf[3];
		event->au8_touch_event[i] = buf[0] >> 6;
		event->au8_finger_id[i] = (buf[2]) >> 4;
		event->weight[i] = buf[4];
	}
	if(event->au16_y[0] < 260){
		finger_up = false;
	}
	if ((1 == event->touch_point) && finger_up == true) {
		if(!ft6x0x_touchket_detec(event->au16_x[0], event->au16_y[0])) {
			ft6x0x_touchkey_report(data->input_dev);
		}
	}

	event->pressure = FT_PRESS;

	return 0;
}

/*
 *report the point information
 */
static int ft6x0x_report_value(struct ft6x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	int i = 0;
	for (i = 0; i < event->touch_point; i++) {
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
		pr_pos("x[%d]: %d,\ty[%d]: %d\n", i, event->au16_x[i], i, event->au16_y[i]);

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

#ifndef FTTP_THREAD_MODE
static void ft6x0x_work_handler(struct work_struct *work)
{
	struct ft6x0x_ts_data *ft6x0x_ts = container_of(work, struct ft6x0x_ts_data,work);
	int ret = 0;
	ret = ft6x0x_read_Touchdata(ft6x0x_ts);
	if (ret == 0)
		ft6x0x_report_value(ft6x0x_ts);
	enable_irq(ft6x0x_ts->client->irq);
}
#else
static int touch_event_handler(void *unused)
{
	int ret = -1;
	struct ft6x0x_ts_data *ft6x0x_ts = (struct ft6x0x_ts_data *)unused;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD};

	sched_setscheduler(current, SCHED_RR, &param);
	set_freezable();
	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_freezable(waiter, ft6x0x_ts->tpd_flag != 0);
		ft6x0x_ts->tpd_flag = 0;
		set_current_state(TASK_RUNNING);
		ret = ft6x0x_read_Touchdata(ft6x0x_ts);
		if (ret == 0)
			ft6x0x_report_value(ft6x0x_ts);
		enable_irq(ft6x0x_ts->client->irq);
	} while (!kthread_should_stop());

	return 0;
}
#endif

/*The ft6x0x device will signal the host about TRIGGER_FALLING.
*Processed when the interrupt is asserted.
*/
static irqreturn_t ft6x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft6x0x_ts_data *ft6x0x_ts = dev_id;
	disable_irq_nosync(ft6x0x_ts->irq);

	if(ft6x0x_ts->is_suspend == 1)
		return IRQ_HANDLED;
#ifdef CONFIG_KEY_SPECIAL_POWER_KEY
	if(bkl_flag != 1) {
	if(timer_pending(&ft6x0x_ts->tp_blk_delay))
		del_timer_sync(&ft6x0x_ts->tp_blk_delay);

		bkl_flag = 2;
		mod_timer(&ft6x0x_ts->tp_blk_delay, get_jiffies_64()
			+ msecs_to_jiffies(ft6x0x_ts->pdata->blight_off_timer));
	}
#endif
#ifdef FTTP_THREAD_MODE
	 ft6x0x_ts->tpd_flag = 1;
	 wake_up_interruptible(&waiter);
#else
	queue_work(ft6x0x_ts->workqueue, &ft6x0x_ts->work);
#endif
	return IRQ_HANDLED;
}
#ifdef CONFIG_KEY_SPECIAL_POWER_KEY
static void tp_off_blk_timer(unsigned long _data)
{

	set_backlight_light(bkl_flag);

}
#endif
static void ft6x0x_gpio_init(struct ft6x0x_ts_data *ft6x0x_ts, struct i2c_client *client)
{
	struct ft6x0x_platform_data *pdata = ft6x0x_ts->pdata;

	ft6x0x_ts->gpio.irq = &pdata->gpio[0];
	ft6x0x_ts->gpio.wake = &pdata->gpio[1];

	if (gpio_request_one(ft6x0x_ts->gpio.irq->num,
			     GPIOF_DIR_IN, "ft6x0x_irq")) {
		dev_err(&client->dev, "no irq pin available\n");
		ft6x0x_ts->gpio.irq->num = -EBUSY;
	}
	if (gpio_request_one(ft6x0x_ts->gpio.wake->num,
			     ft6x0x_ts->gpio.wake->enable_level
			     ? GPIOF_OUT_INIT_LOW
			     : GPIOF_OUT_INIT_HIGH,
			     "ft6x0x_shutdown")) {
		dev_err(&client->dev, "no shutdown pin available\n");
		ft6x0x_ts->gpio.wake->num = -EBUSY;
	}
}

static int ft6x0x_ts_power_on(struct ft6x0x_ts_data *ft6x0x_ts)
{
	if (!IS_ERR(ft6x0x_ts->power)) {
		if(atomic_cmpxchg(&ft6x0x_ts->regulator_enabled, 0, 1) == 0)
			return regulator_enable(ft6x0x_ts->power);
	} else if (ft6x0x_ts->pdata->power_on)
		ft6x0x_ts->pdata->power_on(&ft6x0x_ts->client->dev);

	return 0;
}

static int ft6x0x_ts_power_off(struct ft6x0x_ts_data *ft6x0x_ts)
{
	if (!IS_ERR(ft6x0x_ts->power)) {
		if (atomic_cmpxchg(&ft6x0x_ts->regulator_enabled, 1, 0))
			return regulator_disable(ft6x0x_ts->power);
	} else if (ft6x0x_ts->pdata->power_off) {
		ft6x0x_ts->pdata->power_off(&ft6x0x_ts->client->dev);
	}

	return 0;
}

static void ft6x0x_ts_reset(struct ft6x0x_ts_data *ft6x0x_ts)
{
	set_pin_status(ft6x0x_ts->gpio.wake, 0);
	msleep(5);
	set_pin_status(ft6x0x_ts->gpio.wake, 1);
	msleep(5);
	set_pin_status(ft6x0x_ts->gpio.wake, 0);
	msleep(15);
}

static int ft6x0x_ts_disable(struct ft6x0x_ts_data *ft6x0x_ts)
{
	int ret = 0;
	int err = 1;
	unsigned char uc_reg_value;
	unsigned char uc_reg_addr;
	int timeout = 0x05;

#ifndef FTTP_THREAD_MODE
	flush_workqueue(ft6x0x_ts->workqueue);
	ret = cancel_work_sync(&ft6x0x_ts->work);
#endif
	err = ft6x0x_set_reg(ft6x0x_ts, FT6X0X_REG_PMODE, PMODE_HIBERNATE);
	uc_reg_addr = FT6X0X_REG_PMODE;
	ret = ft6x0x_i2c_Read(ft6x0x_ts->client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if (err == 0) {
		while ((0x03 != uc_reg_value) && (timeout-- > 0) && (ret == 0)) {
			msleep(5);
			ret = ft6x0x_i2c_Read(ft6x0x_ts->client, &uc_reg_addr, 1, &uc_reg_value, 1);
		}
	}
	ret = ft6x0x_ts_power_off(ft6x0x_ts);
	if (ret < 0) {
		dev_info(&ft6x0x_ts->client->dev, "TSC DISBALE ERROR!\n");
		return ret;
	}
	return 0;
}

static int ft6x0x_ts_enable(struct ft6x0x_ts_data *ft6x0x_ts)
{
	int ret = 0;

	ret = ft6x0x_ts_power_on(ft6x0x_ts);
	if (ret < 0) {
		dev_info(&ft6x0x_ts->client->dev, "TSC ENABLE ERROR!\n");
		return ret;
	}
	mdelay(10);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft6x0x_ts_resume(struct early_suspend *handler);
static void ft6x0x_ts_suspend(struct early_suspend *handler);
#endif

#if defined(CONFIG_FT6X0X_EXT_FUNC)
#include "ft6x06_ex_fun.c"
#endif

static int ft6x0x_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft6x0x_platform_data *pdata = NULL;
	struct input_dev *input_dev;

	int err = 0;
	uint8_t uc_reg_value, uc_reg_addr;

	pdata = (struct ft6x0x_platform_data *)client->dev.platform_data;
	if (!pdata) {
		dev_info(&client->dev, "ERROR : %s --> platform data is NULL! will exit!\n",__func__);
		err = -EINVAL;
		goto exit_pdata_is_null;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft6x0x_ts = kzalloc(sizeof(struct ft6x0x_ts_data), GFP_KERNEL);
	if (!ft6x0x_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	ft6x0x_ts->pdata = pdata;

	ft6x0x_gpio_init(ft6x0x_ts, client);

	i2c_set_clientdata(client, ft6x0x_ts);

	ft6x0x_ts->power = regulator_get(&client->dev, "vtsc");
	if (IS_ERR(ft6x0x_ts->power)) {
		dev_warn(&client->dev, "get regulator vtsc failed, try board power interface.\n");
		if (pdata->power_init) {
			pdata->power_init(&client->dev);
		} else {
			dev_warn(&client->dev, "board power control interface is NULL !\n");
		}
	}

	atomic_set(&ft6x0x_ts->regulator_enabled, 0);

	ft6x0x_ts_power_on(ft6x0x_ts);

	/*make sure CTP already finish startup process */
	ft6x0x_ts_reset(ft6x0x_ts);
	msleep(100);

	/*get some register information */
	uc_reg_addr = FT6X0X_REG_FW_VER;

	err = ft6x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if(err < 0){
		dev_info(&client->dev, "ft6x0x_ts  probe failed\n");
		goto exit_read_reg_failed;
	}

#if defined(CONFIG_FT6X0X_EXT_FUNC)
	err = fts_ctpm_auto_upgrade(client);
	if (err < 0) {
		dev_info(&client->dev, "fts_ctpm_auto_upgrade return %d\n",err);
	}
#endif

	dev_dbg(&client->dev, "[FTS] Firmware version = 0x%x\n", uc_reg_value);

	uc_reg_addr = FT6X0X_REG_POINT_RATE;
	err = ft6x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if(err < 0){
		dev_info(&client->dev, "ft6x0x_ts  probe failed\n");
		goto exit_read_reg_failed;
	}
	dev_dbg(&client->dev, "[FTS] report rate is %dHz.\n",
		uc_reg_value * 10);

	uc_reg_addr = FT6X0X_REG_THGROUP;
	err = ft6x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	if(err < 0){
		dev_info(&client->dev, "ft6x0x_ts  probe failed\n");
		goto exit_read_reg_failed;
	}
	dev_dbg(&client->dev, "[FTS] touch threshold is %d.\n",
		uc_reg_value * 4);

//	ft6x0x_set_reg(ft6x0x_ts, FT6X0X_REG_THCAL, 4);

	mutex_init(&ft6x0x_ts->lock);
	mutex_init(&ft6x0x_ts->rwlock);

	client->dev.init_name=client->name;

#ifndef FTTP_THREAD_MODE
	INIT_WORK(&ft6x0x_ts->work, ft6x0x_work_handler);
	ft6x0x_ts->workqueue = create_singlethread_workqueue("ft6x0x_tsc");
	if (!ft6x0x_ts->workqueue) {
		dev_info(&client->dev, "create_singlethread_workqueue failed!\n");
		goto exit_create_singlethread_workqueue;
	}
#else
	ft6x0x_ts->thread = kthread_run(touch_event_handler, (void *)ft6x0x_ts, "ft6x0x_ts");
	if (IS_ERR(ft6x0x_ts->thread))
		goto exit_create_singlethread_workqueue;
#endif

	client->irq = gpio_to_irq(ft6x0x_ts->gpio.irq->num);
	err = request_irq(client->irq, ft6x0x_ts_interrupt,
			    IRQF_TRIGGER_FALLING | IRQF_DISABLED,
			  "ft6x0x_ts", ft6x0x_ts);
	if (err < 0) {
		dev_err(&client->dev, "ft6x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}

	ft6x0x_ts->irq = client->irq;
	ft6x0x_ts->client = client;
	ft6x0x_ts->x_max = pdata->x_max;
	ft6x0x_ts->y_max = pdata->y_max;
	ft6x0x_ts->is_suspend = 0;

	disable_irq(client->irq);
#ifdef CONFIG_KEY_SPECIAL_POWER_KEY
	setup_timer(&ft6x0x_ts->tp_blk_delay, tp_off_blk_timer,
			(unsigned long)ft6x0x_ts);
#endif
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft6x0x_ts->input_dev = input_dev;

//	set_bit(KEY_MENU, input_dev->keybit);
	set_bit(KEY_BACK, input_dev->keybit);

	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID,input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev,
			ABS_MT_POSITION_X, 0, ft6x0x_ts->x_max, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_POSITION_Y, 0, ft6x0x_ts->y_max, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_TOUCH_MAJOR, 0, 250, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);

	input_dev->name = FTS_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"ft6x0x_ts_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	ft6x0x_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft6x0x_ts->early_suspend.suspend = ft6x0x_ts_suspend;
	ft6x0x_ts->early_suspend.resume	= ft6x0x_ts_resume;
	register_early_suspend(&ft6x0x_ts->early_suspend);
#endif

#if defined(CONFIG_FT6X0X_EXT_FUNC)
	ft6x0x_create_sysfs(client);
#endif

	enable_irq(client->irq);
	return 0;


exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, ft6x0x_ts);

exit_irq_request_failed:
#ifndef FTTP_THREAD_MODE
	destroy_workqueue(ft6x0x_ts->workqueue);
#else
	kthread_stop(ft6x0x_ts->thread);
#endif
exit_read_reg_failed:
exit_create_singlethread_workqueue:
	ft6x0x_ts_power_off(ft6x0x_ts);
	if (!IS_ERR(ft6x0x_ts->power))
		regulator_put(ft6x0x_ts->power);

	gpio_free(ft6x0x_ts->gpio.irq->num);
	gpio_free(ft6x0x_ts->gpio.wake->num);
	i2c_set_clientdata(client, NULL);
	kfree(ft6x0x_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
exit_pdata_is_null:
	return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ft6x0x_ts_suspend(struct early_suspend *handler)
{
	int ret = 0;
	struct ft6x0x_ts_data *ft6x0x_ts;
	ft6x0x_ts = container_of(handler, struct ft6x0x_ts_data,
						early_suspend);

	mutex_lock(&ft6x0x_ts->lock);
	ft6x0x_ts->is_suspend = 1;
	disable_irq_nosync(ft6x0x_ts->client->irq);
	ret = ft6x0x_ts_disable(ft6x0x_ts);
	mutex_unlock(&ft6x0x_ts->lock);
	if (ret < 0)
		dev_dbg(&ft6x0x_ts->client->dev, "[FTS]ft6x0x suspend failed! \n");

	dev_dbg(&ft6x0x_ts->client->dev, "[FTS]ft6x0x suspend\n");
}

static void ft6x0x_ts_resume(struct early_suspend *handler)
{
	int ret = 0;
	struct ft6x0x_ts_data *ft6x0x_ts = container_of(handler, struct ft6x0x_ts_data,
						early_suspend);

	mutex_lock(&ft6x0x_ts->lock);
	ret = ft6x0x_ts_enable(ft6x0x_ts);
	if (ret < 0)
		dev_info(&ft6x0x_ts->client->dev, "-------tsc resume failed!------\n");
	ft6x0x_ts->is_suspend = 0;
	mutex_unlock(&ft6x0x_ts->lock);

	enable_irq(ft6x0x_ts->client->irq);
}
#endif

static int __devexit ft6x0x_ts_remove(struct i2c_client *client)
{
	struct ft6x0x_ts_data *ft6x0x_ts;
	ft6x0x_ts = i2c_get_clientdata(client);
	input_unregister_device(ft6x0x_ts->input_dev);
	gpio_free(ft6x0x_ts->gpio.wake->num);
	free_irq(client->irq, ft6x0x_ts);
	i2c_set_clientdata(client, NULL);
	ft6x0x_ts_power_off(ft6x0x_ts);
	if (!IS_ERR(ft6x0x_ts->power))
		regulator_put(ft6x0x_ts->power);
	kfree(ft6x0x_ts);
	return 0;
}

static const struct i2c_device_id ft6x0x_ts_id[] = {
	{FTS_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ft6x0x_ts_id);

static struct i2c_driver ft6x0x_ts_driver = {
	.probe = ft6x0x_ts_probe,
	.remove = __devexit_p(ft6x0x_ts_remove),
	.id_table = ft6x0x_ts_id,
	.driver = {
		   .name = FTS_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init ft6x0x_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&ft6x0x_ts_driver);
	if (ret) {
		printk(KERN_WARNING "Adding ft6x0x driver failed "
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			ft6x0x_ts_driver.driver.name);
	}
	return ret;
}

static void __exit ft6x0x_ts_exit(void)
{
	i2c_del_driver(&ft6x0x_ts_driver);
}

module_init(ft6x0x_ts_init);
module_exit(ft6x0x_ts_exit);

MODULE_AUTHOR("hfwang@ingenic.cn");
MODULE_DESCRIPTION("FocalTech ft6x0x TouchScreen driver");
MODULE_LICENSE("GPL");
