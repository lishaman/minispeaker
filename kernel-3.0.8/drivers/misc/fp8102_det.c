/*
 * Sample uevent report implementation
 *
 * Copyright (C) 2015-2017 jbxu <jbxu@ingenic.com>
 * Copyright (C) 2015 Novell Inc.
 *
 * Released under the GPL version 2 only.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <asm/irq.h>
#include <mach/fp8102_det.h>

int irqnum = 0;
struct uevent_platform_data* uinfo = NULL;
static struct kset *uevent_kset = NULL;
static struct work_struct uevent_wq;

static char charged_str[] = "fully-charged";
static char charging_str[] = "charging";

struct uevent_obj {
	struct kobject kobj;
	int val;
};
struct uevent_obj *uobj;

struct uevent_attribute {
	struct attribute attr;
	ssize_t (*show)(struct uenent_obj *uobj, struct attribute *attr, char *buf);
	ssize_t (*store)(struct uevent_obj *uobj, struct attribute *attr, const char *buf, size_t count);
};

static ssize_t uevent_attr_show(struct kobject *kobj,
			struct attribute *attr,
			char *buf)
{
	ssize_t count;
	if (gpio_get_value(uinfo->detect_pin) ^ uinfo->charge_active_low)
		count = sprintf(buf, "%s\n", charging_str);
	else
		count = sprintf(buf, "%s\n", charged_str);
	return  count;
}

static ssize_t uevent_attr_store(struct kobject *kobj,
			struct attribute *attr,
			const char *buf, size_t len)
{
	return 0;
}

static const struct sysfs_ops uevent_sysfs_ops = {
	.show = uevent_attr_show,
	.store = uevent_attr_store,
};

static void uevent_release(struct kobject *kobj)
{
	kfree(uobj);
}

static ssize_t uevent_show(struct uevent_obj *uevent_obj, struct attribute *attr,
			char *buf)
{
	/*do nothing*/
	return  0;
}

static ssize_t uevent_store(struct uvent_obj *uevent_obj, struct attribute *attr,
			 const char *buf, size_t count)
{
	/*do nothing*/
	return  0;
}

static struct uevent_attribute fp8102_uevent_attr =
	__ATTR(uevent, 0666, uevent_show, uevent_store);

static struct attribute *uevent_default_attrs[] = {
	&fp8102_uevent_attr.attr,
	NULL,
};

static struct kobj_type uevent_ktype = {
	.sysfs_ops = &uevent_sysfs_ops,
	.release = uevent_release,
	.default_attrs = uevent_default_attrs,
};

static void destroy_uevent_obj(struct uevent_obj *ub)
{
	kobject_put(&ub->kobj);
	kfree(&ub->kobj);
}

void uevent_do_work(struct work_struct *work)
{
	char *msg[] = {NULL, NULL};
	if (gpio_get_value(uinfo->detect_pin) ^ uinfo->charge_active_low)
		msg[0] = charging_str;
	else
		msg[0] = charged_str;

	kobject_uevent_env(&uobj->kobj, KOBJ_CHANGE, msg);
}

static irqreturn_t interrupt_isr(int irq, void *data)
{
	schedule_work(&uevent_wq);
	return IRQ_HANDLED;
}

static int fp8102_det_probe( struct platform_device *pdev)
{
	int ret = 0;
	uinfo = pdev->dev.platform_data;
	if(!uinfo){
		printk("get uevent pdata failed \n");
		return -EINVAL;
	}

	uevent_kset = kset_create_and_add("uevent_report", NULL, kernel_kobj);
	if (!uevent_kset){
		return -ENOMEM;
	}
	uobj = kzalloc(sizeof(*uobj), GFP_KERNEL);
	if (!uobj){
		goto kset_error;
	}

	INIT_WORK(&uevent_wq, uevent_do_work);

	uobj->kobj.kset = uevent_kset;
	ret = kobject_init_and_add(&uobj->kobj, &uevent_ktype, NULL, "%s", "uevent_report");
	if (ret){
		goto uevent_error;
	}

	ret = gpio_request(uinfo->detect_pin, "uevent_report");
	if (ret){
		printk("gpio reuest for uevent report failed\n");
		goto uevent_error;
	}

	irqnum = gpio_to_irq(uinfo->detect_pin);
	if (request_irq(irqnum, interrupt_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "uevent_report", NULL)) {
		printk(KERN_ERR "uevent.c: Can't allocate irq %d\n", irqnum);
		goto gpio_error;
	}

	return 0;

gpio_error:
	gpio_free(uinfo->detect_pin);
uevent_error:
	destroy_uevent_obj(uobj);
kset_error:
	kset_unregister(uevent_kset);
	return -EINVAL;
}

static int fp8102_det_remove(struct platform_device *dev)
{
	destroy_uevent_obj(uobj);
	kset_unregister(uevent_kset);
	free_irq(irqnum, NULL);
	gpio_free(uinfo->detect_pin);
	return 0;
}

static struct platform_driver fp8102_det_driver = {
	.driver = {
		.name = "fp8102_det",
		.owner = THIS_MODULE,
	},
	.probe = fp8102_det_probe,
	.remove = fp8102_det_remove,
};

static int __init fp8102_det_init(void)
{
	return platform_driver_register(&fp8102_det_driver);
}

static void __exit fp8102_det_exit(void)
{
	platform_driver_unregister(&fp8102_det_driver);
}

module_init(fp8102_det_init);
module_exit(fp8102_det_exit);

MODULE_DESCRIPTION("fp8102 charge status report");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("jbxu");
