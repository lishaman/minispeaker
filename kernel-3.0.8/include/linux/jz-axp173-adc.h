#ifndef __JZ_AXP173_ADC_H
#define __JZ_AXP173_ADC_H

#include <linux/device.h>
#include <linux/power/jz-current-battery.h>

struct axp173_adc_platform_data{
	struct jz_current_battery_info *battery_info;
};

#endif /*jz-axp173-adc.h*/
