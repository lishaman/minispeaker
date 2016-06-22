/*
 * li-ion-charger.c - General Li-ion Charger driver.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Duke Fong <duke@dukelec.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>

#include <linux/delay.h>
#include <linux/power/li_ion_charger.h>
#include <linux/power/jz_battery.h>
#include <linux/mfd/pmu-common.h>
#include <linux/earlysuspend.h>



struct li_ion_charger {
	const struct li_ion_charger_platform_data *pdata;
	unsigned int irq_charge;
	unsigned int irq_ac;
	struct delayed_work work;
	int online;
	int status;
	struct power_supply charger;
	struct early_suspend early_suspend;
};


#ifdef DEBUG
static char* status_dbg(int status)
{
	switch (status) {
	case POWER_SUPPLY_STATUS_UNKNOWN:
		return "UNKNOWN";
	case POWER_SUPPLY_STATUS_CHARGING:
		return "CHARGING";
	case POWER_SUPPLY_STATUS_DISCHARGING:
		return "DISCHARGING";
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		return "NOT_CHARGING";
	case POWER_SUPPLY_STATUS_FULL:
		return "FULL";
	default:
		return "ERROR";
	}
}
#endif
static int is_ac_online(struct li_ion_charger *li_ion)
{
	return !gpio_get_value(li_ion->pdata->gpio_ac);
}
static int is_charge_online(struct li_ion_charger *li_ion)
{
	return !gpio_get_value(li_ion->pdata->gpio_charge);
}

static void update_battery(struct li_ion_charger *li_ion, int status)
{
	struct power_supply *psy_battery = power_supply_get_by_name("battery");
	struct jz_battery *jz_battery = container_of(psy_battery, struct jz_battery, battery);
	set_charger_offline(jz_battery, USB);
	if (status == POWER_SUPPLY_STATUS_NOT_CHARGING)
		set_charger_offline(jz_battery, AC);
	else
		set_charger_online(jz_battery, AC);
	jz_battery->status = status;
}

static void li_ion_work(struct work_struct *work)
{
	struct li_ion_charger *li_ion;
	int status;
	int online;

	li_ion = container_of(work, struct li_ion_charger, work.work);

	online = is_ac_online(li_ion);

	status = online ? POWER_SUPPLY_STATUS_CHARGING : POWER_SUPPLY_STATUS_NOT_CHARGING;
	if (status == POWER_SUPPLY_STATUS_CHARGING && !is_charge_online(li_ion)){
		status = POWER_SUPPLY_STATUS_FULL;
	}

	if ((li_ion->status != status)) {
		li_ion->status = status;
		update_battery(li_ion, status);
		power_supply_changed(&li_ion->charger);
	}

	li_ion->online = online;

	enable_irq(li_ion->irq_ac);
}

static irqreturn_t li_ion_irq(int irq, void *devid)
{
	struct li_ion_charger *li_ion = devid;

	if(irq == li_ion->irq_ac){
		disable_irq_nosync(li_ion->irq_ac);

		if(is_ac_online(li_ion))
			enable_irq(li_ion->irq_charge);

		else if(!is_ac_online(li_ion))
			disable_irq_nosync(li_ion->irq_charge);

	}

	schedule_delayed_work(&li_ion->work, msecs_to_jiffies(200));
	return IRQ_HANDLED;
}



static inline struct li_ion_charger *psy_to_li_ion(struct power_supply *psy)
{
	return container_of(psy, struct li_ion_charger, charger);
}

static int li_ion_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct li_ion_charger *li_ion = psy_to_li_ion(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = li_ion->online;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void li_ion_external_power_changed(struct power_supply *psy)
{
	struct li_ion_charger *li_ion = psy_to_li_ion(psy);

	if (li_ion->status == POWER_SUPPLY_STATUS_FULL && !is_ac_online(li_ion)) {
		pr_debug("li_ion: ac offline: FULL -> NOT_CHARGING\n");
		li_ion->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		update_battery(li_ion, li_ion->status);//
		power_supply_changed(&li_ion->charger);
	} else
		pr_debug("li_ion: ac changed (skip)\n");

}

static enum power_supply_property li_ion_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static int get_pmu_status(void *pmu_interface, int status)
{
	struct li_ion_charger *li_ion = (struct li_ion_charger *)pmu_interface;

	switch (status) {
	case AC:
		return is_ac_online(li_ion);
	case USB:
		return 0;
	case STATUS:
		return li_ion->status;
	}
	return -1;
}

static void pmu_work_enable(void *pmu_interface)
{
	struct li_ion_charger *li_ion = (struct li_ion_charger *)pmu_interface;

	pr_debug("li_ion: p_ion->workmu_work_enable\n");
	schedule_delayed_work(&li_ion->work, msecs_to_jiffies(200));
}

static void li_ion_callback_init(struct li_ion_charger *li_ion)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct jz_battery *jz_battery;
	jz_battery = container_of(psy, struct jz_battery, battery);
	jz_battery->pmu_interface = li_ion;
	jz_battery->get_pmu_status = get_pmu_status;
	jz_battery->pmu_work_enable = pmu_work_enable;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void li_ion_early_suspend(struct early_suspend *early_suspend)
{
	struct li_ion_charger *li_ion ;
	li_ion = container_of(early_suspend, struct li_ion_charger, early_suspend);
	disable_irq_nosync(li_ion->irq_ac);
	disable_irq_nosync(li_ion->irq_charge);
}
static void li_ion_late_resume(struct early_suspend *early_suspend)
{
	struct li_ion_charger *li_ion ;
	li_ion = container_of(early_suspend, struct li_ion_charger, early_suspend);
	enable_irq(li_ion->irq_ac);
	enable_irq(li_ion->irq_charge);
}
#endif

static int __devinit li_ion_charger_probe(struct platform_device *pdev)
{
	const struct li_ion_charger_platform_data *pdata = pdev->dev.platform_data;
	struct li_ion_charger *li_ion;
	struct power_supply *charger;
	int ret;
	int irq;

	if (!pdata) {
		dev_err(&pdev->dev, "No platform data\n");
		return -EINVAL;
	}

	if (!gpio_is_valid(pdata->gpio_ac)) {
		dev_err(&pdev->dev, "Invalid gpio pin\n");
		return -EINVAL;
	}

	if (!gpio_is_valid(pdata->gpio_charge)) {
		dev_err(&pdev->dev, "Invalid gpio pin\n");
		return -EINVAL;
	}
	li_ion = kzalloc(sizeof(*li_ion), GFP_KERNEL);
	if (!li_ion) {
		dev_err(&pdev->dev, "Failed to alloc driver structure\n");
		return -ENOMEM;
	}

	charger = &li_ion->charger;
	charger->name = "li_ion_charge";
	charger->type = POWER_SUPPLY_TYPE_MAINS;
	charger->properties = li_ion_properties;
	charger->num_properties = ARRAY_SIZE(li_ion_properties);
	charger->get_property = li_ion_get_property;
	charger->supplied_to = supply_list;
	charger->num_supplicants = ARRAY_SIZE(supply_list);
	charger->external_power_changed = li_ion_external_power_changed;

#ifdef CONFIG_HAS_EARLYSUSPEND
	li_ion->early_suspend.suspend = li_ion_early_suspend;
	li_ion->early_suspend.resume = li_ion_late_resume;
	li_ion->early_suspend.level	= EARLY_SUSPEND_LEVEL_DISABLE_FB;
	register_early_suspend(&li_ion->early_suspend);
#endif

	ret = gpio_request(pdata->gpio_charge, dev_name(&pdev->dev));
	if (ret) {
		dev_err(&pdev->dev, "Failed to request gpio pin: %d\n", ret);
		goto err_free;
	}

	ret = gpio_direction_input(pdata->gpio_charge);
	if (ret) {
		dev_err(&pdev->dev, "Failed to set gpio to input: %d\n", ret);
		goto err_gpio_free_charge;
	}

	ret = gpio_request(pdata->gpio_ac, dev_name(&pdev->dev));
	if (ret) {
		dev_err(&pdev->dev, "Failed to request gpio pin: %d\n", ret);
		goto err_free;
	}
	ret = gpio_direction_input(pdata->gpio_ac);
	if (ret) {
		dev_err(&pdev->dev, "Failed to set gpio to input: %d\n", ret);
		goto err_gpio_free_ac;
	}

	li_ion->pdata = pdata;
	li_ion->online = -1;
	li_ion->status = POWER_SUPPLY_STATUS_UNKNOWN;

	ret = power_supply_register(&pdev->dev, charger);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register power supply: %d\n",
			ret);
		goto err_gpio_free_charge;
	}

	INIT_DELAYED_WORK(&li_ion->work, li_ion_work);

	irq = gpio_to_irq(pdata->gpio_charge);
	if (irq > 0) {
		ret = request_any_context_irq(irq, li_ion_irq,
               IRQF_TRIGGER_RISING,
				"li_ion_charge", li_ion);
		if (ret){
			dev_warn(&pdev->dev, "Failed to request irq: %d\n", ret);
		} else {
			li_ion->irq_charge = irq;
			if(!is_ac_online(li_ion))
				disable_irq_nosync(li_ion->irq_charge);
		}
	}

	irq = gpio_to_irq(pdata->gpio_ac);
	if(irq > 0) {
			ret = request_any_context_irq(irq, li_ion_irq,
                IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"li_ion_ac", li_ion);
		if (ret){
			dev_warn(&pdev->dev, "Failed to request irq: %d\n", ret);
		} else {
			li_ion->irq_ac = irq;
			disable_irq_nosync(li_ion->irq_ac);
		}
	}

	li_ion_callback_init(li_ion);
	platform_set_drvdata(pdev, li_ion);

	return 0;

err_gpio_free_ac:
	gpio_free(pdata->gpio_ac);
err_gpio_free_charge:
	gpio_free(pdata->gpio_charge);
err_free:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&li_ion->early_suspend);
#endif
	kfree(li_ion);
	return ret;
}

static int __devexit  li_ion_charger_remove(struct platform_device *pdev)
{
	struct li_ion_charger *li_ion = platform_get_drvdata(pdev);

	if (li_ion->irq_ac)
		free_irq(li_ion->irq_ac, &li_ion->charger);

	if (li_ion->irq_charge)
		free_irq(li_ion->irq_charge, &li_ion->charger);

	power_supply_unregister(&li_ion->charger);

	gpio_free(li_ion->pdata->gpio_ac);
	gpio_free(li_ion->pdata->gpio_charge);

	platform_set_drvdata(pdev, NULL);
	kfree(li_ion);

	return 0;
}



#ifdef CONFIG_PM_SLEEP
static int li_ion_charger_resume(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	struct li_ion_charger *li_ion = platform_get_drvdata(pdev);

	power_supply_changed(&li_ion->charger);

#endif
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(li_ion_charger_pm_ops, NULL, li_ion_charger_resume);

static struct platform_driver li_ion_charger_driver = {
	.probe = li_ion_charger_probe,
	.remove = __devexit_p(li_ion_charger_remove),
	.driver = {
		.name = "li-ion-charger",
		.owner = THIS_MODULE,
		.pm = &li_ion_charger_pm_ops,
	},
};

static int __init li_ion_charger_init(void)
{
	return platform_driver_register(&li_ion_charger_driver);
}
module_init(li_ion_charger_init);

static void __exit li_ion_charger_exit(void)
{
	platform_driver_unregister(&li_ion_charger_driver);
}
module_exit(li_ion_charger_exit);

MODULE_AUTHOR("Duke Fong <duke@dukelec.com>");
MODULE_DESCRIPTION("Driver for general Li-ion charger "
		   "which report their charge status through GPIO");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:li-ion-charger");
