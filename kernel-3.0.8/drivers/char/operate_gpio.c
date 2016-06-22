/* drivers/mfd/operate_gpio.c
 *
 * Simulation key using GPIO, in order to test the stability of the stage sleep and wake.
 *
 * Copyright(C)2012 Ingenic Semiconductor Co., LTD.
 *	http://www.ingenic.cn
 *	Sun Jiwei <jwsun@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Sooftware Foundation; either version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>

static unsigned int parameter_to_gpio(const char *p)
{
	unsigned int gpio;
	unsigned int tmp;

	char *q = (char *)p;

	if ((*q > 'F') || (*q < 'A'))
		return 193;

	gpio = (*q - 'A') * 32;

	q++;

	if ((*q == '\0') || (*q == '\n')) return 193;

	for (tmp = 0; (*q != '\0') && (*q != '\n'); q++) {
		if ((*q >= '0') && (*q <= '9')) {
			tmp *=  10;
			tmp += (*q - '0');
		} else {
			return 193;
		}
	}

	if (tmp > 31) return 193;

	gpio += tmp;

	return gpio;
}

static void print_gpio(const char *p)
{
	char *q = (char *)p;

	printk("The gpio is ");

	while (*q != '\0') {
		printk("%c",*q);
		q++;
	}

	printk("\n");
}

static int proc_write_apply(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[5] = {'\0'};
	char *p = (char *)buf;
	unsigned int gpio;

	if ((count > 4) || (count < 2)) {
		printk("Input char count = %d\n",(int)count);
		goto error;
	}

	if (copy_from_user(buf, buffer, count)) {
		printk("copy data from user error\n");
		return -EFAULT;
	}
	print_gpio(p);

	gpio = parameter_to_gpio(p);
	if (gpio >= 193) {
		goto error;
	}

	if (gpio_request_one(gpio, GPIOF_OUT_INIT_HIGH, "Operate GPIO")) {
		printk("GPIO %d is used\n", gpio);
		printk("Please use:  \n"
			"	echo GPIO > /proc/operate_gpio/release_gpio \n"
			"to relase GPIO\n\n"
			"For example:\n"
			"	echo A25 > /proc/operate_gpio/release_gpio \n"
			);

		return -EFAULT;
	}

	return count;

error:
	printk("The parameter is error!\n");
	return -EFAULT;
}

static int proc_write_release(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[5] = {'\0'};
	char *p = (char *)buf;
	unsigned int gpio;

	if ((count > 4) || (count < 2)) {
		printk("The parameter is error!\n");
		return -EFAULT;
	}

	if (copy_from_user(buf, buffer, count)) {
		printk("copy data from user error\n");
		return -EFAULT;
	}
	print_gpio(p);

	gpio = parameter_to_gpio(p);
	if (gpio >= 193) {
		printk("The parameter is error!\n");
		return -EFAULT;
	}

	gpio_free(gpio);

	return count;

}

static int proc_write_high(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[5] = {'\0'};
	char *p = (char *)buf;
	unsigned int gpio;

	if ((count > 4) || (count < 2)) {
		printk("The parameter is error!\n");
		return -EFAULT;
	}

	if (copy_from_user(buf, buffer, count)) {
		printk("copy data from user error\n");
		return -EFAULT;
	}

	gpio = parameter_to_gpio(p);
	if (gpio >= 193) {
		printk("The parameter is error!\n");
		return -EFAULT;
	}

	gpio_set_value(gpio, 1);

	return count;
}

static int proc_write_low(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[5] = {'\0'};
	char *p = (char *)buf;
	unsigned int gpio;

	if ((count > 4) || (count < 2)) {
		printk("The parameter is error!\n");
		return -EFAULT;
	}

	if (copy_from_user(buf, buffer, count)) {
		printk("copy data from user error\n");
		return -EFAULT;
	}

	gpio = parameter_to_gpio(p);
	if (gpio >= 193) {
		printk("The parameter is error!\n");
		return -EFAULT;
	}

	gpio_set_value(gpio, 0);

	return count;
}

static int __init operate_gpio_init(void)
{
	struct proc_dir_entry *root;
	struct proc_dir_entry *res;

	root = proc_mkdir("operate_gpio", 0);

	res = create_proc_entry("apply_gpio", 0666, root);
	if (res) {
		res->write_proc = proc_write_apply;
	}

	res = create_proc_entry("release_gpio", 0666, root);
	if (res) {
		res->write_proc = proc_write_release;
	}

	res = create_proc_entry("output_high", 0666, root);
	if (res) {
		res->write_proc = proc_write_high;
	}

	res = create_proc_entry("output_low", 0666, root);
	if (res) {
		res->write_proc = proc_write_low;
	}

	return 0;
}
module_init(operate_gpio_init);

static void __exit operate_gpio_exit(void)
{

}
module_exit(operate_gpio_exit);

MODULE_ALIAS("operate_gpio");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sun Jiwei<jwsun@ingenic.cn>");
MODULE_DESCRIPTION("Operation GPIO accord proc entry");
