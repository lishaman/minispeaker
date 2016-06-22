/**
 * drivers/mfd/jz4785-adc-aux.c
 *
 * aux1 aux2 channels voltage sample interface for Ingenic SoC
 *
 * Copyright(C)2012 Ingenic Semiconductor Co., LTD.
 * http://www.ingenic.cn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/mfd/core.h>
#include <linux/delay.h>
#include <linux/jz4780-adc.h>
#include <linux/vmalloc.h>/*for vfree*/
#include <linux/miscdevice.h>

#include <irq.h>

#define AVDD33     3300
#define AUXCONST   4096
#define OUTPUTR4   4700
#define OUTPUTR5   5400
#define OUTPUTR6   6800
#define DEFAULT_OUTPUT	OUTPUTR5

#define MODEPRM_1680    61
#define MODEPRM_1620    57
#define DEFAULT_MODEPRM	MODEPRM_1620

#define HMODE      10000	// 100 lux
#define MMODE      100000	// 1000 lux
#define DEFAULT_MODE	MMODE

#ifndef BITS_H2L
#define BITS_H2L(msb, lsb)  ((0xFFFFFFFF >> (32-((msb)-(lsb)+1))) << (lsb))
#endif

struct bh1680fvc {
	struct platform_device *pdev;

	struct resource *mem;
	void __iomem *base;

	int irq;

	const struct mfd_cell *cell;

	unsigned int voltage;

	struct completion read_completion;

	struct miscdevice mdev;
};

struct bh1680fvc *bh1680fvc = NULL;

enum aux_ch {
	SADC_AUX1 = 1,
	SADC_AUX2,
};
extern int jz_adc_set_config(struct device *dev, uint32_t mask, uint32_t val);

static irqreturn_t jz_bh1680fvc_irq_handler(int irq, void *devid)
{
	struct bh1680fvc *bh1680fvc = (struct bh1680fvc *)devid;

	complete(&bh1680fvc->read_completion);

	return IRQ_HANDLED;
}

static int bh1680fvc_suspend(struct platform_device *pdev, pm_message_t state)
{
	bh1680fvc->cell->disable(bh1680fvc->pdev);
	return 0;
}

static int bh1680fvc_resume(struct platform_device *pdev)
{
	bh1680fvc->cell->enable(bh1680fvc->pdev);
	return 0;
}

unsigned int bh1680fvc_read_value(struct bh1680fvc *bh1680fvc, uint8_t config)
{
	unsigned long tmp;
	unsigned int value;

	INIT_COMPLETION(bh1680fvc->read_completion);

	jz_adc_set_config(bh1680fvc->pdev->dev.parent, ADCFG_CMD_MASK, config);

	bh1680fvc->cell->enable(bh1680fvc->pdev);
	enable_irq(bh1680fvc->irq);

	tmp = wait_for_completion_interruptible_timeout(&bh1680fvc->read_completion, HZ);
	if (tmp > 0) {
		value = readw(bh1680fvc->base) & 0xfff;
	} else {
		value = tmp ? tmp : -ETIMEDOUT;
	}

	if (value < 0) {
		printk("bh1680fvc read value error!!\n");
		return -EIO;
	}

	disable_irq(bh1680fvc->irq);
	bh1680fvc->cell->disable(bh1680fvc->pdev);

	return value;
}

int bh1680fvc_sample_volt(enum aux_ch channels)
{
	uint8_t config = 0;
	int i = 0, sadc_volt = 0,sadc_volt_max = 0, sadc_volt_min = 0, sadc_volt_sum = 0;

	if (!bh1680fvc) {
		printk("bh1680fvc is null ! return\n");
		return -EINVAL;
	}

	config = ADCFG_CMD_AUX(channels);
	for (i= 0; i < 5; i ++) {
		if (!i) {
			sadc_volt = sadc_volt_min = sadc_volt_max = bh1680fvc_read_value(bh1680fvc, config);
		} else {
			sadc_volt = bh1680fvc_read_value(bh1680fvc, config);
			sadc_volt_min = min(sadc_volt, sadc_volt_min);
			sadc_volt_max = max(sadc_volt, sadc_volt_max);
		}
		sadc_volt_sum += sadc_volt;
	}
	sadc_volt = (sadc_volt_sum - sadc_volt_min - sadc_volt_max) / 3;
	sadc_volt = sadc_volt * AVDD33 / AUXCONST;

	return sadc_volt;
}


int bh1680fvc_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int bh1680fvc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t bh1680fvc_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
	int sadc_val = 0;

	if (len < sizeof(int)) {
		printk("Parameters of 'len' must be sizeof(int)\n");
		return -EINVAL;
	}

	sadc_val = bh1680fvc_sample_volt(SADC_AUX2);
	if (sadc_val < 0) {
		printk("bh1680fvc read value error !!\n");
		return -EINVAL;
	}
	sadc_val = sadc_val * DEFAULT_MODE / ( DEFAULT_OUTPUT * DEFAULT_MODEPRM );

	if(copy_to_user(buf, &sadc_val, sizeof(int))) {
		return -EFAULT;
	}

	return sizeof(int);
}


struct file_operations bh1680fvc_fops= {
	.owner= THIS_MODULE,
	.open= bh1680fvc_open,
	.release= bh1680fvc_release,
	.read= bh1680fvc_read,
};

static int __devinit bh1680fvc_probe(struct platform_device *pdev)
{
	int ret = 0;

	bh1680fvc = kzalloc(sizeof(*bh1680fvc), GFP_KERNEL);
	if (!bh1680fvc) {
		dev_err(&pdev->dev, "Failed to allocate driver structre\n");
		return -ENOMEM;
	}

	bh1680fvc->cell = mfd_get_cell(pdev);
	if (!bh1680fvc->cell) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get mfd cell for bh1680fvc!\n");
		goto err_free;
	}

	bh1680fvc->irq = platform_get_irq(pdev, 0);
	if (bh1680fvc->irq < 0) {
		ret = bh1680fvc->irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto err_free;
	}

	bh1680fvc->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!bh1680fvc->mem) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get platform mmio resource\n");
		goto err_free;
	}

	bh1680fvc->mem = request_mem_region(bh1680fvc->mem->start,
			resource_size(bh1680fvc->mem), pdev->name);
	if (!bh1680fvc->mem) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		goto err_free;
	}

	bh1680fvc->base = ioremap_nocache(bh1680fvc->mem->start,resource_size(bh1680fvc->mem));
	if (!bh1680fvc->base) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		goto err_free;
	}

	bh1680fvc->pdev = pdev;
	bh1680fvc->mdev.minor = MISC_DYNAMIC_MINOR;
	bh1680fvc->mdev.name =  "bh1680fvc_aux";
	bh1680fvc->mdev.fops = &bh1680fvc_fops;

	ret = misc_register(&bh1680fvc->mdev);
	if (ret < 0) {
		dev_err(&pdev->dev, "misc_register failed\n");
		goto err_free;
	}

	init_completion(&bh1680fvc->read_completion);

	ret = request_irq(bh1680fvc->irq, jz_bh1680fvc_irq_handler, 0, pdev->name, bh1680fvc);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %d\n", ret);
		goto err_free;
	}

	disable_irq(bh1680fvc->irq);

	platform_set_drvdata(pdev, bh1680fvc);

	return 0;

err_free :
	kfree(bh1680fvc);
	return ret;

}

static int __devexit bh1680fvc_remove(struct platform_device *pdev)
{
	struct bh1680fvc *bh1680fvc = platform_get_drvdata(pdev);

	misc_deregister(&bh1680fvc->mdev);
	free_irq(bh1680fvc->irq, bh1680fvc);
	iounmap(bh1680fvc->base);
	release_mem_region(bh1680fvc->mem->start,resource_size(bh1680fvc->mem));
	kfree(bh1680fvc);

	return 0;
}

static struct platform_driver bh1680fvc_driver = {
	.probe	= bh1680fvc_probe,
	.remove	= __devexit_p(bh1680fvc_remove),
	.driver = {
		.name	= "jz-hwmon",
		.owner	= THIS_MODULE,
	},
	.suspend	= bh1680fvc_suspend,
	.resume		= bh1680fvc_resume,
};

static int __init bh1680fvc_init(void)
{
	platform_driver_register(&bh1680fvc_driver);

	return 0;
}

static void __exit bh1680fvc_exit(void)
{
	platform_driver_unregister(&bh1680fvc_driver);
}

module_init(bh1680fvc_init);
module_exit(bh1680fvc_exit);

MODULE_ALIAS("platform: JZ bh1680fvc");
MODULE_AUTHOR("Liu Yanbin<ybliu_hf@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("JZ adc aux sample driver");
