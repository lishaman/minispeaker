/*
 *  kernel/drivers/input/misc/mc32x0.c - Linux kernel driver for 3-Axis G-Sensor module.
 *  based on MTK code
 *  	date : 	2011-4-8
 *  	revision : 2.5
 *
 *  Copyright (C) 2012 Ingenic Semiconductor Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/miscdevice.h>
#include <linux/input-polldev.h>
#include <linux/input.h>
#include <linux/errno.h>
#include <linux/gsensor.h>
#include <linux/linux_sensors.h>
#include "gsensor_mc32x0.h"

/* register define */
#define MC32X0_XOUT_REG				0x00
#define MC32X0_YOUT_REG				0x01
#define MC32X0_ZOUT_REG				0x02
#define MC32X0_Tilt_Status_REG			0x03
#define MC32X0_Sampling_Rate_Status_REG		0x04
#define MC32X0_Sleep_Count_REG			0x05
#define MC32X0_Interrupt_Enable_REG		0x06
#define MC32X0_Mode_Feature_REG			0x07
#define MC32X0_Sample_Rate_REG			0x08
#define MC32X0_Tap_Detection_Enable_REG		0x09
#define MC32X0_TAP_Dwell_Reject_REG		0x0a
#define MC32X0_DROP_Control_Register_REG	0x0b
#define MC32X0_SHAKE_Debounce_REG		0x0c
#define MC32X0_XOUT_EX_L_REG			0x0d
#define MC32X0_XOUT_EX_H_REG			0x0e
#define MC32X0_YOUT_EX_L_REG			0x0f
#define MC32X0_YOUT_EX_H_REG			0x10
#define MC32X0_ZOUT_EX_L_REG			0x11
#define MC32X0_ZOUT_EX_H_REG			0x12
#define MC32X0_CHIP_ID_REG			0x18
#define MC32X0_RANGE_Control_REG		0x20
#define MC32X0_SHAKE_Threshold_REG		0x2B
#define MC32X0_UD_Z_TH_REG			0x2C
#define MC32X0_UD_X_TH_REG			0x2D
#define MC32X0_RL_Z_TH_REG			0x2E
#define MC32X0_RL_Y_TH_REG			0x2F
#define MC32X0_FB_Z_TH_REG			0x30
#define MC32X0_DROP_Threshold_REG		0x31
#define MC32X0_TAP_Threshold_REG		0x32
#define MC32X0_PCODE_REG			0x3B

/* bits define */
#define MC32X0_G_RANGE_BIT		2
#define MC32X0_G_RANGE_2G		(0x0 << MC32X0_G_RANGE_BIT)
#define MC32X0_G_RANGE_4G		(0x1 << MC32X0_G_RANGE_BIT)
#define MC32X0_G_RANGE_8G_10BITS	(0x2 << MC32X0_G_RANGE_BIT)
#define MC32X0_G_RANGE_8G_14BITS	(0x3 << MC32X0_G_RANGE_BIT)

#define MC32X0_PWRCTRL_BIT		0
#define MC32X0_PWRCTRL_MSK		0x3
#define MC32X0_PWRCTRL_OFF		(0x3 << MC32X0_PWRCTRL_BIT)
#define MC32X0_PWRCTRL_ON		(0x1 << MC32X0_PWRCTRL_BIT)

enum mc32x0_pwr_ctrl {
	MC32X0_WAKE = 0,
	MC32X0_STANDBY,
};
/* chip id */
#define MC3250_CHIP_ID	0x01
static char mc32x0_chipid_table[] = {
	MC3250_CHIP_ID,
};
/* MC3210/20 define this */

//#define MCUBE_2G_10BIT_TAP
//#define MCUBE_2G_10BIT
//#define MCUBE_8G_14BIT_TAP
#define MCUBE_8G_14BIT  0x10

//#define DOT_CALI

#define MC32X0_HIGH_END	0x01
#define MC32X0_LOW_END 0x02
/* MC3210/20 define this */

#define MCUBE_1_5G_8BIT 0x20
//#define MCUBE_1_5G_8BIT_TAP
//#define MCUBE_1_5G_6BIT
#define MC32X0_MODE_DEF 		0x03

#define mc32x0_I2C_NAME			"mc32x0"
#define A10ASENSOR_DEV_COUNT	        1
#define A10ASENSOR_DURATION_DEFAULT	20

#define MAX_RETRY	20
#define INPUT_FUZZ	0
#define INPUT_FLAT	0

#define AUTO_CALIBRATION	0

static unsigned char  McubeID=0;

#ifdef DOT_CALI
#define CALIB_PATH		"/data/data/com.mcube.acc/files/mcube-calib.txt"
#define DATA_PATH		"/sdcard/mcube-register-map.txt"
#endif


static int log_debug = 1, rawdata_debug = 0, rptdata_debug = 0;
module_param_named(rawdata,  rawdata_debug, bool, S_IRUGO | S_IWUSR);
module_param_named(rptdata,  rptdata_debug, bool, S_IRUGO | S_IWUSR);
module_param_named(logdebug,     log_debug, bool, S_IRUGO | S_IWUSR);

#define rawdata_debug(x...) \
	do { \
		if (rawdata_debug) { \
			printk("%s -->\t",__func__); \
			printk(x); \
		} \
	}while(0)

#define rptdata_debug(x...) \
	do { \
		if (rptdata_debug) { \
			printk("%s -->\t",__func__); \
			printk(x); \
		} \
	}while(0)

#define dprintk(x...) \
	do { \
		if (log_debug) { \
			printk("%s -->\t",__func__); \
			printk(x); \
		} \
	}while(0)

#define assert(expr) \
	if(!(expr)) { \
		dprintk( "Assertion failed! %s,%d,%s,%s\n",__FILE__,__LINE__,__func__,#expr); \
	}

/* mc32x0 status */
struct mc32x0_status {
	uint8_t mode;
	uint8_t nor_odr;
};

static struct mc32x0_status mc32x0_status = {
	.mode 		= 0,
	.nor_odr	= 0
};

/* output data rate table */
static struct linux_sensor_t hardware_data = {
	"mc32x0 3-axis Accelerometer",
	"MTK sensor",
	SENSOR_TYPE_ACCELEROMETER,0,256,(9.8f * 2)/512.0f , 1, { }
};

static struct workqueue_struct *mc32x0_wqueue;
static struct mc32x0_cali_data cali_data;

struct mc32x0_data {
	struct i2c_client *client;
	struct mutex lock;
	struct delayed_work work;
	struct device *device;
	struct input_dev *input_dev;
	int is_suspend;
	int use_count;
	atomic_t enabled;
	volatile uint32_t duration;
	int use_irq; 
	int irq;
	unsigned long irqflags;
	int gpio;
	unsigned int map[3];
	int inv[3];
	struct gsensor_platform_data *pdata;
};

struct mc32x0_data *mc32x0_data;

static int mc32x0_set_delay(int delay);


static int mc32x0_smbus_read_block(struct device *dev, unsigned char reg,
					int count, void *buf)
{
    int ret;
	struct i2c_client *client = to_i2c_client(dev);

	ret = i2c_smbus_read_i2c_block_data(client, reg, count, buf);
	if(ret < 0){
		dprintk("smbus read block fialed %d\n", ret);
	}
    return ret;
}

static int mc32x0_smbus_write(struct device *dev,
			       unsigned char reg, unsigned char val)
{
	struct i2c_client *client = to_i2c_client(dev);
	return i2c_smbus_write_byte_data(client, reg, val);
}

static int mc32x0_smbus_set_bits(struct device *dev,
			       unsigned char reg, unsigned char bits)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t reg_val = i2c_smbus_read_byte_data(client,reg);
	reg_val |= bits;
	return i2c_smbus_write_byte_data(client, reg, reg_val);
}

static int mc32x0_smbus_clear_bits(struct device *dev,
			       unsigned char reg, unsigned char bits)
{
	struct i2c_client *client = to_i2c_client(dev);
	uint8_t reg_val = i2c_smbus_read_byte_data(client,reg);
	reg_val &= (~bits);
	return i2c_smbus_write_byte_data(client, reg, reg_val);
}

static int mc32x0_dev_pwr_ctrl(struct mc32x0_data *mc32x0, enum mc32x0_pwr_ctrl mode)
{
	int ret;
	uint8_t reg_val = i2c_smbus_read_byte_data(mc32x0_data->client, MC32X0_Mode_Feature_REG);
	reg_val &= (~MC32X0_PWRCTRL_MSK);
	switch (mode) {
		case MC32X0_WAKE :
			reg_val |= MC32X0_PWRCTRL_ON;
			break;
		case MC32X0_STANDBY :
			reg_val |= MC32X0_PWRCTRL_OFF;
			break;
		default :
			break;
	}
	ret = mc32x0_smbus_write(&mc32x0_data->client->dev, MC32X0_Mode_Feature_REG, reg_val);
	return ret;
}

static int mc32x0_enable(struct mc32x0_data *mc32x0)
{
	int ret;
	if(!atomic_cmpxchg(&mc32x0_data->enabled, 0, 1)) {
		ret = mc32x0_dev_pwr_ctrl(mc32x0, MC32X0_WAKE);
		if(ret < 0) {
			dprintk("ENABLE ERROR\n");
			atomic_set(&mc32x0_data->enabled, 0);
			return ret;
		}

		queue_delayed_work(mc32x0_wqueue, &mc32x0_data->work, 0);
	}
	return 0;
}

#if 0
static int mc32x0_calibrate(struct mc32x0_data *mc32x0)
{
#define SAMPLE_TIMES	100
	uint8_t tmp_data[6];
	short hw_d[3] = {0};
	int ret = 0, i = 0, sum_x = 0, sum_y = 0, sum_z = 0;

	disable_irq_nosync(mc32x0_data->client->irq);
	for (i = 0; i < SAMPLE_TIMES; i ++) {
		ret = mc32x0_smbus_read_block(&mc32x0_data->client->dev,
				DATAX0, DATAZ1 - DATAX0 + 1, tmp_data);
		if (ret < 0) {
			dprintk("i2c block read failed\n");
			return -1;
		}

		hw_d[0] = ((tmp_data[1] << 8) & 0xff00) | tmp_data[0];
		hw_d[1] = ((tmp_data[3] << 8) & 0xff00) | tmp_data[2];
		hw_d[2] = ((tmp_data[5] << 8) & 0xff00) | tmp_data[4];

		sum_x += hw_d[0];
		sum_y += hw_d[1];
		sum_z += hw_d[2];
	}

	sum_x /= SAMPLE_TIMES;
	sum_y /= SAMPLE_TIMES;
	sum_z /= SAMPLE_TIMES;

	cali_data.x_offset = 0 - sum_x;
	cali_data.y_offset = 0 - sum_y;
	if (sum_z < 0) {
		cali_data.z_offset = (-3568 - sum_z);
	} else {
		cali_data.z_offset = 3568 - sum_z;
	}

	printk("%s--> ret : x_offset = %d\t y_offset = %d\t, z_offset = %d\n",__func__,cali_data.x_offset, cali_data.y_offset, cali_data.z_offset);
	enable_irq(mc32x0_data->client->irq);

	return 0;
}
#endif
static int mc32x0_disable(struct mc32x0_data *mc32x0)
{
	int ret;
	if(atomic_cmpxchg(&mc32x0_data->enabled, 1, 0)) {
		disable_irq_nosync(mc32x0_data->client->irq);
		flush_workqueue(mc32x0_wqueue);
		cancel_delayed_work_sync(&mc32x0_data->work);
		ret = mc32x0_dev_pwr_ctrl(mc32x0, MC32X0_STANDBY);

		if(ret < 0) {
			dprintk("DISABLE ERROR\n");
			atomic_set(&mc32x0_data->enabled, 1);
			return ret;
		}
	}
	return 0;
}

static int mc32x0_print_regs(struct i2c_client *client)
{
	return 0;
}

static int mc32x0_chip_init(struct i2c_client *client)
{
	uint8_t data = 0;

	data = i2c_smbus_read_byte_data(client, MC32X0_PCODE_REG);

	switch (data) {
		case 0x19 :
		case 0x29 :
			McubeID = 0x22;
			break;
		case 0x88 : 
		case 0xA8 : 
		case 0x90 : 
			McubeID = 0x11;
			break;
		default :
			McubeID = 0;
	}

	data = MC32X0_MODE_DEF;
	i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG,data);
	data = 0x00;
	i2c_smbus_write_byte_data(client, MC32X0_Tap_Detection_Enable_REG,data);
	switch (mc32x0_data->pdata->g_range) {
		case MC32X0_2G :
			data = MC32X0_G_RANGE_2G;
			break;
		case MC32X0_4G :
			data = MC32X0_G_RANGE_4G;
			break;
		case MC32X0_8G_10BITS :
			data = MC32X0_G_RANGE_8G_10BITS;
			break;
		case MC32X0_8G_14BITS :
			data = MC32X0_G_RANGE_8G_14BITS;
			break;
		default :
			data = MC32X0_G_RANGE_2G;
	}
	data |= 0x83;
	i2c_smbus_write_byte_data(client, MC32X0_RANGE_Control_REG,data);
#if defined(CONFIG_MC32X0_MODE_INT)
	data = 0xFF;
	i2c_smbus_write_byte_data(client, MC32X0_Interrupt_Enable_REG,data);	
#endif

	if (McubeID & MCUBE_8G_14BIT) {
#ifdef DOT_CALI
		gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 1024;
#endif
	} else if (McubeID & MCUBE_1_5G_8BIT) {		
#ifdef DOT_CALI
		gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 86;
#endif
	}

#if 0
	data = 0x41;
	i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG,data);	
#endif
	return 0;
}

static int mc32x0_read_data(short *x, short *y, short *z)
{
	int ret = 0;
	uint8_t buf[6] = { 0 };
	int16_t hw_d[3] = { 0 };

	if (McubeID & MC32X0_HIGH_END) {
		ret = i2c_smbus_read_i2c_block_data(mc32x0_data->client , MC32X0_XOUT_EX_L_REG , 6 , buf);
		if (ret < 0) {
			dprintk("ERROR : %s --> %d\n",__func__,__LINE__);
		}
		hw_d[0] = (int16_t)((buf[1] << 8) | (buf[0]));
		hw_d[1] = (int16_t)((buf[3] << 8) | (buf[2]));
		hw_d[2] = (int16_t)((buf[5] << 8) | (buf[4]));
	} else if (McubeID & MC32X0_LOW_END) {
		ret = i2c_smbus_read_i2c_block_data(mc32x0_data->client , MC32X0_XOUT_REG , 3 , buf);
		if (ret < 0) {
			dprintk("ERROR : %s --> %d\n",__func__,__LINE__);
		}
		hw_d[0] = (int16_t)buf[0];
		hw_d[1] = (int16_t)buf[1];
		hw_d[2] = (int16_t)buf[2];
	}

	rawdata_debug("raw data:  x:%05d  y:%05d  z:%05d\n",hw_d[0], hw_d[1], hw_d[2]);

	*x = ((mc32x0_data->pdata->negate_x) ? (-hw_d[mc32x0_data->pdata->axis_map_x])
			: (hw_d[mc32x0_data->pdata->axis_map_x]));

	*y = ((mc32x0_data->pdata->negate_y) ? (-hw_d[mc32x0_data->pdata->axis_map_y])
			: (hw_d[mc32x0_data->pdata->axis_map_y]));

	*z = ((mc32x0_data->pdata->negate_z) ? (-hw_d[mc32x0_data->pdata->axis_map_z])
			: (hw_d[mc32x0_data->pdata->axis_map_z]));



	return 0;
}

#ifdef CONFIG_SENSORS_ORI
extern void orientation_report_values(int x,int y,int z);
#endif
static void report_abs(void)
{
	short x,y,z;

	if(mc32x0_read_data(&x, &y, &z) != 0)
		return;

	sensor_swap_pr((u16*)(&x),(u16*)(&y));
	rptdata_debug("report data: x:%05d  y:%05d  z:%05d\n", x, y, z);

	input_report_abs(mc32x0_data->input_dev, ABS_X, x);
	input_report_abs(mc32x0_data->input_dev, ABS_Y, y);
	input_report_abs(mc32x0_data->input_dev, ABS_Z, z);
	//dprintk("x:%d \t y:%d \t z:%d \t\n",x,y,z);
	input_sync(mc32x0_data->input_dev);

#ifdef CONFIG_SENSORS_ORI
	if(mc32x0_data->pdata->ori_pr_swap == 1){
		sensor_swap_pr((u16*)(&x),(u16*)(&y));
	}
	x = (mc32x0_data->pdata->ori_roll_negate) ? (-x)
			: x;
	y = (mc32x0_data->pdata->ori_pith_negate) ? (-y)
			: y;
	orientation_report_values(x,y,z);
#endif
}

static int mc32x0_set_delay(int delay)
{
	/* this function not run for mc32x0 */
	return 0;
}

static int mc32x0_input_open(struct input_dev *input)
{
	return mc32x0_enable(mc32x0_data);
}

static int mc32x0_input_close(struct input_dev *input)
{
	return mc32x0_disable(mc32x0_data);
}

static int mc32x0_misc_open(struct inode *inode, struct file *file)
{
	int ret;
	ret = nonseekable_open(inode, file);
	if (ret < 0)
		return ret;
	return 0;
}

long mc32x0_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int interval;

	switch (cmd) {
	case SENSOR_IOCTL_GET_DELAY:
		interval = mc32x0_data->pdata->poll_interval;
		dprintk("get delay = %d\n", interval);
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EFAULT;
		break;

	case SENSOR_IOCTL_SET_DELAY:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;

		dprintk("set delay = %d ms\n", interval);
		if (interval < mc32x0_data->pdata->min_interval)
			interval = mc32x0_data->pdata->min_interval;
		else if (interval > mc32x0_data->pdata->max_interval)
			interval = mc32x0_data->pdata->max_interval;

		mc32x0_data->pdata->poll_interval = interval;
		mc32x0_set_delay(mc32x0_data->pdata->poll_interval);
		break;

	case SENSOR_IOCTL_SET_ACTIVE:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval > 1)
			return -EINVAL;
		dprintk("set active = %s\n",interval ? "TRUE" : "FALSE");
		if (interval){
			mc32x0_enable(mc32x0_data);
		}else
			mc32x0_disable(mc32x0_data);
		break;

#if 0
	case SENSOR_IOCTL_CALIBRATE:
		printk("%s-->%d\n",__func__,__LINE__);
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval > 1)
			return -EINVAL;
		if (interval){
			printk("%s-->%d\n",__func__,__LINE__);
			if (mc32x0_calibrate(mc32x0) < 0)
				return -1;
		}
		break;
#endif
	case SENSOR_IOCTL_GET_ACTIVE:
		interval = atomic_read(&mc32x0_data->enabled);
		dprintk("%s --> get active = %s\n",__func__, interval ? "TRUE" : "FALSE");
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EINVAL;
		break;

	case SENSOR_IOCTL_GET_DATA:
		if (copy_to_user(argp, &hardware_data, sizeof(hardware_data)))
			return -EINVAL;
		break;

	case SENSOR_IOCTL_GET_DATA_MAXRANGE:
		if (copy_to_user(argp, &mc32x0_data->pdata->g_range, sizeof(mc32x0_data->pdata->g_range)))
			return -EFAULT;
		break;

	case SENSOR_IOCTL_WAKE:
		input_event(mc32x0_data->input_dev, EV_SYN, SYN_CONFIG, 0);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void mc32x0_work(struct work_struct *work)
{
	uint16_t delay_ms = 0;
	if (mc32x0_data->pdata->poll_interval < 1) {
		delay_ms = 1;
	} else {
		delay_ms = mc32x0_data->pdata->poll_interval;
	}

	report_abs();
#if defined(CONFIG_MC32X0_MODE_INT)
	msleep(delay_ms);
	enable_irq(mc32x0_data->client->irq);
#else
	queue_delayed_work(mc32x0_wqueue, &mc32x0_data->work, msecs_to_jiffies(delay_ms));
#endif
}

#if defined(CONFIG_MC32X0_MODE_INT)
static irqreturn_t mc32x0_interrupt(int irq, void *dev_id)
{
	struct mc32x0_data *mc32x0 = i2c_get_clientdata(mc32x0_data->client);
	disable_irq_nosync(mc32x0_data->client->irq);

	if(mc32x0_data->is_suspend == 1 || atomic_read(&mc32x0_data->enabled) == 0)
		return IRQ_HANDLED;

	if (!delayed_work_pending(&mc32x0_data->work)) {
		queue_delayed_work(mc32x0_wqueue, &mc32x0_data->work, 0);
	}
	return IRQ_HANDLED;
}
#endif

static const struct file_operations mc32x0_misc_fops = {
	.owner = THIS_MODULE,
	.open = mc32x0_misc_open,
	.unlocked_ioctl = mc32x0_misc_ioctl,
};

static struct miscdevice mc32x0_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name =  "g_sensor",
	.fops = &mc32x0_misc_fops,
};

static int __devinit mc32x0_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	uint8_t buf = 0;
	int i = 0, ret = -1;

	if(!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA)){
		dev_err(&client->dev, "client not i2c capable\n");
		ret = -ENODEV;
	       	goto exit;
	}

	if (!client->dev.platform_data) {
		dev_err(&client->dev, "client has not platform data!\n");
		ret = -EINVAL;
		goto exit;
	}

	buf = i2c_smbus_read_byte_data(client,MC32X0_CHIP_ID_REG);
	for (i = 0; i < ARRAY_SIZE(mc32x0_chipid_table); i++) {
		if (buf == mc32x0_chipid_table[i])
			break;
	}

	if (i < ARRAY_SIZE(mc32x0_chipid_table)) {
		printk("detected mc32x0 chip! id = 0x%02x\n",buf);
	} else {
		printk("can not detected mc32x0 chip! id = 0x%02x\n",buf);
		ret = -ENODEV;
		goto exit;
	}

	mc32x0_data = kzalloc(sizeof(*mc32x0_data), GFP_KERNEL);
	if (NULL == mc32x0_data) {
		dev_err(&client->dev,"failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto exit;
	}

	mutex_init(&mc32x0_data->lock);
	mutex_lock(&mc32x0_data->lock);

	mc32x0_data->client = client;

	mc32x0_data->pdata = kmalloc(sizeof(*mc32x0_data->pdata), GFP_KERNEL);
	if(NULL == mc32x0_data->pdata)
		goto exit_kfree1;

	memcpy(mc32x0_data->pdata, client->dev.platform_data, sizeof(*mc32x0_data->pdata));
	//memcpy(&cali_data, &mc32x0_data->pdata->cali_data, sizeof(cali_data));

	i2c_set_clientdata(client,mc32x0_data);

	mc32x0_chip_init(client);

	INIT_DELAYED_WORK(&mc32x0_data->work, mc32x0_work);

	mc32x0_wqueue = create_singlethread_workqueue("mc32x0");
	if(!mc32x0_wqueue){
		ret = -ESRCH;
		goto exit_kfree2;
	}

#if defined(CONFIG_MC32X0_MODE_INT)
	if (gpio_request_one(mc32x0_data->pdata->gpio_int,
				GPIOF_DIR_IN, "gsensor mc32x0 irq")) {
		dev_err(&client->dev, "no irq pin available\n");
		mma->pdata->gpio_int = -EBUSY;
	} else {
		client->irq = gpio_to_irq(mc32x0_data->pdata->gpio_int);
		ret = request_irq(client->irq, mc32x0_interrupt, 
				IRQF_DISABLED | IRQF_TRIGGER_LOW,
				"mc32x0", mc32x0_data);
		if (ret < 0) {
			printk(" request irq is error\n");
			dev_err(&client->dev, "%s rquest irq failed\n", __func__);
			goto exit_kfree2;
		}

		disable_irq_nosync(client->irq);
	}
#endif

	/*input poll device register */
	mc32x0_data->input_dev = input_allocate_device();
	if (!mc32x0_data->input_dev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		printk("alloc poll device failed!\n");
		ret = -ENOMEM;
		goto exit_free_irq;
	}

	input_set_drvdata(mc32x0_data->input_dev,mc32x0_data);

	set_bit(EV_ABS, mc32x0_data->input_dev->evbit);
	input_set_abs_params(mc32x0_data->input_dev, ABS_X, -8192, 8192, 0, 0);
	input_set_abs_params(mc32x0_data->input_dev, ABS_Y, -8192, 8192, 0, 0);
	input_set_abs_params(mc32x0_data->input_dev, ABS_Z, -8192, 8192, 0, 0);

	mc32x0_data->input_dev->name = "g_sensor";
	ret = input_register_device(mc32x0_data->input_dev);
	if (ret) {
		dev_err(&client->dev, "register poll device failed!\n");
		dprintk("register poll device failed!\n");
		goto exit_input_free_dev;
	}

	ret = misc_register(&mc32x0_misc_device);
	if (ret) {
		dev_err(&client->dev, "register misc device failed!\n");
		goto exit_input_unregister_dev;
	}

	atomic_set(&mc32x0_data->enabled,0);
	mc32x0_data->is_suspend = 0;


	mutex_unlock(&mc32x0_data->lock);

	return 0;

exit_input_unregister_dev:
	input_unregister_device(mc32x0_data->input_dev);
exit_input_free_dev:
	input_free_device(mc32x0_data->input_dev);

exit_free_irq:
	if (mc32x0_data->pdata->gpio_int != -EBUSY)
		free_irq(client->irq, mc32x0_data);
exit_kfree2:
	kfree(mc32x0_data->pdata);

exit_kfree1:
	mutex_unlock(&mc32x0_data->lock);
	kfree(mc32x0_data);
exit:
	return ret;
}

static int __devexit mc32x0_remove(struct i2c_client *client)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG, MC32X0_STANDBY);
	assert(ret==0);

	input_unregister_device(mc32x0_data->input_dev);
	misc_deregister(&mc32x0_misc_device);

	return ret;
}

static const struct i2c_device_id mc32x0_id[] = {
	{ MC32X0_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mc32x0_id);

static struct i2c_driver mc32x0_driver = {
	.driver = {
		.name	= MC32X0_NAME,
		.owner	= THIS_MODULE,
	},

	.probe	= mc32x0_probe,
	.remove	= __devexit_p(mc32x0_remove),
	.id_table = mc32x0_id,
};

static int __init mc32x0_init(void)
{
	int ret;

	printk("add mc32x0 i2c driver\n");
	ret = i2c_add_driver(&mc32x0_driver);
	if (ret < 0) {
		printk("add mc32x0 i2c driver failed\n");
		return -ENODEV;
	}

	return ret;
}

static void __exit mc32x0_exit(void)
{
	printk("remove mc32x0 i2c driver.\n");
	i2c_del_driver(&mc32x0_driver);
}

module_init(mc32x0_init);
module_exit(mc32x0_exit);

MODULE_AUTHOR("Aaron Wang <hfwang@ingenic.cn>");
MODULE_DESCRIPTION("mc32x0 3-Axis G-Sensor driver");
MODULE_LICENSE("GPL");
