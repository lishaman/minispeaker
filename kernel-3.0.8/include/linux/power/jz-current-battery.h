#ifndef __JZ_CURRENT_BATTERY_H
#define __JZ_CURRENT_BATTERY_H

#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include <linux/io.h>
#include <linux/power_supply.h>

#define CHARGING_ON 1
#define CHARGING_OFF 0

#define psy_to_jz_current_battery(psy) container_of(psy, struct jz_current_battery, battery_adc);
#define get_battery_info(battery, info) (battery->battery_info->info)

struct jz_current_battery_info {
	int max_vol;
	int min_vol;
	int inter_resist;
	int sample_count;
	int adc_sw_adjust;
	int battery_max_cpt;
	int ac_chg_current;
	int usb_chg_current;
	int suspend_current;
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	int low_temp_chg;
	int high_temp_chg;
	int low_temp_dischg;
	int high_temp_dischg;
#endif
};

struct jz_current_battery {
	struct platform_device *pdev;
	const struct mfd_cell *cell;
	struct resource *mem;
	void __iomem *base;

	int adc_slop;
	int adc_cut;

	int charging;
	struct jz_current_battery_info *battery_info;
	void *pmu_interface;
	void *battery_interface;
	int (*get_pmu_charging)(void *pmu_interface);
	int (*get_pmu_current)(void *pmu_interface);
	int (*get_pmu_voltage)(void *pmu_interface);
	int (*get_battery_max_cpt)(void *battery_interface);
	int (*get_battery_current_cpt)(void *battery_interface);
	int (*get_battery_ac_chg_current)(void *battery_interface);
	int (*get_battery_usb_chg_current)(void *battery_interface);
	int (*get_battery_sample_count)(void *battery_interface);
	int (*get_battery_suspend_current)(void *battery_interface);
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	int (*get_battery_low_temp_chg)(void *battery_interface);
	int (*get_battery_high_temp_chg)(void *battery_interface);
	int (*get_battery_low_temp_dischg)(void *battery_interface);
	int (*get_battery_high_temp_dischg)(void *battery_interface);
#endif
	int irq;
	unsigned int battery_ocv;
	unsigned int real_vol;
	struct power_supply battery_adc;
	struct completion read_completion;
	struct mutex lock;
};

struct ocv2soc {
	int vol;
	int cpt;
};

#endif

#endif
