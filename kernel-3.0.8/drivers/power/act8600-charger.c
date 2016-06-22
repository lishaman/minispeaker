/*
 * act8600_charger.c - ACT8600 Charger driver, based on ACT8600 mfd driver.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Large Dipper <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/power/jz4780-battery.h>
#include <linux/mfd/act8600-private.h>
#include <linux/mfd/pmu-common.h>
#include <linux/err.h>

static enum power_supply_property act8600_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

#if defined(CONFIG_USB_DWC2) && defined(CONFIG_USB_CHARGER_SOFTWARE_JUDGE) && defined(CONFIG_REGULATOR)
extern int dwc2_register_state_notifier(struct notifier_block *nb);
extern void dwc2_unregister_state_notifier(struct notifier_block *nb);

struct act8600_notify_st {
	struct notifier_block mode_notify;
	struct regulator  *usb_charger;
	struct i2c_client *client;
};

static int dwc2_state_notifier(struct notifier_block * nb, unsigned long is_charger, void * unused)
{
	struct act8600_notify_st  *notify_st =
		container_of(nb, struct act8600_notify_st, mode_notify);
	struct regulator *usb_charger =
		(struct regulator *)notify_st->usb_charger;
	int	curr_limit = 0;
	uint8_t otg_con;

	if (notify_st->client && usb_charger) {
		act8600_read_reg(notify_st->client,OTG_CON,&otg_con);
		if (((!!otg_con & VBUSDAT) && (!!is_charger)) ||
				(!is_charger)) {
			curr_limit = regulator_get_current_limit(usb_charger);
			printk("Before changed: the current is %d\n", curr_limit);
			if (!!is_charger) {
				/*charger...*/
				regulator_set_current_limit(usb_charger, 0, 800000);
			} else {
				/*host...*/
				regulator_set_current_limit(usb_charger, 0, 400000);
			}
			curr_limit = regulator_get_current_limit(usb_charger);
			printk("After changed: the current is %d\n", curr_limit);

		}
	}
	return 0;
}
#endif

struct act8600_charger {
	struct device *dev;
	struct act8600 *iodev;
	struct delayed_work work;
	int irq;

	struct power_supply usb;
	struct power_supply ac;

#if defined(CONFIG_USB_DWC2) && defined(CONFIG_USB_CHARGER_SOFTWARE_JUDGE) && defined(CONFIG_REGULATOR)
	struct act8600_notify_st *notify_st;
#endif
};


static int act8600_get_charge_state(struct act8600_charger *charger,
		struct jz_battery *jz_battery)
{
	struct act8600 *iodev = charger->iodev;
	unsigned char chgst;

	act8600_read_reg(iodev->client, APCH_STAT, &chgst);

	switch(chgst & CSTATE_MASK) {
	case CSTATE_EOC:
		jz_battery->status = POWER_SUPPLY_STATUS_FULL;
		return POWER_SUPPLY_STATUS_FULL;
	case CSTATE_PRE:
	case CSTATE_CHAGE:
		jz_battery->status = POWER_SUPPLY_STATUS_CHARGING;
		return POWER_SUPPLY_STATUS_CHARGING;
	case CSTATE_SUSPEND:
		jz_battery->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
	return 0;
}

static int act8600_get_ac_state(struct act8600_charger *charger,
		struct jz_battery *jz_battery)
{
#if CONFIG_CHARGER_HAS_AC
	struct act8600 *iodev = charger->iodev;
	unsigned char intr1;

	act8600_read_reg(iodev->client, APCH_INTR1, &intr1);

	if ((intr1 & INDAT) != 0) {
		set_charger_online(jz_battery, AC);
		act8600_write_reg(iodev->client, APCH_INTR1, INSTAT);
		act8600_write_reg(iodev->client, APCH_INTR2, INDIS);
		return 1;
	} else {
		set_charger_offline(jz_battery, AC);
		act8600_write_reg(iodev->client, APCH_INTR1, INSTAT);
		act8600_write_reg(iodev->client, APCH_INTR2, INCON);
		return 0;
	}
#else
	return 0;
#endif
}

static int act8600_get_usb_state(struct act8600_charger *charger,
		struct jz_battery *jz_battery)
{
#if CONFIG_CHARGER_HAS_USB
	struct act8600 *iodev = charger->iodev;
	unsigned char otg_con;

	act8600_read_reg(iodev->client, OTG_CON, &otg_con);

	if ((otg_con & VBUSDAT) && !(otg_con & ONQ1)) {
		set_charger_online(jz_battery, USB);
		act8600_write_reg(iodev->client, OTG_INTR, INVBUSF);
		return 1;
	} else {
		set_charger_offline(jz_battery, USB);
		act8600_write_reg(iodev->client, OTG_INTR, INVBUSR);
		return 0;
	}
#else
	return 0;
#endif
}

static int get_pmu_status(void *pmu_interface, int status)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct jz_battery *jz_battery;
	struct act8600_charger *charger = (struct act8600_charger *)pmu_interface;

	jz_battery = container_of(psy, struct jz_battery, battery);

	switch (status) {
	case STATUS:
		return act8600_get_charge_state(charger, jz_battery);
	case AC:
		return act8600_get_ac_state(charger, jz_battery);
	case USB:
		return act8600_get_usb_state(charger, jz_battery);
	}
	return -1;
}

static unsigned int act8600_update_status(struct act8600_charger *charger)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct jz_battery *jz_battery;
	struct act8600 *iodev = charger->iodev;
	unsigned char chgst,intr1,otg_con;
	unsigned int charger_pre;
	static char *status_text[] = {
		"Unknown", "Charging", "Discharging", "Not charging", "Full"
	};

	jz_battery = container_of(psy, struct jz_battery, battery);
	charger_pre = jz_battery->charger;

#if CONFIG_CHARGER_HAS_AC
	act8600_read_reg(iodev->client, APCH_INTR1, &intr1);

	if (((intr1 & INDAT) != 0)) {
		set_charger_online(jz_battery, AC);
		act8600_write_reg(iodev->client, APCH_INTR1, INSTAT);
		act8600_write_reg(iodev->client, APCH_INTR2, INDIS);
	} else {
		set_charger_offline(jz_battery, AC);
		act8600_write_reg(iodev->client, APCH_INTR1, INSTAT);
		act8600_write_reg(iodev->client, APCH_INTR2, INCON);
	}
#endif
#if CONFIG_CHARGER_HAS_USB
	act8600_read_reg(iodev->client, OTG_CON, &otg_con);

	if ((otg_con & VBUSDAT) && !(otg_con & ONQ1)) {
		set_charger_online(jz_battery, USB);
		act8600_write_reg(iodev->client, OTG_INTR, INVBUSF);
	} else {
		set_charger_offline(jz_battery, USB);
		act8600_write_reg(iodev->client, OTG_CON , otg_con & ~DBILIMQ3);		//current limit to 400mA
		act8600_write_reg(iodev->client, OTG_INTR, INVBUSR);
	}
#endif

	act8600_read_reg(iodev->client, APCH_STAT, &chgst);

	switch (chgst & CSTATE_MASK) {
	case CSTATE_EOC:
		jz_battery->status = POWER_SUPPLY_STATUS_FULL;
		break;
	case CSTATE_PRE:
	case CSTATE_CHAGE:
		jz_battery->status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case CSTATE_SUSPEND:
		jz_battery->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	}

	pr_info("Battery: %s \tUSB: %s\tAC: %s\n",
		status_text[jz_battery->status],
		(get_charger_online(jz_battery, USB) ? "online" : "offline"),
		(get_charger_online(jz_battery, AC) ? "online" : "offline"));

	return charger_pre ^ jz_battery->charger;
}

static void act8600_charger_work(struct work_struct *work)
{
	unsigned int changed_psy;
	struct act8600_charger *charger;

	charger = container_of(work, struct act8600_charger, work.work);
	changed_psy = act8600_update_status(charger);

	if (changed_psy & (1 << USB))
		power_supply_changed(&charger->usb);
	if (changed_psy & (1 << AC))
		power_supply_changed(&charger->ac);

	enable_irq(charger->irq);
}

static irqreturn_t act8600_charger_irq(int irq, void *data)
{
	struct act8600_charger *charger = data;

	disable_irq_nosync(charger->irq);
	schedule_delayed_work(&charger->work, HZ);

	return IRQ_HANDLED;
}

static void pmu_work_enable(void *pmu_interface)
{
	struct act8600_charger *charger = (struct act8600_charger *)pmu_interface;

	schedule_delayed_work(&charger->work, 0);
}

static int act8600_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct power_supply *psy_bat = power_supply_get_by_name("battery");
	struct jz_battery *jz_battery;

	jz_battery = container_of(psy_bat, struct jz_battery, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((psy->type == POWER_SUPPLY_TYPE_MAINS) &&
			((jz_battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
			 (jz_battery->status == POWER_SUPPLY_STATUS_FULL))) {
			val->intval = get_charger_online(jz_battery, AC);
		} else if((psy->type == POWER_SUPPLY_TYPE_USB) &&
			((jz_battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
			 (jz_battery->status == POWER_SUPPLY_STATUS_FULL))) {
			val->intval = get_charger_online(jz_battery, USB);
		} else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void power_supply_init(struct act8600_charger *charger)
{
#define DEF_POWER(PSY, NAME, TYPE)						\
	charger->PSY.name = NAME;						\
	charger->PSY.type = TYPE;						\
	charger->PSY.supplied_to = supply_list;					\
	charger->PSY.num_supplicants = ARRAY_SIZE(supply_list);			\
	charger->PSY.properties = act8600_power_properties;			\
	charger->PSY.num_properties = ARRAY_SIZE(act8600_power_properties);	\
	charger->PSY.get_property = act8600_get_property

	DEF_POWER(usb, "usb", POWER_SUPPLY_TYPE_USB);
	DEF_POWER(ac, "ac", POWER_SUPPLY_TYPE_MAINS);
#undef DEF_POWER
	power_supply_register(charger->dev, &charger->usb);
	power_supply_register(charger->dev, &charger->ac);
}

static int charger_init(struct act8600_charger *charger)
{
	struct act8600 *iodev = charger->iodev;
	unsigned char intr1, intr2, otgcon;

	act8600_read_reg(iodev->client, APCH_INTR1, &intr1);
	act8600_write_reg(iodev->client, APCH_INTR1,
			  intr1 | CHGSTAT | INSTAT | TEMPSTAT);

	act8600_read_reg(iodev->client, APCH_INTR2, &intr2);
	act8600_write_reg(iodev->client, APCH_INTR2,
			  intr2 | CHGEOCOUT | CHGEOCIN
			  | INDIS | INCON | TEMPOUT | TEMPIN);

	act8600_write_reg(iodev->client, OTG_INTR, INVBUSF | INVBUSR);

	act8600_read_reg(iodev->client, OTG_CON, &otgcon);
	act8600_write_reg(iodev->client, OTG_CON, otgcon | VBUSSTAT);

	return 0;
}

static void set_max_charger_current(struct charger_board_info *charger_board_info)
{
	if (charger_board_info->gpio < 0) {
		return;
	}

	gpio_set_value(charger_board_info->gpio, charger_board_info->enable_level);
}

static void act8600_callback_init(struct act8600_charger *charger)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct jz_battery *jz_battery;
	jz_battery = container_of(psy, struct jz_battery, battery);

	jz_battery->pmu_interface = charger;
	jz_battery->get_pmu_status = get_pmu_status;
	jz_battery->pmu_work_enable = pmu_work_enable;
}
static int __devinit act8600_charger_probe(struct platform_device *pdev)
{
	struct act8600 *iodev = dev_get_drvdata(pdev->dev.parent);
	struct pmu_platform_data *pdata = dev_get_platdata(iodev->dev);
	struct act8600_charger *charger;
	int ret = 0;

	if (!pdata) {
		dev_err(&pdev->dev, "No platform_data supplied\n");
		return -ENXIO;
	}

	charger = kzalloc(sizeof(struct act8600_charger), GFP_KERNEL);
	if (!charger) {
		dev_err(&pdev->dev, "Failed to allocate driver structure\n");
		return -ENOMEM;
	}

#if defined(CONFIG_USB_DWC2) && defined(CONFIG_USB_CHARGER_SOFTWARE_JUDGE) && defined(CONFIG_REGULATOR)
	{
		struct regulator *usb_charger = NULL;
		struct act8600_notify_st *notify_st = NULL;
		notify_st = (struct act8600_notify_st *)kzalloc(sizeof(struct act8600_notify_st),GFP_KERNEL);
		if (notify_st) {
			usb_charger = regulator_get(NULL, "ucharger");
			if (IS_ERR(usb_charger)) {
				dev_warn(&pdev->dev,"ucharger regulator get error,"
						"usb charger can not be use\n");
				kfree(notify_st);
			} else {
				charger->notify_st = notify_st;
				notify_st->usb_charger = usb_charger;
				notify_st->client = iodev->client;
				notify_st->mode_notify.notifier_call = dwc2_state_notifier;
				ret = dwc2_register_state_notifier(&notify_st->mode_notify);
				if (ret)
					dev_warn(&pdev->dev,"ucharger notifier_block regist error\n");
			}
		}
	}
#endif


	INIT_DELAYED_WORK(&charger->work, act8600_charger_work);

	if (pdata->charger_board_info->gpio != -1 && gpio_request_one(pdata->charger_board_info->gpio,
				GPIOF_DIR_OUT, "charger-current-set")) {
		dev_err(&pdev->dev, "no detect pin available\n");
		pdata->charger_board_info->gpio = -EBUSY;
		ret = ENODEV;
		goto err_free;
	}

	set_max_charger_current(pdata->charger_board_info);

	if (gpio_request_one(pdata->gpio,
			     GPIOF_DIR_IN, "charger-detect")) {
		dev_err(&pdev->dev, "no detect pin available\n");
		pdata->gpio = -EBUSY;
		ret = ENODEV;
		goto err_free;
	}

	charger->irq = gpio_to_irq(pdata->gpio);
	if (charger->irq < 0) {
		ret = charger->irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto err_free_gpio;
	}

	ret = request_irq(charger->irq, act8600_charger_irq,
			  IRQF_TRIGGER_LOW | IRQF_DISABLED,
			  "charger-detect",
			  charger);

	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %d\n", ret);
		goto err_free_irq;
	}
	enable_irq_wake(charger->irq);
	disable_irq(charger->irq);

	charger->dev = &pdev->dev;
	charger->iodev = iodev;

	charger_init(charger);
	power_supply_init(charger);

	act8600_callback_init(charger);

	platform_set_drvdata(pdev, charger);

	return 0;

err_free_irq:
	free_irq(charger->irq, charger);
err_free_gpio:
	gpio_free(pdata->gpio);
err_free:
	kfree(charger);

	return ret;
}

static int __devexit act8600_charger_remove(struct platform_device *pdev)
{
	struct act8600_charger *charger = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&charger->work);

	power_supply_unregister(&charger->usb);
	power_supply_unregister(&charger->ac);

	free_irq(charger->irq, charger);

#if defined(CONFIG_USB_DWC2) && defined(CONFIG_USB_CHARGER_SOFTWARE_JUDGE) && defined(CONFIG_REGULATOR)
	if (charger->notify_st) {
		if (charger->notify_st->usb_charger) {
			dwc2_unregister_state_notifier(&charger->notify_st->mode_notify);
			regulator_put(charger->notify_st->usb_charger);
			charger->notify_st->usb_charger = NULL;
		}
		kfree(charger->notify_st);
	}
#endif

	kfree(charger);

	return 0;
}

#ifdef CONFIG_PM
static int act8600_charger_suspend(struct device *dev)
{
	return 0;
}

static int act8600_charger_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops act8600_charger_pm_ops = {
	.suspend	= act8600_charger_suspend,
	.resume		= act8600_charger_resume,
};

#define ACT8600_CHARGER_PM_OPS (&act8600_charger_pm_ops)
#else
#define ACT8600_CHARGER_PM_OPS NULL
#endif

static struct platform_driver act8600_charger_driver = {
	.probe		= act8600_charger_probe,
	.remove		= __devexit_p(act8600_charger_remove),
	.driver = {
		.name = "act8600-charger",
		.owner = THIS_MODULE,
		.pm = ACT8600_CHARGER_PM_OPS,
	},
};

static int __init act8600_charger_init(void)
{
	return platform_driver_register(&act8600_charger_driver);
}
module_init(act8600_charger_init);

static void __exit act8600_charger_exit(void)
{
	platform_driver_unregister(&act8600_charger_driver);
}
module_exit(act8600_charger_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Large Dipper <ykli@ingenic.cn>");
MODULE_DESCRIPTION("act8600 charger driver for JZ battery");
