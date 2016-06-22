/* drivers/misc/bma250e.c
 *
 * Copyright (c) 2015.01  Ingenic Semiconductor Co., Ltd.
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

#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
//#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/clk.h>
#include <linux/syscalls.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <asm/div64.h>
#include <linux/kfifo.h>
#include <linux/linux_sensors.h>
#include <linux/i2c/bma250e.h>

#define dbg(format, arg...) printk(KERN_INFO "[%s : %d ]" format , __func__, __LINE__, ## arg)
#define DBG_BMA250E 1

#define DATA_BUF_SIZE	(1 * 1024 * 1024)
#define BMA250e_GET_DATA(data) (1 << 30) | 		\
	((data).single.accel_x << 20) | 		\
	((data).single.accel_y << 10) | 		\
	 (data).single.accel_z

#define ADD_TIMESTAMP_64BITS(buff_32, time_64) do {   \
	buff_32[0] = (u32)(time_64 >> 32) | (2 << 30);  \
	buff_32[1] = (u32)(time_64);  } while(0)

typedef union bma250_accel_data {

	/** raw register data */
	uint32_t d32;

	/** register bits */
	struct {
		unsigned accel_z:10;
		unsigned accel_y:10;
		unsigned accel_x:10;
		unsigned flag:2;
	} single;
} bma250_accel_data_t;

typedef struct accel_info_pt {
	int threshold;
	int frequency;
	int ranges;
} accel_info_ptr_t;

struct bma250e_dev {
	bma250_accel_data_t accel_data;
	struct device *dev;
	struct i2c_client *client;
	struct miscdevice mdev;
	struct completion completion;
	struct regulator *power;
	struct task_struct *kthread;
	struct kfifo bma250e_kfifo;

	u32 updata_time;
	u32 bma250e_data;
	u64 time_ms;
	int irq;
	int gpio;
	int wakeup;
	int suspend;
};

static u64 get_current_time(void)
{
	u64 current_msec;
#if 0
	struct timespec time;
	time = current_kernel_time();
	current_msec = (long long)time.tv_sec * 1000 + time.tv_nsec / 1000000;
    dbg("gettimeofday----> %ld  %ld  %lld\n", time.tv_sec, time.tv_nsec, current_msec);
#else
	struct timeval time;
	do_gettimeofday(&time);
	current_msec = (long long)time.tv_sec * 1000 + time.tv_usec / 1000;
	/*
	u64 test_time = (u64)time.tv_sec * 1000000 + time.tv_usec;
	do_div(test_time, 1000);
	dbg("test_time = %lld\n", test_time);
	*/
#endif
	return current_msec;
}

static int bma250e_kfifo_init(struct bma250e_dev *bma250e)
{
	int ret = 0;
	if((ret = kfifo_alloc(&bma250e->bma250e_kfifo, DATA_BUF_SIZE, GFP_KERNEL)) != 0){
		printk("----kfifo_alloc error!----\n");
	}
	return ret;
}

static int bma250e_open(struct inode *inode, struct file *filep)
{
	int ret = 0;
	struct miscdevice *dev = filep->private_data;
	struct bma250e_dev *bma250e = container_of(dev, struct bma250e_dev, mdev);

	wake_up_process(bma250e->kthread);
	bma250e->time_ms = get_current_time();
#ifdef DBG_BMA250E
	dbg("bma250e_open is ok!\n");
#endif
	return ret;
}

static int bma250e_release(struct inode *inode, struct file *filep)
{
	int ret = 0;
#ifdef DBG_BMA250E
	dbg("bma250e_release is ok!\n");
#endif
	return ret;
}

static int bma250e_init_set(struct i2c_client *client)
{
	int ret = 0;
	//bma250e_reset
	ret = i2c_smbus_write_byte_data(client, BMA250_RESET_REG, BMA250_RESET);
	if(ret)  goto err_i2c_write;
	msleep(200);
	//bma250e set register
	ret = i2c_smbus_write_byte_data(client, BMA250_BW_SEL_REG, BMA250_BW_7_81HZ);
	if(ret)  goto err_i2c_write;
	//bma250e set range register
	ret = i2c_smbus_write_byte_data(client, BMA250_RANGE_REG, BMA250_RANGE_2G);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_INT_SRC, BMA250_INT_FILTER);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_ACCD_HBW, BMA250_SHADOW_DIS);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_SLOPE_THR, BMA250_SLOPE_TH_4);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_SLOPE_DUR, BMA250_SLOPE_DUR_VALUE);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_INT_ENABLE1_REG, BMA250_INT_SLOPE_XYZ);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_INT_PIN1_REG, BMA250_INT_PIN1_SLOPE);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_INT_CTRL_REG, BMA250_INT_RESET);
	if(ret)  goto err_i2c_write;
	ret = i2c_smbus_write_byte_data(client, BMA250_INT_OUT_CTRL, BMA250_INT1_LVL);
	if(ret)  goto err_i2c_write;

	return ret;
err_i2c_write:
	return ret;
}

static int bma250e_get_data(struct bma250e_dev *bma250e, u32 *data)
{
	int ret = 0;
	u8 rx_buf[6];
	bma250_accel_data_t bma250e_data;
	bma250e_data.d32 = 0;
	bma250e_data.single.flag = 1;

	if((ret = i2c_smbus_read_i2c_block_data(bma250e->client, BMA250_X_AXIS_LSB_REG, 6, rx_buf)) == 0) {
		printk("i2c_smbus_read_i2c_block_data  error \n");
	}
	bma250e_data.single.accel_x = ((rx_buf[1] << 8) | (rx_buf[0])) >> 6;
	bma250e_data.single.accel_y = ((rx_buf[3] << 8) | (rx_buf[2])) >> 6;
	bma250e_data.single.accel_z = ((rx_buf[5] << 8) | (rx_buf[4])) >> 6;
	*data = BMA250e_GET_DATA(bma250e_data);
	/*dbg("x = %d\t y = %d\t z = %d\t\n", bma250e_data.single.accel_x,*/
			/*bma250e_data.single.accel_y, bma250e_data.single.accel_z);*/

	return ret;
}

static ssize_t bma250e_read(struct file *filep, char __user *buf, size_t size, loff_t *off)
{
	ssize_t out_byte_len = 0;
	unsigned int  kfifo_len;
	u32 data_buff[1024];
	struct miscdevice *dev = filep->private_data;
	struct bma250e_dev *bma250e = container_of(dev, struct bma250e_dev, mdev);

	kfifo_len = kfifo_len(&bma250e->bma250e_kfifo);
	memset(data_buff, 0, sizeof(data_buff));
	if (kfifo_len > 0){
#if 0
		/*dbg("kfifo_len = %d\n", kfifo_len);*/
		if(kfifo_len > 1016) {
			out_byte_len = kfifo_out_peek(&bma250e->bma250e_kfifo, data_buff, 8);
			if(copy_to_user(buf, (char *)data_buff, out_byte_len)){  // long long -----> 8 char
				printk("copy_from_user failed\n");
				return -EFAULT;
			}

		}
#else
//		dbg("out_byte_len = %d\n", out_byte_len);
		out_byte_len = kfifo_out(&bma250e->bma250e_kfifo, data_buff, kfifo_len);
		if(copy_to_user(buf, (char *)data_buff, out_byte_len)){  // long long -----> 8 char
			printk("copy_from_user failed\n");
			return -EFAULT;
		}

#endif
	}

#if 0
	/* test i2c read data from register of bma250e */
	if((ret = i2c_smbus_read_i2c_block_data(bma250e->client, BMA250_X_AXIS_LSB_REG, 6, rx_buf)) == 0) {
		dbg("bma250e_read error  %d\n", __LINE__);
	}

	/* read tempeture */
	if((ret = i2c_smbus_read_i2c_block_data(bma250e->client, BMA250_ACCD_TEMP, 1, rx_buf+6)) == 0) {
		dbg("bma250e_read error  %d\n", __LINE__);
	}

	copy_to_user(buf, rx_buf, 7);
#endif

	return out_byte_len;
}

static long bma250e_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct miscdevice *dev = filep->private_data;
	struct bma250e_dev *bma250e = container_of(dev, struct bma250e_dev, mdev);
	accel_info_ptr_t accel_info;

	if(copy_from_user(&accel_info, (accel_info_ptr_t *)arg, sizeof(accel_info_ptr_t))) {
		printk("copy_from_user failed\n");
		return -EFAULT;
	}

#ifdef DBG_BMA250E
	dbg("frequency = %d, ranges = %d, threshold = %d\n", accel_info.frequency,
			accel_info.ranges, accel_info.threshold);
#endif
	switch(cmd){
	case SENSOR_IOCTL_SET_THRESHOLD:

		ret = i2c_smbus_write_byte_data(bma250e->client,
				BMA250_SLOPE_THR, accel_info.threshold);
		if(ret)
			goto config_exit;
		break;
	case SENSOR_IOCTL_SET_FREQUENCY:
		/*set frequency for bma250e */
		if (accel_info.frequency > 0 && accel_info.frequency <= 8) {
			accel_info.frequency = BMA250_BW_7_81HZ;
			/*bma250e->updata_time = 64;*/
		} else if (accel_info.frequency <= 16) {
			accel_info.frequency = BMA250_BW_15_63HZ;
			/*bma250e->updata_time = 32;*/
		} else if (accel_info.frequency <= 32) {
			accel_info.frequency = BMA250_BW_31_25HZ;
			/*bma250e->updata_time = 16;*/
		} else if (accel_info.frequency <= 63) {
			accel_info.frequency = BMA250_BW_62_50HZ;
			/*bma250e->updata_time = 8;*/
		} else if (accel_info.frequency <= 125) {
			accel_info.frequency = BMA250_BW_125HZ;
			/*bma250e->updata_time = 4;*/
		} else if (accel_info.frequency <= 250) {
			accel_info.frequency = BMA250_BW_250HZ;
			/*bma250e->updata_time = 2;*/
		} else if (accel_info.frequency <= 500) {
			accel_info.frequency = BMA250_BW_500HZ;
			/*bma250e->updata_time = 1;*/
		} else if (accel_info.frequency > 500) {
			accel_info.frequency = BMA250_BW_500HZ;
			/*bma250e->updata_time = 0.5;*/
		} else {
			printk("FREQUENCY cannot Less than zero");
			return -1;
		}

		/*bma250e->updata_time = pow(2, accel_info.frequency);*/
		bma250e->updata_time = 0x40 >> (accel_info.frequency & (~0x8));

		ret = i2c_smbus_write_byte_data(bma250e->client,
				BMA250_BW_SEL_REG, accel_info.frequency);
		if(ret)
			goto config_exit;
		break;
	case SENSOR_IOCTL_SET_RANGES:
		/*set ranges for bma250e */
		switch (accel_info.ranges) {
		case 2:
			accel_info.ranges = BMA250_RANGE_2G;
			break;
		case 4:
			accel_info.ranges = BMA250_RANGE_4G;
			break;
		case 8:
			accel_info.ranges = BMA250_RANGE_8G;
			break;
		case 16:
			accel_info.ranges = BMA250_RANGE_16G;
			break;
		default:
			accel_info.ranges = BMA250_RANGE_2G;
		}

		ret = i2c_smbus_write_byte_data(bma250e->client,
				BMA250_RANGE_REG, accel_info.ranges);

		if(ret)
			goto config_exit;
		break;
/*
	case SENSOR_IOCTL_GET_TEMPUTER:

		break;
*/
	default:
		break;
	}

	return 0;

	config_exit:
	return ret;
}

static irqreturn_t bma250e_interrupt(int irq, void *dev)
{
	struct bma250e_dev *bma250e = dev;

	complete(&bma250e->completion);

	return IRQ_HANDLED;
}

static int bma250e_daemon(void *dev)
{
	int ret = 0;
	struct bma250e_dev *bma250e = dev;
	u64 time_base = 0,
		time_now = 0;
	u32 time_delay = bma250e->updata_time * 2;
	u32 time_tmp[2];

	if(bma250e->suspend) {
		bma250e->time_ms = get_current_time();
		bma250e->suspend = 0;
	}

	while(!kthread_should_stop()) {

		time_delay = bma250e->updata_time * 2;
		wait_for_completion_interruptible(&bma250e->completion);
		time_base = bma250e->time_ms;
		time_now = get_current_time();
		/* time in the time_delay(default--->64ms) */
		if(time_delay >= (time_now - time_base)){
#ifdef DBG_BMA250E
	//		dbg("in the time_delay\n");
#endif
			if((ret = bma250e_get_data(bma250e, &bma250e->bma250e_data)) == 0) {
				printk("[%s, %d]  bma250e_get_data error!\n", __func__, __LINE__);
				goto err;
			}
			ret = kfifo_in(&bma250e->bma250e_kfifo,(char *)&bma250e->bma250e_data, sizeof(u32));
			if(ret == 0) {
				printk("[%s, %d]  kfifo_in error!\n", __func__, __LINE__);
			//	goto err;
			}
			bma250e->time_ms = get_current_time();
		}else {
#ifdef DBG_BMA250E
	//		dbg("out of the time_delay\n");
#endif
		/*
			time_tmp[1] = (u32)time_now;
			time_tmp[0] = (u32)(time_now >> 32) | (2 << 30);      //low bit store high bit time_now
		*/
			/* add timestamp */
			ADD_TIMESTAMP_64BITS(time_tmp, time_now);
			ret = kfifo_in(&bma250e->bma250e_kfifo, (char *)time_tmp, sizeof(time_tmp));
			if(ret == 0) {
				printk("[%s, %d]  kfifo_in error!\n", __func__, __LINE__);
			//	goto err;
			}
			/* store x/y/z values */
			if((ret = bma250e_get_data(bma250e, &bma250e->bma250e_data)) == 0) {
				printk("[%s, %d]  bma250e_get_data error!\n", __func__, __LINE__);
				goto err;
			}
			ret = kfifo_in(&bma250e->bma250e_kfifo, (char *)&bma250e->bma250e_data, sizeof(u32));
			if(ret == 0) {
				printk("[%s, %d]  kfifo_in error!\n", __func__, __LINE__);
			//	goto err;
			}
			bma250e->time_ms = get_current_time();
		}
	}

	return 0;

	err:
		ret = -1;
	return ret;
}
static struct file_operations bma250e_fp = {
	.owner	= THIS_MODULE,
	.open 	= bma250e_open,
	.release= bma250e_release,
	.read   = bma250e_read,
	.unlocked_ioctl	= bma250e_ioctl,
};

static int bma250e_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	u8 rx_buf[2];

	struct bma250e_dev *bma250e;
	struct bma250_platform_data *pdata =
		(struct bma250_platform_data *)client->dev.platform_data;
#ifdef DBG_BMA250E
	dbg("bma250e_probe\n");
#endif
	bma250e = kzalloc(sizeof(struct bma250e_dev), GFP_KERNEL);
	if(!bma250e){
		printk("kzalloc bma250e memery error\n");
		return -ENOMEM;
	}

	bma250e->dev = &client->dev;
	bma250e->gpio = pdata->gpio;
	bma250e->wakeup = pdata->wakeup;
	if(gpio_request_one(bma250e->gpio, GPIOF_DIR_IN, "bma250e_irq")) {
		dev_err(bma250e->dev, "no irq pin available\n");
		ret = -EBUSY;
		goto err_gpio;
	}

	init_completion(&bma250e->completion);
	if((ret = bma250e_kfifo_init(bma250e)) != 0) {
		printk("bma250e_kfifo_init error!\n");
		goto err_gpio;
	}

	client->irq = gpio_to_irq(bma250e->gpio);
	ret = request_irq(client->irq, bma250e_interrupt,
					IRQF_TRIGGER_RISING | IRQF_DISABLED, BMA250_NAME,
					bma250e);
	if(ret < 0)	{
		dev_err(bma250e->dev, "%s: request_irq failed\n", __func__);
		goto err_request_gpio;
	}

	bma250e->irq = client->irq;
	bma250e->client = client;

	i2c_set_clientdata(client, bma250e);

	/*dbg("--------bma250e->kthread---------\n");*/
	bma250e->kthread = kthread_create(bma250e_daemon, bma250e, "bma250e_daemon");
	if (IS_ERR(bma250e->kthread)){
		ret = -1;
		goto err_kthread;
	}

	bma250e->power = regulator_get(bma250e->dev, "vcc_sensor1v8");
	if(!IS_ERR(bma250e->power)){
		regulator_enable(bma250e->power);
	}else {
		dev_warn(bma250e->dev, "get regulator failed\n");
		goto err_regulator;
	}

	if(!(ret = i2c_smbus_read_i2c_block_data(client, BMA250_CHIP_ID_REG, 2, rx_buf))){
		printk(KERN_INFO "bma250e register error\n");
		goto err_i2c;
	}
	printk("bma250e: detected chip id %x, rev %X\n", rx_buf[0], rx_buf[1]);

	if((ret = bma250e_init_set(client))) {
		printk("bma250e_init_set error\n");
		goto err_i2c;
	}

	bma250e->mdev.minor = MISC_DYNAMIC_MINOR;
	bma250e->mdev.name = BMA250_NAME;
	bma250e->mdev.fops = &bma250e_fp;
	if((ret = misc_register(&bma250e->mdev)) < 0){
		dev_err(bma250e->dev, "misc register failed\n");
		goto err_misc;
	}else {
		printk("misc register is ok!\n");
	}

	bma250e->updata_time = 64;

	return 0;

	err_misc:
	err_i2c:
		regulator_put(bma250e->power);
	err_regulator:
	err_kthread:
		i2c_set_clientdata(client, NULL);
		free_irq(bma250e->irq, bma250e);
	err_request_gpio:
		kfifo_free(&bma250e->bma250e_kfifo);
		gpio_free(bma250e->gpio);
	err_gpio:
		kfree(bma250e);

	return ret;
}

static int bma250e_remove(struct i2c_client *client)
{
	int ret = 0;
	struct bma250e_dev *bma250e = i2c_get_clientdata(client);
#ifdef DBG_BMA250E
	dbg("bma250e_remove\n");
#endif
	misc_deregister(&bma250e->mdev);
	regulator_put(bma250e->power);
	gpio_free(bma250e->gpio);
	i2c_set_clientdata(client, NULL);
	free_irq(bma250e->irq, bma250e);
	kfifo_free(&bma250e->bma250e_kfifo);
	kfree(bma250e);

	return ret;
}

static int bma250e_suspend(struct i2c_client *client, pm_message_t state)
{
	int ret = 0;
	struct bma250e_dev *bma250e = i2c_get_clientdata(client);

	if (bma250e->wakeup)
		enable_irq_wake(bma250e->irq);
#ifdef DBG_BMA250E
	dbg("bma250e_suspend\n");
#endif
	return ret;
}

static int bma250e_resume(struct i2c_client *client)
{
	int ret = 0;
	struct bma250e_dev *bma250e = i2c_get_clientdata(client);

	bma250e->suspend = 1;
#ifdef DBG_BMA250E
	dbg("bma250e_resume\n");
#endif
	return ret;
}

static const struct i2c_device_id bma250e_id[] = {
	{"bma250e-misc", 0},
	{},
};

static struct of_device_id bma250e_dt_match[] = {
	{.compatible = "bosh, bma250e"},
	{},
};

static struct i2c_driver bma250e_driver = {
	.driver = {
		.name = "bma250e-misc",
		.owner = THIS_MODULE,
		.of_match_table = bma250e_dt_match,
	},
	.probe = bma250e_probe,
	.remove = bma250e_remove,
	.suspend = bma250e_suspend,
	.resume = bma250e_resume,
	.id_table = bma250e_id,
};

static int __init bma250e_init(void)
{
#ifdef DBG_BMA250E
	dbg("bma250e_init\n");
#endif
	return i2c_add_driver(&bma250e_driver);
}

static void __exit bma250e_exit(void)
{
#ifdef DBG_BMA250E
	dbg("bma250e_exit\n");
#endif
	i2c_del_driver(&bma250e_driver);
}

module_init(bma250e_init);
module_exit(bma250e_exit);

MODULE_DESCRIPTION("bma250e misc driver");
MODULE_LICENSE("GPL");
