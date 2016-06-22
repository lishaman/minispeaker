/* drivers/power/jz4780-battery.c
 *
 * Battery measurement code for Ingenic JZ SoC
 *
 * Copyright(C)2012 Ingenic Semiconductor Co., LTD.
 *	http://www.ingenic.cn
 *	Sun Jiwei <jwsun@ingenic.cn>
 * Based on JZ4740-battery.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/proc_fs.h>
#include <linux/mfd/core.h>
#include <linux/power_supply.h>

#include <linux/power/jz4780-battery.h>
#include <linux/mfd/act8600-private.h>
#include <linux/android_alarm.h>
#include <linux/earlysuspend.h>

#define RD_ADJ		15
#define RD_STROBE	7
#define	ADDRESS		0x10
#define LENGTH		0x3
#define	RD_EN		0x01
#define RD_DOWN		0x1

#define REG_EFUCTRL	0xb34100d0
#define REG_EFUCFG	0xb34100d4
#define REG_EFUSTATE	0xb34100d8
#define REG_EFUDATA0	0xb34100dc

#define	REG_READ(addr)		((unsigned int)(*(volatile unsigned int *)(addr)))
#define REG_WRITE(value, addr)	(*(volatile unsigned int *)(addr) = (value))

#define VOL_DIV 8

static struct alarm alarm;
static long	slop;
static long	cut;

static void get_slop_cut(void)
{
	spinlock_t	lock;
	unsigned long	flags;
	unsigned int	tmp;
	char		slop_r;
	char		cut_r;

	spin_lock_init(&lock);

	spin_lock_irqsave(&lock, flags);
	tmp = REG_READ(REG_EFUCFG);
	tmp &= ~(0xf << 20 | 0xf << 16);
	tmp |= (RD_ADJ << 20) | (RD_STROBE << 16);
	REG_WRITE(tmp, REG_EFUCFG);
	spin_unlock_irqrestore(&lock, flags);

	tmp = REG_READ(REG_EFUCFG);
	tmp = (tmp >> 16) & 0xf;
	if (tmp == 0xf) {
		spin_lock_irqsave(&lock, flags);
		tmp = REG_READ(REG_EFUCFG);
		tmp &= ~(0xf << 20 | 0xf << 16);
		tmp |= (RD_ADJ << 20) | (0xf << 16);
		REG_WRITE(tmp, REG_EFUCFG);
		spin_unlock_irqrestore(&lock, flags);
	}

	spin_lock_irqsave(&lock, flags);
	tmp = REG_READ(REG_EFUCTRL);
	tmp &= ~(0x1ff << 21 | 0x1f << 16 | 0x1 << 0);
	tmp |= ((ADDRESS << 21) | (LENGTH << 16) | (RD_EN << 0));
	REG_WRITE(tmp, REG_EFUCTRL);
	spin_unlock_irqrestore(&lock, flags);

	do {
		tmp = REG_READ(REG_EFUSTATE);
	} while (!(tmp & RD_DOWN));

	tmp = REG_READ(REG_EFUDATA0);

	slop_r = (tmp >> 24) & 0xff;
	cut_r = (tmp  >> 16) & 0xff;

	slop = 2 * slop_r + 2895;
	cut = 1000 * cut_r  + 90000;
}

static inline struct jz_battery *psy_to_jz_battery(struct power_supply *psy)
{
	return container_of(psy, struct jz_battery, battery);
}

static irqreturn_t jz_battery_irq_handler(int irq, void *devid)
{
	struct jz_battery *jz_battery = devid;

	complete(&jz_battery->read_completion);
	return IRQ_HANDLED;
}

static unsigned int jz_battery_read_value(struct jz_battery *jz_battery)
{
	unsigned long tmp;
	unsigned int value;

	mutex_lock(&jz_battery->lock);

	INIT_COMPLETION(jz_battery->read_completion);

	enable_irq(jz_battery->irq);
	jz_battery->cell->enable(jz_battery->pdev);

	tmp = wait_for_completion_interruptible_timeout(
			&jz_battery->read_completion, HZ);
	if (tmp > 0) {
		value = readw(jz_battery->base) & 0xfff;
	} else {
		value = tmp ? tmp : -ETIMEDOUT;
	}

	jz_battery->cell->disable(jz_battery->pdev);
	disable_irq(jz_battery->irq);

	mutex_unlock(&jz_battery->lock);

	return value;
}

static int jz_battery_adjust_voltage(struct jz_battery *battery)
{
	unsigned int current_value[12], final_value = 0;
	unsigned int value_sum = 0;
	unsigned int max_value, min_value;
	unsigned int i,j = 0, temp;
	unsigned int max_index = 0, min_index = 0;
	unsigned int real_voltage = 0;

	current_value[0] = jz_battery_read_value(battery);

	value_sum = max_value = min_value = current_value[0];

	for (i = 1; i < 12; i++) {
		current_value[i] = jz_battery_read_value(battery);
		value_sum += current_value[i];
		if (max_value < current_value[i]) {
			max_value = current_value[i];
			max_index = i;
		}
		if (min_value > current_value[i]) {
			min_value = current_value[i];
			min_index = i;
		}
	}

	value_sum -= (max_value + min_value);
	final_value = value_sum / 10;

	for (i = 0; i < 12; i++) {
		if (i == min_index)
			continue;
		if (i == max_index)
			continue;

		temp = abs(current_value[i] - final_value);
		if (temp > 4) {
			j++;
			value_sum -= current_value[i];
		}
	}

	if (j < 10) {
		final_value = value_sum / (10 - j);
	}

	if ((slop == 2895) && (cut == 90000)) {
		real_voltage = final_value * 1200 / 4096;
		real_voltage *= 4;
		pr_debug("Battery driver:The voltage is %dmV\n",real_voltage);
	} else {
		real_voltage = (final_value * slop + cut) / 10000;
		real_voltage *= 4;
		pr_debug("Battery driver:The adjust voltage is %dmV\n",real_voltage);
	}

	return real_voltage;
}

#define VOLTAGE_BOUNDARY 85

static int jz_battery_calculate_capacity(struct jz_battery *jz_battery)
{
	unsigned int tmp_min, tmp_max;
	unsigned int capacity_div_vol;

	if (jz_battery->ac) {
		tmp_min = jz_battery->pdata->info.ac_min_vol;
		tmp_max = jz_battery->pdata->info.ac_max_vol;
		capacity_div_vol = (tmp_max - tmp_min) / VOLTAGE_BOUNDARY;

		if (jz_battery->voltage <= tmp_min + capacity_div_vol)
			return 0;
		if (jz_battery->voltage > tmp_max - 3 * capacity_div_vol) {
			if (jz_battery->status_charge == POWER_SUPPLY_STATUS_FULL)
				return 100;
			else {
				return 93;
			}
		}
		return (jz_battery->voltage - tmp_min) * VOLTAGE_BOUNDARY / \
			(tmp_max - tmp_min);
	}

	if (jz_battery->usb) {
		tmp_max = jz_battery->pdata->info.usb_max_vol;
		tmp_min = jz_battery->pdata->info.usb_min_vol;
		capacity_div_vol = (tmp_max - tmp_min) / VOLTAGE_BOUNDARY;

		if (jz_battery->voltage < tmp_min + capacity_div_vol)
			return 0;
		if (jz_battery->voltage > tmp_max - 3 * capacity_div_vol) {
			if (jz_battery->status_charge == POWER_SUPPLY_STATUS_FULL)
				return 100;
			else {
				return 93;
			}
		}

		return (jz_battery->voltage - tmp_min) * VOLTAGE_BOUNDARY / \
			(tmp_max - tmp_min);
	}

	tmp_max = jz_battery->pdata->info.max_vol;
	tmp_min = jz_battery->pdata->info.min_vol;

	capacity_div_vol = (tmp_max - tmp_min) / 100;
	if (jz_battery->voltage >= tmp_max - 3 * capacity_div_vol)
		return 100;
	if (jz_battery->voltage <= tmp_min + capacity_div_vol)
		return 0;

	return (jz_battery->voltage - tmp_min) * 100 / (tmp_max - tmp_min);
}


#define MAX_CT 120
#define MIN_CT 8

static void jz_battery_capacity_falling(struct jz_battery *jz_battery)
{
	if (jz_battery->voltage < jz_battery->pdata->info.min_vol + \
			jz_battery->gate_voltage) {
		jz_battery->next_scan_time = 15;
	} else {
		jz_battery->next_scan_time = MIN_CT + jz_battery->capacity * \
			     (MAX_CT - MIN_CT) / 100;
	}

	if (jz_battery->capacity - jz_battery->capacity_calculate > 20) {
		jz_battery->next_scan_time /= 5;
	} else if (jz_battery->capacity - jz_battery->capacity_calculate > 15) {
		jz_battery->next_scan_time /= 4;
	} else if (jz_battery->capacity - jz_battery->capacity_calculate > 10) {
		jz_battery->next_scan_time /= 3;
	} else if (jz_battery->capacity - jz_battery->capacity_calculate > 5) {
		jz_battery->next_scan_time /= 2;
	}

	if (jz_battery->next_scan_time < 15)
		jz_battery->next_scan_time = 15;

	if (jz_battery->capacity_calculate < jz_battery->capacity) {
		jz_battery->capacity--;
	}

	if (jz_battery->capacity < 0)
		jz_battery->capacity = 0;
}

static void jz_battery_capacity_full(struct jz_battery *jz_battery)
{
	if (jz_battery->capacity >= 99) {
		jz_battery->capacity = 100;
		jz_battery->status_charge = POWER_SUPPLY_STATUS_FULL;
		jz_battery->next_scan_time = 5 * 60;
	} else {
		jz_battery->next_scan_time = 60;
		jz_battery->capacity++;
	}
}

static void jz_battery_calculate_for_ac(struct jz_battery *jz_battery)
{
	unsigned int capacity_div_vol;
	unsigned int tmp_min;
	unsigned int tmp_max;

	tmp_min = jz_battery->pdata->info.ac_min_vol;
	tmp_max = jz_battery->pdata->info.ac_max_vol;

	capacity_div_vol = (tmp_max - tmp_min) / VOLTAGE_BOUNDARY;

	if (jz_battery->voltage > tmp_max - 3 * capacity_div_vol) {
		if (jz_battery->capacity_calculate != jz_battery->capacity) {
			jz_battery->capacity++;
		} else if (jz_battery->capacity == 93)
			jz_battery->capacity++;
	} else if (jz_battery->capacity_calculate > jz_battery->capacity) {
		jz_battery->capacity++;
	}
}

static void jz_battery_calculate_for_usb(struct jz_battery *jz_battery)
{
	unsigned int capacity_div_vol;
	unsigned int tmp_min;
	unsigned int tmp_max;

	tmp_min = jz_battery->pdata->info.usb_min_vol;
	tmp_max = jz_battery->pdata->info.usb_max_vol;

	capacity_div_vol = (tmp_max - tmp_min) / VOLTAGE_BOUNDARY;

	if (jz_battery->voltage > tmp_max - 3 * capacity_div_vol) {
		if (jz_battery->capacity != jz_battery->capacity_calculate) {
			jz_battery->capacity++;
		} else if (jz_battery->capacity == 93)
			jz_battery->capacity++;
	} else if (jz_battery->capacity_calculate > jz_battery->capacity) {
		jz_battery->capacity++;
	}
}

static void jz_battery_capacity_rising(struct jz_battery *jz_battery)
{
	if (jz_battery->capacity >= 100) {
		jz_battery->next_scan_time = 60;
		jz_battery->capacity = 99;
		return;
	}

	if (jz_battery->capacity == 99) {
		jz_battery->next_scan_time = 60;
		return ;
	}

	if (jz_battery->ac == 1) {
		if (jz_battery->voltage >=
				jz_battery->pdata->info.ac_max_vol - VOL_DIV) {
			jz_battery->next_scan_time = jz_battery->ac_charge_time << 1;
		} else {
			jz_battery->next_scan_time = jz_battery->ac_charge_time;
		}
	} else if (jz_battery->usb == 1) {
		if (jz_battery->voltage >=
				jz_battery->pdata->info.usb_max_vol - VOL_DIV) {
			jz_battery->next_scan_time = jz_battery->usb_charge_time << 1;
		} else {
			jz_battery->next_scan_time = jz_battery->usb_charge_time;
		}
	}

	if (jz_battery->capacity_calculate - jz_battery->capacity > 15) {
		jz_battery->next_scan_time /= 4;
	} else if (jz_battery->capacity_calculate - jz_battery->capacity > 10) {
		jz_battery->next_scan_time /= 3;
	} else if (jz_battery->capacity_calculate - jz_battery->capacity > 5) {
		jz_battery->next_scan_time /= 2;
	}

	if (jz_battery->next_scan_time < 40)
		jz_battery->next_scan_time = 40;

	if (1 == jz_battery->usb && 0 == jz_battery->ac) {
		jz_battery_calculate_for_usb(jz_battery);
	} else if (1 == jz_battery->ac && 0 == jz_battery->usb) {
		jz_battery_calculate_for_ac(jz_battery);
	} else if(jz_battery->ac == 1) {
		jz_battery_calculate_for_ac(jz_battery);
	}
}

static void jz_battery_get_capacity(struct jz_battery *jz_battery)
{
	switch (jz_battery->status_tmp) {
	case POWER_SUPPLY_STATUS_CHARGING:
		jz_battery_capacity_rising(jz_battery);
		break;
	case POWER_SUPPLY_STATUS_FULL:
		jz_battery_capacity_full(jz_battery);
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		jz_battery_capacity_falling(jz_battery);
		break;
	case POWER_SUPPLY_STATUS_UNKNOWN:
		jz_battery->next_scan_time = 60;
		break;
	}
}

static int jz_battery_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct jz_battery *jz_battery = psy_to_jz_battery(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = jz_battery->status_charge;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LIPO;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = jz_battery->capacity;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = jz_battery->voltage;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = jz_battery->pdata->info.max_vol;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = jz_battery->pdata->info.min_vol;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

unsigned long get_wakeup_time(struct jz_battery *jz_battery)
{
	unsigned long tmp = 60 * 60;

	if (jz_battery->pdata->info.sleep_current == 0){
		tmp = 60 * 60;
		return tmp;
	}

	if (jz_battery->capacity > 10) {
		tmp = ((jz_battery->capacity - 10) *
			jz_battery->pdata->info.battery_max_cpt * 36) /
				(jz_battery->pdata->info.sleep_current);
	} else if (jz_battery->capacity > 3) {
		tmp = ((jz_battery->capacity - 3) *
			jz_battery->pdata->info.battery_max_cpt * 36) /
				(jz_battery->pdata->info.sleep_current);
	} else if (jz_battery->capacity > 1){
		tmp = ((jz_battery->capacity - 1) *
			jz_battery->pdata->info.battery_max_cpt * 36) /
			(jz_battery->pdata->info.sleep_current);
	} else if (jz_battery->capacity == 1){
		tmp = (jz_battery->pdata->info.battery_max_cpt * 36) /
			jz_battery->pdata->info.sleep_current;
	}

	if (tmp == 0)
		tmp = 30 * 60;

	return tmp;
}

static void jz_battery_set_resume_time(struct jz_battery *jz_battery)
{
	struct timespec alarm_time;
	unsigned long	 interval;

	getnstimeofday(&alarm_time);

	interval = get_wakeup_time(jz_battery);

	alarm_time.tv_sec += interval;
	alarm_start_range(&alarm, timespec_to_ktime(alarm_time),
			timespec_to_ktime(alarm_time));
}

static void jz_battery_external_power_changed(struct power_supply *psy)
{
	struct jz_battery *jz_battery = psy_to_jz_battery(psy);

	cancel_delayed_work(&jz_battery->work);
	schedule_delayed_work(&jz_battery->work, 0);
}

static void jz_battery_update_work(struct jz_battery *jz_battery)
{
	bool has_changed = false;
	unsigned int voltage;
	unsigned int usb;
	unsigned int ac;

	if (jz_battery->get_pmu_status == NULL) {
		ac = get_charger_online(jz_battery, AC);
		usb = get_charger_online(jz_battery, USB);
	} else {
		usb = jz_battery->get_pmu_status(jz_battery->pmu_interface, USB);
		ac = jz_battery->get_pmu_status(jz_battery->pmu_interface, AC);
	}

	if ((jz_battery->capacity >= 100) &&
		(jz_battery->status == POWER_SUPPLY_STATUS_CHARGING)) {
		jz_battery->status = POWER_SUPPLY_STATUS_FULL;
		jz_battery->capacity = 100;
	}

	if ((jz_battery->capacity == 99) &&
		(jz_battery->status == POWER_SUPPLY_STATUS_CHARGING)) {
		if (jz_battery->time < 10) {
			jz_battery->time++;
		} else {
			jz_battery->status = POWER_SUPPLY_STATUS_FULL;
			jz_battery->time = 1;
		}
	}
	jz_battery->status_tmp = jz_battery->status;

	voltage = jz_battery_adjust_voltage(jz_battery);

	if ((jz_battery->usb == 1) && (usb == 1)) {
		if (jz_battery->get_pmu_status != NULL)
			jz_battery->status_tmp =
				jz_battery->get_pmu_status(jz_battery->pmu_interface, STATUS);
	}

	if ((jz_battery->ac == 1) && (ac == 1)) {
		if (jz_battery->get_pmu_status != NULL)
			jz_battery->status_tmp =
				jz_battery->get_pmu_status(jz_battery->pmu_interface, STATUS);
	}

	if (jz_battery->status_tmp == POWER_SUPPLY_STATUS_FULL) {
		has_changed = true;
	}

	if (abs(voltage - jz_battery->voltage) > VOL_DIV) {
		jz_battery->voltage = voltage;
		has_changed = true;
	}

	jz_battery->capacity_calculate = jz_battery_calculate_capacity(jz_battery);

	if (jz_battery->capacity_calculate != jz_battery->capacity) {
		has_changed = true;
	}

	if (jz_battery->status_tmp == POWER_SUPPLY_STATUS_CHARGING) {
		if (ac && (voltage > (jz_battery->pdata->info.ac_max_vol - \
					VOL_DIV))) {
			has_changed = true;
		}
		if (usb && (voltage > (jz_battery->pdata->info.usb_max_vol - \
					VOL_DIV))) {
			has_changed = true;
		}
		if (jz_battery->capacity_calculate == 93) {
			has_changed =true;
		}
	}

	if (jz_battery->status_charge != jz_battery->status_tmp) {
		has_changed = true;
		jz_battery->status_charge = jz_battery->status_tmp;
		jz_battery->capacity_calculate = jz_battery->capacity;
		jz_battery->next_scan_time = 60;
	}

	if (((jz_battery->ac == 1) && (ac == 0)) ||
			((jz_battery->ac == 0) && (ac == 1))) {
		has_changed = true;
		jz_battery->capacity_calculate = jz_battery->capacity;
		jz_battery->next_scan_time = 60;
	}

	if (((jz_battery->usb == 1) && (usb == 0)) ||
			((jz_battery->usb == 0) && (usb == 1))) {
		has_changed = true;
		jz_battery->capacity_calculate = jz_battery->capacity;
		jz_battery->next_scan_time = 60;
	}

	if ((jz_battery->status_charge == POWER_SUPPLY_STATUS_FULL) && \
			(jz_battery->capacity != 100)) {
		has_changed = true;
		jz_battery->status_charge = POWER_SUPPLY_STATUS_CHARGING;
	}

	jz_battery->ac = ac;
	jz_battery->usb = usb;

	pr_debug("Battery driver: Now battery status is %d, status_tmp is %d\n", jz_battery->status,
			jz_battery->status_tmp);
	pr_debug("Battery driver: Now display status is %d\n", jz_battery->status_charge);
	pr_debug("Battery driver: Now battery capacity calculate is %d\n", jz_battery->capacity_calculate);

	if (has_changed) {
		unsigned int tmp = jz_battery->capacity;
		pr_debug("Battery driver: Now battery voltage is %dmV\n", jz_battery->voltage);
		jz_battery_get_capacity(jz_battery);
		if (tmp != jz_battery->capacity)
			pr_debug("Battery driver: Capacity is %d\n", jz_battery->capacity);

		power_supply_changed(&jz_battery->battery);

		if (jz_battery->power_on_flag) {
			jz_battery_set_resume_time(jz_battery);
		}
	}
}

static void jz_battery_work(struct work_struct *work)
{
	struct jz_battery *jz_battery = container_of(work, struct jz_battery,
			work.work);

	jz_battery_update_work(jz_battery);

	pr_debug("Battery driver: Next check time is %ds\n", jz_battery->next_scan_time);
	schedule_delayed_work(&jz_battery->work, jz_battery->next_scan_time * HZ);
}

static void wake_up_fun(struct alarm *alarm)
{
	;
}

static void jz_battery_init_work(struct work_struct *work)
{
	struct delayed_work *delayed_work = container_of(work, \
			struct delayed_work, work);
	struct jz_battery *jz_battery = container_of(delayed_work, \
			struct jz_battery, init_work);

        jz_battery->usb = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, USB);
	jz_battery->ac = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, AC);
	jz_battery->status_charge = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, STATUS);
	jz_battery->voltage = jz_battery_adjust_voltage(jz_battery);
	jz_battery->capacity = jz_battery_calculate_capacity(jz_battery);

	power_supply_changed(&jz_battery->battery);

	cancel_delayed_work_sync(&jz_battery->work);
	schedule_delayed_work(&jz_battery->work, 20 * HZ);

	alarm_init(&alarm, ANDROID_ALARM_RTC_WAKEUP, wake_up_fun);
	jz_battery->power_on_flag = 1;

	jz_battery->pmu_work_enable(jz_battery->pmu_interface);
}

static void jz_battery_resume_capacity_for_ac(struct jz_battery *jz_battery)
{
	int time_tmp;
	if (jz_battery->private.voltage >=
			(jz_battery->pdata->info.ac_max_vol - VOL_DIV)) {
		jz_battery->capacity += jz_battery->private.timecount /	\
					(jz_battery->ac_charge_time << 1);
	} else if (jz_battery->voltage <=
			jz_battery->pdata->info.ac_max_vol - VOL_DIV) {
		if (jz_battery->private.timecount > 3 * 60) {
			if (jz_battery->private.voltage < jz_battery->voltage) {
				jz_battery->capacity = jz_battery->capacity_calculate;
			} else if (jz_battery->private.timecount > 50 * 60)
				printk("Battery driver: error in calculate capacity before sleep\n");
		} else {
			pr_debug("Battery driver: the sleep time < 3m, the capcacity don't change\n");
		}
	} else {
		time_tmp = jz_battery->private.timecount -		\
			((VOLTAGE_BOUNDARY - jz_battery->capacity) *	\
			jz_battery->ac_charge_time);
		if (time_tmp > 0) {
			jz_battery->capacity = VOLTAGE_BOUNDARY +	\
				(time_tmp / (jz_battery->ac_charge_time << 1));
		} else {
			jz_battery->capacity = VOLTAGE_BOUNDARY;
			pr_info("Battery driver: error in resume\n");
		}
	}

}

static void jz_battery_resume_capacity_for_usb(struct jz_battery *jz_battery)
{
	int time_tmp;

	if (jz_battery->private.voltage >=	\
			(jz_battery->pdata->info.usb_max_vol - VOL_DIV)) {
		jz_battery->capacity += jz_battery->private.timecount /	\
					(jz_battery->usb_charge_time << 1);
	} else if (jz_battery->voltage <=
			jz_battery->pdata->info.usb_max_vol - VOL_DIV) {
		if (jz_battery->private.timecount > 3 * 60) {
			if (jz_battery->private.voltage < jz_battery->voltage) {
				jz_battery->capacity = jz_battery->capacity_calculate;
			} else if (jz_battery->private.timecount > 50 * 60)
				printk("Battery driver: error in calculate capacity before sleep\n");
		} else {
			pr_debug("Battery driver: the sleep time < 3m, the capcacity don't change\n");
		}
	} else {
		time_tmp = jz_battery->private.timecount -		\
			((VOLTAGE_BOUNDARY - jz_battery->capacity) *	\
			jz_battery->usb_charge_time);
		if (time_tmp > 0) {
			jz_battery->capacity = VOLTAGE_BOUNDARY *
				(time_tmp / (jz_battery->usb_charge_time << 1));
		} else {
			jz_battery->capacity = VOLTAGE_BOUNDARY;
			pr_info("Battery driver: error in resume\n");
		}
	}
}

static void jz_battery_resume_capacity_for_charge(struct jz_battery *jz_battery)
{
	if (jz_battery->ac)
		jz_battery_resume_capacity_for_ac(jz_battery);
	else if (jz_battery->usb)
		jz_battery_resume_capacity_for_usb(jz_battery);
	else {
		pr_info("Battery driver: error in resume\n");
		pr_info("Battery driver: cannot get the online status\n");
	}

	if (jz_battery->capacity > 100) {
		jz_battery->capacity = 100;
		jz_battery->status_charge = POWER_SUPPLY_STATUS_FULL;
	}
}

static void jz_battery_resume_capacity_for_discharge(struct jz_battery *jz_battery)
{
	if (jz_battery->private.timecount > 3 * 60) {
		jz_battery->capacity = jz_battery->capacity_calculate;
	}
}

static void jz_battery_resume_charge_capacity(struct jz_battery *jz_battery)
{
	switch (jz_battery->status_charge) {
	case POWER_SUPPLY_STATUS_CHARGING:
		jz_battery_resume_capacity_for_charge(jz_battery);
		return;
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		jz_battery_resume_capacity_for_discharge(jz_battery);
		return;
	default:
		jz_battery->capacity = jz_battery->capacity_calculate;
		pr_info("Battery driver: error in resume\n");
		pr_info("Battery driver: the status is unknown\n");
	}

}

static void jz_battery_resume_work(struct work_struct *work)
{
	struct delayed_work *delayed_work = container_of(work, \
			struct delayed_work, work);
	struct jz_battery *jz_battery = container_of(delayed_work,
			struct jz_battery, resume_work);
	struct timeval battery_time;
	int status;

	jz_battery->private.voltage = jz_battery->voltage;
	jz_battery->voltage = jz_battery_adjust_voltage(jz_battery);
	status = jz_battery->status_charge;
	jz_battery->status_charge = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, STATUS);
	jz_battery->ac = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, AC);
	jz_battery->usb = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, USB);
	jz_battery->capacity_calculate =
		jz_battery_calculate_capacity(jz_battery);
	pr_info("Battery driver: resume capacity calculate is %d\n", jz_battery->capacity_calculate);

	do_gettimeofday(&battery_time);
	jz_battery->resume_time = battery_time.tv_sec;
	pr_info("Battery driver: resume_time is %ld\n", battery_time.tv_sec);
	jz_battery->private.timecount = jz_battery->resume_time - \
		jz_battery->suspend_time;
	pr_info("Battery driver: sleep time is %ld\n", jz_battery->private.timecount);

	if ((status == POWER_SUPPLY_STATUS_DISCHARGING) ||
			(status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
		if ((jz_battery->capacity_calculate < jz_battery->capacity) \
				&& (jz_battery->private.timecount > 15 * 60)) {
			jz_battery->capacity = jz_battery->capacity_calculate;
		}
	} else if (status == POWER_SUPPLY_STATUS_FULL ||	\
		jz_battery->status_charge == POWER_SUPPLY_STATUS_FULL) {
		jz_battery->capacity = 100;
	} else if (status == POWER_SUPPLY_STATUS_CHARGING) {
		jz_battery_resume_charge_capacity(jz_battery);
	}

	power_supply_changed(&jz_battery->battery);

	pr_info("Battery driver: resume charger status is %d\n", jz_battery->status_charge);
	pr_info("Battery driver: resume ac status is %d\n", jz_battery->ac);
	pr_info("Battery driver: resume USB status is %d\n", jz_battery->usb);
	pr_info("Battery driver: resume capacity is %d\n", jz_battery->capacity);
	jz_battery->next_scan_time = 15;
	schedule_delayed_work(&jz_battery->work, jz_battery->next_scan_time * HZ);
	jz_battery_set_resume_time(jz_battery);
	wake_unlock(&jz_battery->work_wake_lock);
}

#ifdef CONFIG_PM
static int jz_battery_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct jz_battery *jz_battery = platform_get_drvdata(pdev);
	struct timeval battery_time;

	if (jz_battery->get_pmu_status != NULL) {
		jz_battery->status_charge = jz_battery->get_pmu_status(
				jz_battery->pmu_interface, STATUS);
		jz_battery->ac = jz_battery->get_pmu_status(
				jz_battery->pmu_interface, AC);
		jz_battery->usb = jz_battery->get_pmu_status(
				jz_battery->pmu_interface, USB);
	} else {
		jz_battery->status_charge = jz_battery->status;
		jz_battery->ac = get_charger_online(jz_battery, AC);
		jz_battery->usb = get_charger_online(jz_battery, USB);
	}

	pr_info("Battery driver: Before sleep charger status is %d\n", jz_battery->status_charge);
	pr_info("Battery driver: Before sleep ac status is %d\n", jz_battery->ac);
	pr_info("Battery driver: Before sleep USB status is %d\n", jz_battery->usb);
	pr_info("Battery driver: Before sleep capacity is %d\n", jz_battery->capacity);
	pr_info("Battery driver: Before sleep voltage is %d\n", jz_battery->voltage);

	cancel_delayed_work_sync(&jz_battery->resume_work);
	cancel_delayed_work_sync(&jz_battery->work);

	do_gettimeofday(&battery_time);
	jz_battery->suspend_time = battery_time.tv_sec;
	pr_info("Battery driver: suspend_time is %ld\n", battery_time.tv_sec);

	return 0;
}

static int jz_battery_resume(struct platform_device *pdev)
{
	struct jz_battery *jz_battery = platform_get_drvdata(pdev);

	cancel_delayed_work(&jz_battery->resume_work);
	wake_lock(&jz_battery->work_wake_lock);
	schedule_delayed_work(&jz_battery->resume_work, HZ);

	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#define WAKEUP_TIME	10

static void jz_battery_early_suspend(struct early_suspend *early_suspend)
{
	struct jz_battery *jz_battery;

	jz_battery = container_of(early_suspend, struct jz_battery, early_suspend);
	jz_battery_set_resume_time(jz_battery);
}

static void jz_battery_late_resume(struct early_suspend *early_suspend)
{
	;
}
#endif

static enum power_supply_property jz_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_PRESENT,
};

static int proc_read_voltage(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct jz_battery *jz_battery = (struct jz_battery *)data;
	unsigned int value;
	unsigned int real_voltage;

	value = jz_battery_read_value(jz_battery);

	real_voltage = value * 1200 / 4096;
	real_voltage *= 4;

	pr_info("Value	= %d\n", value);
	pr_info("Real_Voltage = %d\n", real_voltage);
	return 0;
}

static int proc_read_adjust_voltage(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct jz_battery *jz_battery = (struct jz_battery *)data;
	unsigned int adjust_voltage;

	adjust_voltage = jz_battery_adjust_voltage(jz_battery);

	pr_info("Adjust_Voltage = %d\n", adjust_voltage);
	return 0;
}

static int proc_read_capacity(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct jz_battery *jz_battery = (struct jz_battery *)data;
	unsigned int voltage;
	unsigned int tmp_min, tmp_max;
	unsigned int capacity_div_vol;
	unsigned capacity;

	voltage = jz_battery_adjust_voltage(jz_battery);

	if (jz_battery->ac) {
		tmp_min = jz_battery->pdata->info.ac_min_vol;
		tmp_max = jz_battery->pdata->info.ac_max_vol;
		capacity_div_vol = (tmp_max - tmp_min) / VOLTAGE_BOUNDARY;

		if (voltage <= tmp_min + capacity_div_vol) {
			capacity = 0;
		} else if (voltage > tmp_max - 3 * capacity_div_vol) {
			if (jz_battery->status_charge == POWER_SUPPLY_STATUS_FULL) {
				capacity = 100;
			} else {
				capacity = 93;
			}
		} else {
			capacity = (voltage - tmp_min) * VOLTAGE_BOUNDARY / \
				(tmp_max - tmp_min);
		}
	} else if (jz_battery->usb) {
		tmp_max = jz_battery->pdata->info.usb_max_vol;
		tmp_min = jz_battery->pdata->info.usb_min_vol;
		capacity_div_vol = (tmp_max - tmp_min) / VOLTAGE_BOUNDARY;

		if (voltage < tmp_min + capacity_div_vol) {
			capacity = 0;
		} else if (voltage > tmp_max - 3 * capacity_div_vol) {
			if (jz_battery->status_charge == POWER_SUPPLY_STATUS_FULL) {
				capacity = 100;
			} else {
				capacity = 93;
			}
		} else {
			capacity = (voltage - tmp_min) * VOLTAGE_BOUNDARY / \
				(tmp_max - tmp_min);
		}
	} else {
		tmp_max = jz_battery->pdata->info.max_vol;
		tmp_min = jz_battery->pdata->info.min_vol;

		capacity_div_vol = (tmp_max - tmp_min) / 100;
		if (voltage >= tmp_max - 3 * capacity_div_vol) {
			capacity = 100;
		} else if (voltage <= tmp_min + capacity_div_vol) {
			capacity = 0;
		} else {
			capacity = (voltage - tmp_min) * 100 / (tmp_max - tmp_min);
		}
	}

	pr_info("Voltage	= %d\n", voltage);
	pr_info("USB = %d, AC = %d\n",jz_battery->usb, jz_battery->ac);
	pr_info("Capacity = %d\n", capacity);
	return 0;
}

static int proc_read_status(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	unsigned int status_charge;
	unsigned int ac;
	unsigned int usb;
	struct jz_battery *jz_battery = (struct jz_battery *)data;

	status_charge = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, STATUS);
	ac = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, AC);
	usb = jz_battery->get_pmu_status(
			jz_battery->pmu_interface, USB);

	pr_info("Status = %d\n", status_charge);
	pr_info("AC = %d, USB = %d\n", ac, usb);
	return 0;
}

static int __devinit jz_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_battery *jz_battery;
	struct power_supply *battery;
	struct proc_dir_entry *root;
	struct proc_dir_entry *res;
	struct jz_battery_info *info;
	//******modify by vincent<junyang@ingenic.cn> 2013-08-19*****
	struct jz_battery_platform_data *pdata = kzalloc(sizeof(struct jz_battery_platform_data),GFP_KERNEL);
	if (!pdata) {
		dev_err(&pdev->dev, "Failed to allocate platform_data structre\n");
		return -ENOMEM;
	}
	jz_battery = kzalloc(sizeof(*jz_battery), GFP_KERNEL);
	if (!jz_battery) {
		dev_err(&pdev->dev, "Failed to allocate driver structre\n");
		return -ENOMEM;
	}

	jz_battery->cell = mfd_get_cell(pdev);
	info = jz_battery->cell->platform_data;
	if(!info)
	{
		dev_err(&pdev->dev,"no cell data,cat attach\n");
		return -EINVAL;
	}
	pdata->info = *info;
	jz_battery->irq = platform_get_irq(pdev, 0);
	if (jz_battery->irq < 0) {
		ret = jz_battery->irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto err_free;
	}

	jz_battery->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!jz_battery->mem) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get platform mmio resource\n");
		goto err_free;
	}

	jz_battery->mem = request_mem_region(jz_battery->mem->start,
			resource_size(jz_battery->mem), pdev->name);
	if (!jz_battery->mem) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		goto err_free;
	}

	jz_battery->base = ioremap_nocache(jz_battery->mem->start,
			resource_size(jz_battery->mem));
	if (!jz_battery->base) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		goto err_release_mem_region;
	}

	get_slop_cut();

	battery = &jz_battery->battery;
	battery->name = "battery";
	battery->type = POWER_SUPPLY_TYPE_BATTERY;
	battery->properties = jz_battery_properties;
	battery->num_properties = ARRAY_SIZE(jz_battery_properties);
	battery->get_property = jz_battery_get_property;
	battery->external_power_changed = jz_battery_external_power_changed;
	battery->use_for_apm = 1;

	jz_battery->pdata = pdata;
	jz_battery->pdev = pdev;

#ifdef CONFIG_HAS_EARLYSUSPEND
	jz_battery->early_suspend.suspend = jz_battery_early_suspend;
	jz_battery->early_suspend.resume = jz_battery_late_resume;
	jz_battery->early_suspend.level	= EARLY_SUSPEND_LEVEL_DISABLE_FB;
	register_early_suspend(&jz_battery->early_suspend);
#endif
	init_completion(&jz_battery->read_completion);
	mutex_init(&jz_battery->lock);

	INIT_DELAYED_WORK(&jz_battery->work, jz_battery_work);
	INIT_DELAYED_WORK(&jz_battery->init_work, jz_battery_init_work);
	INIT_DELAYED_WORK(&jz_battery->resume_work, jz_battery_resume_work);
	wake_lock_init(&jz_battery->work_wake_lock, WAKE_LOCK_SUSPEND, "jz_battery");

	ret = request_irq(jz_battery->irq, jz_battery_irq_handler, 0,
			pdev->name, jz_battery);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %d\n", ret);
		goto err_iounmap;
	}
	disable_irq(jz_battery->irq);

	ret = power_supply_register(&pdev->dev, &jz_battery->battery);
	if (ret) {
		dev_err(&pdev->dev, "Power supply battery register failed.\n");
		goto err_iounmap;
	}

	platform_set_drvdata(pdev, jz_battery);

	jz_battery->time = 1;

	jz_battery->gate_voltage = (jz_battery->pdata->info.max_vol - \
			jz_battery->pdata->info.min_vol) / 10;
	if (pdata->info.usb_chg_current) {
		jz_battery->usb_charge_time = pdata->info.battery_max_cpt /	\
				      pdata->info.usb_chg_current * 36;
	}
	if (pdata->info.ac_chg_current) {
		jz_battery->ac_charge_time = pdata->info.battery_max_cpt  /	\
				     pdata->info.ac_chg_current * 36;
	}

	jz_battery->voltage = jz_battery_adjust_voltage(jz_battery);

	jz_battery->next_scan_time = 15;
	schedule_delayed_work(&jz_battery->init_work, 5 * HZ);

	root = proc_mkdir("power_supply", 0);

	res = create_proc_entry("voltage", 0444, root);
	if (res) {
		res->read_proc = proc_read_voltage;
		res->data = jz_battery;
	}

	res = create_proc_entry("adjust_voltage", 0444, root);
	if (res) {
		res->read_proc = proc_read_adjust_voltage;
		res->data = jz_battery;
	}

	res = create_proc_entry("capacity", 0444, root);
	if (res) {
		res->read_proc = proc_read_capacity;
		res->data = jz_battery;
	}

	res = create_proc_entry("status", 0444, root);
	if (res) {
		res->read_proc = proc_read_status;
		res->data = jz_battery;
	}

	pr_info("Battery driver:max_vol is %d\n", pdata->info.max_vol);
	pr_info("Battery driver:min_vol is %d\n", pdata->info.min_vol);
	pr_info("Battery driver:usb_max_vol is %d\n", pdata->info.usb_max_vol);
	pr_info("Battery driver:usb_min_vol is %d\n", pdata->info.usb_min_vol);
	pr_info("Battery driver:ac_max_vol is %d\n", pdata->info.ac_max_vol);
	pr_info("Battery driver:ac_min_vol is %d\n", pdata->info.ac_min_vol);
	pr_info("Battery driver:battery_max_cpt is %d\n", pdata->info.battery_max_cpt);
	pr_info("Battery driver:ac_chg_current is %d\n", pdata->info.ac_chg_current);
	pr_info("Battery driver:usb_chg_current is %d\n", pdata->info.usb_chg_current);
	pr_info("Battery driver:sleep_current is %d\n", pdata->info.sleep_current);

	pr_info("Battery driver registers over!\n");

	return 0;

err_iounmap:
	wake_lock_destroy(&jz_battery->work_wake_lock);
	platform_set_drvdata(pdev, NULL);
	iounmap(jz_battery->base);
err_release_mem_region:
	release_mem_region(jz_battery->mem->start,
			resource_size(jz_battery->mem));
err_free:
	kfree(jz_battery);
	return ret;
}

static int __devexit jz_battery_remove(struct platform_device *pdev)
{
	struct jz_battery *jz_battery = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&jz_battery->work);
	cancel_delayed_work_sync(&jz_battery->resume_work);

	wake_lock_destroy(&jz_battery->work_wake_lock);

	power_supply_unregister(&jz_battery->battery);

	free_irq(jz_battery->irq, jz_battery);

	iounmap(jz_battery->base);
	release_mem_region(jz_battery->mem->start,
			resource_size(jz_battery->mem));
	kfree(jz_battery);

	return 0;
}

static struct platform_driver jz_battery_driver = {
	.probe	= jz_battery_probe,
	.remove = __devexit_p(jz_battery_remove),
	.driver = {
		.name	= "jz4780-battery",
		.owner	= THIS_MODULE,
	},
	.suspend	= jz_battery_suspend,
	.resume		= jz_battery_resume,
};

static int __init jz_battery_init(void)
{
	return platform_driver_register(&jz_battery_driver);
}
module_init(jz_battery_init);

static void __exit jz_battery_exit(void)
{
	platform_driver_unregister(&jz_battery_driver);
}
module_exit(jz_battery_exit);

MODULE_ALIAS("platform:jz4780-battery");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sun Jiwei<jwsun@ingenic.cn>");
MODULE_DESCRIPTION("JZ4780 SoC battery driver");
