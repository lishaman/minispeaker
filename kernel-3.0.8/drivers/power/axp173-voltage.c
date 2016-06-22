#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mfd/core.h>
#include <linux/power_supply.h>
#include <linux/power/jz-current-battery.h>
#include <linux/mfd/axp173-private.h>
#include <linux/delay.h>

#ifdef CONFIG_PRODUCT_X1000_ASLMOM
struct ocv2soc ocv2soc[] = {
	{4321, 100},
	{4152,  97},
	{4096,  93},
	{4030,  88},
	{3974,  83},
	{3920,  78},
	{3868,  73},
	{3804,  67},
	{3764,  62},
	{3721,  56},
	{3679,  48},
	{3653,  41},
	{3628,  35},
	{3603,  28},
	{3576,  22},
	{3552,  17},
	{3522,  12},
	{3497,   9},
	{3461,   6},
	{3417,   3},
	{3371,   0},
};
#else
struct ocv2soc ocv2soc[] = {
	{4153, 100},
	{4083,  95},
	{4012,  90},
	{3942,  82},
	{3872,  75},
	{3836,  66},
	{3801,  58},
	{3766,  51},
	{3731,  46},
	{3696,  40},
	{3660,  17},
	{3625,  10},
	{3555,  0},
};
#endif

static unsigned int jz_current_battery_voltage(struct jz_current_battery *battery)
{
	unsigned int voltage = 0;
	int pmu_charging = 0;
	int pmu_current = 0;

	mutex_lock(&battery->lock);

	if (battery->get_pmu_voltage)
		voltage = battery->get_pmu_voltage(battery->pmu_interface);
	if (battery->get_pmu_charging)
		pmu_charging = battery->get_pmu_charging(battery->pmu_interface);
	if (battery->get_pmu_current)
		pmu_current = battery->get_pmu_current(battery->pmu_interface);
	printk("========++>pmu_charging = %d, pmu_current = %d\n", pmu_charging,
	       pmu_current);
	voltage = pmu_charging == CHARGING_ON ? voltage -               \
			(pmu_current * get_battery_info(battery, inter_resist) \
			 /1000) : voltage + (pmu_current*get_battery_info(battery, inter_resist)/1000);
	battery->battery_ocv = voltage;

	mutex_unlock(&battery->lock);

	return voltage;
}

static int jz_current_battery_get_property(struct power_supply *psy,
					   enum power_supply_property psp,
					   union power_supply_propval *val)
{
	struct jz_current_battery *battery = psy_to_jz_current_battery(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = jz_current_battery_voltage(battery);
		if (val->intval <= get_battery_info(battery, min_vol)/2)
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = get_battery_info(battery, max_vol);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = get_battery_info(battery, min_vol);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void jz_current_battery_external_power_changed(struct power_supply *psy)
{
	/*
	  FIXME: I think we may do nothing here,
	  You can do something you care.
	*/
}

static irqreturn_t jz_current_battery_irq_handler(int irq, void *data)
{
	struct jz_current_battery *battery = data;

	complete(&battery->read_completion);

	return IRQ_HANDLED;
}

static int jz_current_battery_current_cpt(unsigned int voltage)
{
	int i = 0;
	int cpt = 0;

	for (; i < ARRAY_SIZE(ocv2soc); ++i)
		if (voltage >= ocv2soc[i].vol)
			break;
	if (i == 0)
		cpt = ocv2soc[i].cpt;
	else if (i == ARRAY_SIZE(ocv2soc))
		cpt = ocv2soc[i-1].cpt;
	else {
		int cpt_step = (ocv2soc[i-1].vol - ocv2soc[i].vol) /
				(ocv2soc[i-1].cpt - ocv2soc[i].cpt);
		int vol = ocv2soc[i-1].vol - voltage;

		cpt = ocv2soc[i-1].cpt - vol / cpt_step;
	}

	return cpt;
}

static int get_battery_current_cpt(void *battery_interface)
{
	struct jz_current_battery *battery = battery_interface;
	unsigned int voltage = 0;

	mdelay(250);
	voltage = jz_current_battery_voltage(battery);
	return jz_current_battery_current_cpt(voltage);
}

static int get_battery_max_cpt(void *battery_interface)
{
	struct jz_current_battery *battery = battery_interface;

	return get_battery_info(battery, battery_max_cpt);
}

static int get_battery_ac_chg_current(void *battery_interface)
{
	struct jz_current_battery *battery = battery_interface;

	return get_battery_info(battery, ac_chg_current);
}

static int get_battery_usb_chg_current(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, usb_chg_current);
}

static int get_battery_suspend_current(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, suspend_current);
}

static int get_battery_sample_count(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, sample_count);
}
#ifdef CONFIG_AXP173_BAT_TEMP_DET
static int get_battery_low_temp_chg(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, low_temp_chg);
}

static int get_battery_high_temp_chg(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, high_temp_chg);
}

static int get_battery_low_temp_dischg(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, low_temp_dischg);
}

static int get_battery_high_temp_dischg(void *battery_interfusbe)
{
	struct jz_current_battery *battery = battery_interfusbe;

	return get_battery_info(battery, high_temp_dischg);
}
#endif
static void jz_current_battery_callback(struct jz_current_battery *battery)
{
	battery->battery_interface = battery;
	battery->get_battery_max_cpt = get_battery_max_cpt;
	battery->get_battery_current_cpt = get_battery_current_cpt;
	battery->get_battery_ac_chg_current = get_battery_ac_chg_current;
	battery->get_battery_usb_chg_current = get_battery_usb_chg_current;
	battery->get_battery_sample_count = get_battery_sample_count;
	battery->get_battery_suspend_current = get_battery_suspend_current;
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	battery->get_battery_low_temp_chg = get_battery_low_temp_chg;
	battery->get_battery_high_temp_chg = get_battery_high_temp_chg;
	battery->get_battery_low_temp_dischg = get_battery_low_temp_dischg;
	battery->get_battery_high_temp_dischg = get_battery_high_temp_dischg;
#endif
}

#ifdef CONFIG_PM
static int jz_current_battery_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	  FIXME: I think we may do nothing here,
	  You can do something you care.
	*/
	return 0;
}

static int jz_current_battery_resume(struct platform_device *pdev)
{
	/*
	  FIXME: I think we may do nothing here,
	  You can do something you care.
	*/
	return 0;
}
#endif

static enum power_supply_property jz_current_battery_properties[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};

static int __devinit jz_current_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_current_battery *battery = NULL;
	struct power_supply *battery_adc = NULL;
	struct jz_current_battery_info *info = NULL;

	battery = kzalloc(sizeof(*battery), GFP_KERNEL);
	if (!battery) {
		dev_err(&pdev->dev, "Failed to alloc driver structre\n");
		return -ENOMEM;
	}

	battery->cell = mfd_get_cell(pdev);
	info = battery->cell->platform_data;
	if (!info) {
		dev_err(&pdev->dev, "No cell platform data\n");
		ret = -EINVAL;
		goto battery_info_err;
	}
	battery->battery_info = info;
	if (!get_battery_info(battery, sample_count))
		get_battery_info(battery, sample_count) = 1;

        battery_adc = &battery->battery_adc;
        battery_adc->name = "battery-adc";
        battery_adc->type = POWER_SUPPLY_TYPE_BATTERY;
        battery_adc->properties = jz_current_battery_properties;
        battery_adc->num_properties = ARRAY_SIZE(jz_current_battery_properties);
        battery_adc->get_property = jz_current_battery_get_property;
        battery_adc->external_power_changed = jz_current_battery_external_power_changed;

        battery->pdev = pdev;

	mutex_init(&battery->lock);
	platform_set_drvdata(pdev, battery);
	ret = power_supply_register(&pdev->dev, &battery->battery_adc);
	if (ret) {
		dev_err(&pdev->dev, "Power supply battery-adc register failed, ret = %d\n", ret);
		goto battery_info_err;
	}

	jz_current_battery_callback(battery);

	return 0;
battery_info_err:
	kfree(battery);

	return ret;
}

static int __devexit jz_current_battery_remove(struct platform_device *pdev)
{
	struct jz_current_battery *battery = platform_get_drvdata(pdev);

	power_supply_unregister(&battery->battery_adc);
	kfree(battery);

	return 0;
}

static struct platform_driver jz_current_battery_driver = {
	.probe = jz_current_battery_probe,
	.remove = __devexit_p(jz_current_battery_remove),
	.driver = {
		.name = "jz-current-battery",
		.owner = THIS_MODULE,
	},
#ifdef CONFIG_PM
	.suspend = jz_current_battery_suspend,
	.resume	= jz_current_battery_resume,
#endif
};

static int __init jz_current_battery_init(void)
{
	return platform_driver_register(&jz_current_battery_driver);
}
module_init(jz_current_battery_init);

static void __exit jz_current_battery_exit(void)
{
	platform_driver_unregister(&jz_current_battery_driver);
}
module_exit(jz_current_battery_exit);

MODULE_ALIAS("platform:jz-current-battery");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mingli Feng<mingli.feng@ingenic.com>");
MODULE_DESCRIPTION("JZ SoC current method battery driver");
