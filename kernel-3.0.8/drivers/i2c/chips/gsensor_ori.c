/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : orientation.c
* Authors            : MSH - Motion Mems BU - Application Team
*		     : Carmine Iascone (carmine.iascone@st.com)
*		     : Matteo Dameno (matteo.dameno@st.com)
* Version            : V 1.1.0
* Date               : 19/03/2010
* Description        : LSM303DLH 6D module sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
******************************************************************************

26_4_2010: orientation_device_power_off now calling CTRL_REG1 to set power off

******************************************************************************/

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/linux_sensors.h>

#define INPUT_FUZZ	1250
#define INPUT_FLAT	0

struct input_dev *input_dev;


static struct linux_sensor_t ori_hardware_data = {                                                                           
	                  "fake orientation base g-sensor",
	                   "fake jz sensor",
	       SENSOR_TYPE_ORIENTATION,1,64,1, 1, { } //modify version from 0 to 1 for cts
};

void orientation_report_values(int x,int y,int z)
{

	int pitch, roll;
	pitch = y;
	roll = -x;
	pitch *=90;
	roll *= 90;
		
	input_report_abs(input_dev, ABS_RX, 0);
	input_report_abs(input_dev, ABS_RY, pitch);
	input_report_abs(input_dev, ABS_RZ, roll);
	input_sync(input_dev);
}

long orientation_misc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case SENSOR_IOCTL_GET_DATA:
		if (copy_to_user(argp, &ori_hardware_data, sizeof(ori_hardware_data)))
			return -EINVAL;
		break;

	case SENSOR_IOCTL_WAKE:
		input_event(input_dev, EV_SYN,SYN_CONFIG, 0);
		break;
	case SENSOR_IOCTL_SET_DELAY:
	case SENSOR_IOCTL_GET_DELAY:
	case SENSOR_IOCTL_SET_ACTIVE:
	case SENSOR_IOCTL_GET_ACTIVE:
	case SENSOR_IOCTL_GET_DATA_MAXRANGE:
		return 0;
	default:
		return -EINVAL;
	}
	return 0;
}

static int orientation_misc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations orientation_misc_fops = {
	.owner = THIS_MODULE,
	.open = orientation_misc_open,
	.unlocked_ioctl = orientation_misc_ioctl,
};

static struct miscdevice orientation_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = ORIENTATION_SENSOR_NAME,
	.fops = &orientation_misc_fops,
};

static int __init orientation_init(void)
{

	int err;
	input_dev = input_allocate_device();

	if (!input_dev) {
		return -ENOMEM;
	}

	set_bit(EV_ABS, input_dev->evbit);
/*
	input_set_abs_params(input_dev, ABS_RX, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(input_dev, ABS_RY, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(input_dev, ABS_RZ, -8192, 8191, INPUT_FUZZ, INPUT_FLAT);

    */
	input_set_abs_params(input_dev, ABS_RX, -8192, 8191, 0, 0);
	input_set_abs_params(input_dev, ABS_RY, -8192, 8191, 0, 0);
	input_set_abs_params(input_dev, ABS_RZ, -8192, 8191, 0, 0);
	input_dev->name = ORIENTATION_SENSOR_NAME;

	err = input_register_device(input_dev); //??regist a input event device
	if (err) {
		printk(" oriention input register is error\n");
		return -ENODEV;
	}

	err = misc_register(&orientation_misc_device);  //?? register a charactor device 
	if (err < 0) {
		return -ENODEV;
	}
	return 0;
}

static void __exit orientation_exit(void)
{
	misc_deregister(&orientation_misc_device);
}

module_init(orientation_init);
module_exit(orientation_exit);

MODULE_DESCRIPTION("fake orientation driver based on mma8452");
MODULE_AUTHOR("bcjia@ingenic.cn");
MODULE_LICENSE("GPL");

