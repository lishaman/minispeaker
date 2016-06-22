/* linux/drivers/spi/spi_jz_test.c
 *
 * This driver can be used test JZ47xx SSI controller driver.
 * base-to: linux/drivers/spi/spi_jz47xx.c
 *
 * Copyright (c) 2010 Ingenic
 * Author: jwsun<jwsun@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>		/* for spi mode */
#include <linux/proc_fs.h>

#ifdef SSI_DEGUG
#define	pr_dbg(format,arg...)			\
	printk(format,## arg)
#else
#define	pr_dbg(format,arg...)
#endif

struct spi_jz_test {
	bool	full_duplex_enable;
	bool	write_enable;
	bool	read_enable;

	unsigned long buf_len;

	bool	fixed_buf_test;

	struct work_struct      read_work;
	struct work_struct	write_work;
	struct work_struct	full_duplex_work;
	struct spi_device *jz_spi;

	struct workqueue_struct *workqueue;
};

static struct spi_jz_test spi_jz_test;

/**************************************************************
 * test_spi_write_and_read - Test the spi module write function
 *			and read function the same time
 * @txbuf: write buf
 * @rxbuf: read buf
 * @len: the write or read buf length
 **************************************************************/
unsigned int spi_write_and_read(struct spi_device *spi, const void *txbuf,
		void *rxbuf, unsigned len)
{
	struct spi_transfer	t = {
			.tx_buf		= txbuf,
			.rx_buf		= rxbuf,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spi_sync(spi, &m);

}

#if 0
static void read_work(struct work_struct *work)
{
	struct spi_jz_test *spi_jz_test =
		container_of(work, struct spi_jz_test, read_work);

	if (spi_jz_test.jz_spi->bits_per_word == 0) {
		pr_err("Please input the correct bits_per_word\n");
		return ;
	}
}

static void write_work(struct work_struct *work)
{
	struct spi_jz_test *spi_jz_test =
		container_of(work, struct spi_jz_test, write_work);

	if (spi_jz_test.jz_spi->bits_per_word == 0) {
		pr_err("Please input the correct bits_per_word\n");
		return ;
	}
}
#endif

static void full_duplex_work(struct work_struct *work)
{
	unsigned char *tx_buf;
	unsigned char *rx_buf;
	unsigned long i, j;
	unsigned long time;
	unsigned long ret = 0;

	if (spi_jz_test.buf_len == 0 ||
			spi_jz_test.jz_spi->bits_per_word == 0) {
		pr_err("Please input the correct buffer_len or bits_per_word\n");
		return;
	}

	tx_buf = kzalloc(spi_jz_test.buf_len, GFP_KERNEL);
	if (tx_buf == NULL) {
		pr_err("No memory for tx_buf\n");
		return;
	}

	rx_buf = kzalloc(spi_jz_test.buf_len, GFP_KERNEL);
	if (rx_buf == NULL) {
		pr_err("No memory for tx_buf\n");
		return;
	}

	for(i = 0; i < spi_jz_test.buf_len; i++) {
		tx_buf[i] = i % 256;
	}

	if (spi_jz_test.fixed_buf_test)
		j = spi_jz_test.buf_len;
	else
		j= spi_jz_test.jz_spi->bits_per_word / 8;

	while (spi_jz_test.full_duplex_enable) {

		if (j > spi_jz_test.buf_len)
			j = spi_jz_test.jz_spi->bits_per_word / 8;

		time = 5;

		pr_info("Now test %ld\n", j);
		while (time--) {
			for(i = 0; i < j; i++) {
				rx_buf[i] = 0;
			}
			if (!spi_jz_test.full_duplex_enable) {
				return;
			}

			ret = spi_write_and_read(spi_jz_test.jz_spi,
					tx_buf, rx_buf, j);
			if (ret < 0) {
				msleep(2000);
				break;
			}

			for(i = 0; i < j; i++) {
				if (tx_buf[i] != rx_buf[i]) {
					pr_err("The transfer is error\n");
					pr_err("tx_buf[%ld]=%d  rx_buf[%ld]=%d ",
							i, tx_buf[i], i, rx_buf[i]);
					msleep(2000);
					break;
				}
			}

		}

		if (!spi_jz_test.fixed_buf_test)
			j += spi_jz_test.jz_spi->bits_per_word / 8;
	}

}

static ssize_t get_frame(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret;
	
	ret = sprintf(buf, "%d\n", spi_jz_test.jz_spi->bits_per_word);

	return ret;
}

static ssize_t get_buf(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret;
	
	ret = sprintf(buf, "%ld\n", spi_jz_test.buf_len);

	return ret;
}

static ssize_t get_enable(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret;
	
	ret = sprintf(buf, "%d\n", spi_jz_test.full_duplex_enable);

	return ret;
}

static ssize_t get_status(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	int ret;
	
	ret = sprintf(buf, "%d\n", spi_jz_test.fixed_buf_test);

	return ret;
}

static ssize_t set_frame(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	unsigned long	frame;

	rc = strict_strtoul(buf, 0, &frame);
	if (rc)
		return rc;

	printk("Test case: frame is %ld\n",frame);

	if ((frame != 8) && (frame != 16) && (frame != 32))
		goto err;

	spi_jz_test.jz_spi->bits_per_word = frame;

	return size;
err:
	pr_err("frame is %ld\n",frame);
	pr_err("Please try input 8 or 16 or 32\n");
	return -EAGAIN;

}

static ssize_t set_buf(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t size)
{
	int rc;
	unsigned long	buffer_len;

	rc = strict_strtoul(buf, 0, &buffer_len);
	if (rc)
		return rc;

	printk("Test case: buffer length is %ld\n",buffer_len);
	if (spi_jz_test.jz_spi->bits_per_word)
		if (buffer_len % spi_jz_test.jz_spi->bits_per_word)
			goto err;

	spi_jz_test.buf_len = buffer_len;

	return size;
err:
	pr_err("buffer length is %ld\n",buffer_len);
	pr_err("Please try input again\n");
	return -EAGAIN;


}

static ssize_t set_enable(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t size)
{
	int rc;
	unsigned long	enable;

	rc = strict_strtoul(buf, 0, &enable);
	if (rc)
		return rc;

	if (spi_jz_test.jz_spi->bits_per_word && spi_jz_test.buf_len)
		if (spi_jz_test.buf_len % spi_jz_test.jz_spi->bits_per_word)
			goto err;

	if (enable == 1) {
		spi_jz_test.full_duplex_enable = 1;
		queue_work(spi_jz_test.workqueue, &spi_jz_test.full_duplex_work);
	} else if (enable == 0)
		spi_jz_test.full_duplex_enable = 0;
	else
		goto err;

	return size;
err:
	pr_err("Please try input correct frame and buf_len\n");
	return -EAGAIN;
}

static ssize_t set_status(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t size)
{
	int rc;
	unsigned long	status;

	rc = strict_strtoul(buf, 0, &status);
	if (rc)
		return rc;

	if (status == 1) {
		spi_jz_test.fixed_buf_test = 1;
	} else if (status == 0)
		spi_jz_test.fixed_buf_test = 0;
	else
		goto err;

	return size;
err:
	pr_err("Please try input correct status\n");
	return -EAGAIN;
}

static struct device_attribute attributes[] = {
	__ATTR(frame, 0666, get_frame, set_frame),
	__ATTR(buf, 0666, get_buf, set_buf),
	__ATTR(enable, 0666, get_enable, set_enable),
	__ATTR(fixed_buf_test_mode, 0666, get_status, set_status),
};

static int create_sysfs_interface(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto err;
	return 0;

err:
	for( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	return -1;
}

static int __init jz47xx_spi_test_probe(struct spi_device *spi)
{
	int ret;

	spi_jz_test.workqueue = create_singlethread_workqueue(
			"spi_jz_soc_test");
	if (spi_jz_test.workqueue == NULL) {
		goto err1;
	}
#if 0
	INIT_WORK(&spi_jz_test.read_work, read_work);
	INIT_WORK(&spi_jz_test.write_work, write_work);
#endif
	INIT_WORK(&spi_jz_test.full_duplex_work, full_duplex_work);

	spi_jz_test.jz_spi = spi;

	ret = create_sysfs_interface(&spi->dev);
	if (ret < 0)
		dev_err(&spi->dev, "spi_jz_test create sysfs interface failed\n");

	return 0;
err1:
	return -EBUSY;
}

static int __devexit jz_spi_remove(struct spi_device *spi)
{

	return 0;
}

static struct spi_driver jz_spi_driver = {
	.driver = {
		.name   = "spi_test",
		.bus    = &spi_bus_type,
		.owner  = THIS_MODULE,
	},
	.probe          = jz47xx_spi_test_probe,
	.remove         = __devexit_p(jz_spi_remove),
};

static int __init spi_jz_test_init(void)
{
	int ret;

	ret = spi_register_driver(&jz_spi_driver);
	if (ret != 0) {
		pr_err("Failed to register jz SPI driver: %d\n", ret);
	}

	return 0;
}

static void __exit spi_jz_test_exit(void)
{
	spi_unregister_driver(&jz_spi_driver);
}

module_init(spi_jz_test_init);
module_exit(spi_jz_test_exit);
