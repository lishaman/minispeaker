#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/power/jz-current-battery.h>
#include <linux/mfd/axp173-private.h>
#include <linux/mfd/pmu-common.h>
#include <linux/earlysuspend.h>
#include <linux/pm.h>

#ifdef CONFIG_PRODUCT_X1000_ASLMOM
#define GPIO_BOOST 	GPIO_PB(5)
#endif

static int temp_key = 0;
static int audio_playing = 0;
static int volume = 0;

struct current2volume {
	int cur;
	int vol;
};

struct current2volume cur2vol[] = {
#if 0
	{600, 100},
	{575,  90},
	{545,  80},
	{385,  70},
	{265,  60},
	{200,  50},
	{188,  40},
	{188,  30},
	{185,  20},
	{185,  10},
	{170,   0},
#else
	{370, 100},
	{350,  90},
	{320,  80},
	{210,  70},
	{180,  60},
	{155,  50},
	{148,  40},
	{148,  30},
	{145,  20},
	{145,  10},
	{135,   0},
#endif
};

//#define CONFIG_AXP173_CHARGER_DEBUG

#ifdef CONFIG_AXP173_CHARGER_DEBUG
static int axp173_debug = 1;

static noinline int __attribute__((unused))
		axp173_printk(const char *comp, const char *fmt, ...)
{
	va_list args;
	int rtn;

	printk("===>CPU%d-%s: ", smp_processor_id(), comp);
	va_start(args, fmt);
	rtn = vprintk(fmt, args);
	va_end(args);

	return rtn;
}

#define AXP173_DEBUG_MSG(msg...)                        \
	do {                                            \
		if (unlikely(axp173_debug)) {           \
			axp173_printk("AXP173", msg);   \
		}                                       \
	} while (0)

static ssize_t axp173_debug_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", axp173_debug);
}

static ssize_t axp173_debug_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (strncmp(buf, "1", 1) == 0)
		axp173_debug = 1;
	else
		axp173_debug = 0;

	return count;
}

static DEVICE_ATTR(debug, S_IWUSR | S_IRUSR | S_IROTH,
		   axp173_debug_show, axp173_debug_set);
#else
#define AXP173_DEBUG_MSG(msg...)  do {  } while (0)
#endif

static struct attribute *axp173_attributes[] = {
#ifdef CONFIG_AXP173_CHARGER_DEBUG
	&dev_attr_debug.attr,
#endif
	NULL,
};

static const struct attribute_group axp173_attr_group = {
	.attrs = axp173_attributes,
};

struct axp173_charger {
	struct platform_device *pdev;
	struct axp173 *axp173;

	int base_cpt;
	int battery_status;
	int battery_online;
	int ac_online;
	int usb_online;
	int report;
	int sample_count;
	int max_cpt;
	int current_cpt;
	int ac_chg_current;
	int usb_chg_current;
	int suspend_current;
	int irq;
	int real_cpt;
	int real_voltage;
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	int low_temp_chg;
	int high_temp_chg;
	int low_temp_dischg;
	int high_temp_dischg;
#endif
	unsigned int next_scan_time;

	struct power_supply battery;
	struct power_supply usb;
	struct power_supply ac;
	struct delayed_work work;
	struct delayed_work update_work;
	struct mutex lock;
	struct early_suspend early_suspend;
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	struct jz_notifier battery_notifier_play;
	struct jz_notifier battery_notifier_stop;
#endif
};

#define SET_AXP173_CHARGER_AREA(charger, area, value)   \
	do {                                            \
		if (charger->area != value) {           \
			charger->report = 1;            \
			charger->area = value;          \
		}                                       \
	} while (0)

#define BATTERY_CHANGED_REPORT(charger)                                 \
	do {                                                            \
		if (charger->report) {                                  \
			power_supply_changed(&charger->battery);        \
			charger->report = 0;                            \
		}                                                       \
	} while (0)

static struct axp173_charger *g_axp173_charger = NULL;

static char *supply_list[] = {
	"battery",
	"battery-adc",
};

enum adc_type {
	ACIN_VOL = 0,
	ACIN_CUR,
	VBUS_VOL,
	VBUS_CUR,
	BAT_VOL,
	BAT_CUR,
};

static enum power_supply_property axp173_battery_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_ENERGY_EMPTY,
	POWER_SUPPLY_PROP_ENERGY_EMPTY_DESIGN,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property axp173_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static void axp173_charger_read_reg(struct i2c_client *client,
				    unsigned char reg, unsigned char *val)
{
	int ret = 0;

	ret = axp173_read_reg(client, reg, val);
	if (ret < 0)
		dev_warn(&client->dev, "%s reg %d read error\n",
			 __func__, reg);
}

static void axp173_charger_write_reg(struct i2c_client *client,
				     unsigned char reg, unsigned char val)
{
	int ret = 0;

	ret = axp173_write_reg(client, reg, val);
	if (ret < 0)
		dev_warn(&client->dev, "%s reg %d write error\n",
			 __func__, reg);
}

static int axp173_disable_charge(struct axp173_charger *charger)
{
	unsigned char temp;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_CHARGE1, &temp);
	temp &= 0x7f;
	axp173_charger_write_reg(client, POWER_CHARGE1, temp);

	return 0;
}

static int axp173_enable_charge(struct axp173_charger *charger)
{
	unsigned char temp;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_CHARGE1, &temp);
	temp |= 0x80;
	axp173_charger_write_reg(client, POWER_CHARGE1, temp);

	return 0;
}

static void axp173_clr_coulomb(struct axp173_charger *charger)
{
	unsigned char value = 0;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_COULOMB_CTL, &value);
	value |= 0x20;
	value &= 0xbf;
	axp173_charger_write_reg(client, POWER_COULOMB_CTL, value);
	value |= 0x80;
	value &= 0xbf;
	axp173_charger_write_reg(client, POWER_COULOMB_CTL, value);
}

static unsigned int axp173_get_single_adc_data(struct axp173_charger *charger,
					       enum adc_type type)
{
	unsigned char tmp[2] = {0,};
	unsigned int val = 0;
	struct i2c_client *client = charger->axp173->client;

#define GET_ADC_VALUE(reg1, reg2)                       \
	axp173_charger_read_reg(client, reg1, tmp);     \
	axp173_charger_read_reg(client, reg2, tmp+1)

	switch (type) {
	case ACIN_VOL:
		GET_ADC_VALUE(POWER_ACIN_VOL_H8, POWER_ACIN_VOL_L4);
		val = ((tmp[0] << 4) + (tmp[1] & 0x0f)) * 17 / 10;
		break;
	case ACIN_CUR:
		GET_ADC_VALUE(POWER_ACIN_CUR_H8, POWER_ACIN_CUR_L4);
		val = ((tmp[0] << 4) + (tmp[1] & 0x0f)) * 5 / 8;
		break;
	case VBUS_VOL:
		GET_ADC_VALUE(POWER_VBUS_VOL_H8, POWER_VBUS_VOL_L4);
		val = ((tmp[0] << 4) + (tmp[1] & 0x0f)) * 17 / 10;
		break;
	case VBUS_CUR:
		GET_ADC_VALUE(POWER_VBUS_CUR_H8, POWER_VBUS_CUR_L4);
		val = ((tmp[0] << 4) + (tmp[1] & 0x0f)) * 375 / 1000;
		break;
	case BAT_VOL:
		GET_ADC_VALUE(POWER_BAT_AVERVOL_H8, POWER_BAT_AVERVOL_L4);
		val = ((tmp[0] << 4) + ((tmp[1] & 0x0f))) * 11 / 10;
		break;
	case BAT_CUR:
		GET_ADC_VALUE(POWER_BAT_AVERCHGCUR_H8, POWER_BAT_AVERCHGCUR_L5);
			val = ((tmp[0] << 5) + (tmp[1] & 0x1f)) / 2;
		GET_ADC_VALUE(POWER_BAT_AVERDISCHGCUR_H8,
			POWER_BAT_AVERDISCHGCUR_L5);
		val += ((tmp[0] << 5) + (tmp[1] & 0x1f)) / 2;
		break;
	default:
		break;
	}
#undef GET_ADC_VALUE

	return val;
}

static unsigned int axp173_get_adjust_adc_data(struct axp173_charger *charger,
					       enum adc_type type)
{
	int i = 0;
	unsigned int max_val = 0;
	unsigned int min_val = 0;
	unsigned int count = 0;
	unsigned int value = 0;
	int tmp = 0;
	int times = 0;

	for (; i < charger->sample_count; ++i) {
		tmp = axp173_get_single_adc_data(charger, type);
		if (tmp > 0) {
			if (i == 0)
				max_val = min_val = tmp;
			count += tmp;
			times++;
			if (tmp > max_val)
				max_val = tmp;
			else if (tmp < min_val)
				min_val = tmp;
		}
	}

	switch (times) {
	case 0:
		value = 0;
		break;
	case 1:
	case 2:
		value = count / times;
		break;
	default:
		value = (count-max_val-min_val) / (times-2);
		break;
	}

	return value;
}

static unsigned int axp173_get_adc_data(struct axp173_charger *charger,
					enum adc_type type, int sw_adjust)
{
	return sw_adjust + axp173_get_adjust_adc_data(charger, type);
}

static int axp173_chgled_blink_enable(struct axp173_charger *charger)
{
	unsigned char  value = 0;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_OFF_CTL, &value);
	value &= ~(0x7 << 3);
	value |= (0x3 << 3);
	axp173_charger_write_reg(client, POWER_OFF_CTL, value);

	return 0;
}

static int axp173_chgled_blink_disable(struct axp173_charger *charger)
{
	unsigned char  value = 0;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_OFF_CTL, &value);
	value &= ~(1 << 3);
	axp173_charger_write_reg(client, POWER_OFF_CTL, value);

	return 0;
}

static int axp173_prop_online(struct axp173_charger *charger,
			      struct power_supply *psy)
{
	struct i2c_client *client = charger->axp173->client;
	unsigned char reg = 0;
	unsigned char bit = 0;
	unsigned char val = 0;

	switch (psy->type) {
	case POWER_SUPPLY_TYPE_BATTERY:
		reg = POWER_MODE_CHGSTATUS;
		bit = BATTERY_IN;
		break;
	case POWER_SUPPLY_TYPE_USB:
		reg = POWER_STATUS;
		bit = USB_IN;
		break;
	case POWER_SUPPLY_TYPE_MAINS:
		reg = POWER_STATUS;
		bit = AC_IN;
		break;
	default:
		return -EINVAL;
	}
	axp173_charger_read_reg(client, reg, &val);

	return !!(val & bit);
}

static int axp173_prop_status(struct axp173_charger *charger,
			      struct power_supply *psy)
{
	struct i2c_client *client = charger->axp173->client;
	unsigned char power_status = 0;
	unsigned char power_mode_chgstatus = 0;
	unsigned char power_charge1 = 0;
	int ret = 0;

	if (psy->type != POWER_SUPPLY_TYPE_BATTERY)
		return -EINVAL;
	axp173_charger_read_reg(client, POWER_STATUS, &power_status);
	axp173_charger_read_reg(client, POWER_MODE_CHGSTATUS,
				&power_mode_chgstatus);
	axp173_charger_read_reg(client, POWER_CHARGE1, &power_charge1);
	if (!charger->battery_online)
		return -ENODATA;
	if (charger->usb_online || charger->ac_online) {
		ret = POWER_SUPPLY_STATUS_CHARGING;
		if (!(power_charge1 & CHARGE_ENABLE))
			ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
		if ((!(power_mode_chgstatus & CHARGED_STATUS)) &&
		    (power_status & (AC_AVAILABLE | USB_AVAILABLE)))
			ret = POWER_SUPPLY_STATUS_FULL;
	} else
		ret = POWER_SUPPLY_STATUS_DISCHARGING;

	return ret;
}

static unsigned char adc_freq_get(struct axp173_charger *charger)
{
	struct i2c_client *client = charger->axp173->client;
	unsigned char temp = 0;
	unsigned char value = 25;

	axp173_charger_read_reg(client, POWER_ADC_SPEED, &temp);
	temp &= 0xc0;
	switch (temp >> 6) {
	case 0:
		value = 25;
		break;
	case 1:
		value = 50;
		break;
	case 2:
		value = 100;
		break;
	case 3:
		value = 200;
		break;
	default:
		break;
	}
	return value;
}

static unsigned char get_power_monitor_time(struct axp173_charger *charger,
					    int suspend)
{
	int cur = 0;
	int min = 0;
	int charging = 0;

	if (charger->battery_status == POWER_SUPPLY_STATUS_CHARGING)
		charging = 1;

	if (suspend && !charging)
		cur = charger->suspend_current;
	else
		cur = axp173_get_adc_data(charger, BAT_CUR, 0);
	if (!cur)
		cur = 1;

	if (charging)
		min = 0x7f;
	else
		min = charger->current_cpt * charger->max_cpt
				/ 100 * 60 / 3 / cur;

	if (min <= 0)
		min = 1;
	else if (min > 0x7f)
		min = 0x7f;
	AXP173_DEBUG_MSG("monitor time = %d\n", min);

	return min;
}

static void set_power_monitor(struct axp173_charger *charger, int suspend)
{
	unsigned char min = 0;
	struct i2c_client *client = charger->axp173->client;

	if (!charger->battery_online)
		return;

	min = get_power_monitor_time(charger, suspend) | 0x80;
	axp173_charger_write_reg(client, POWER_TIMER_CTL, min);
}

static void low_power_detect(struct axp173_charger *charger, int cap, int voltage)
{
	unsigned char charging = 0;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_STATUS, &charging);
	if(charging == 0xc0) /* when ACIN:discharge's current >  charge's current */
		charging = 0;
	else
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
		charging = !!(charging & AC_AVAILABLE);
#else
		charging = !!(charging & (AC_AVAILABLE | USB_AVAILABLE));
#endif
	if (!charging && (cap == 0)) {
		power_supply_changed(&charger->battery);
		AXP173_DEBUG_MSG("****The capacity of battery is 0, please charge!****\n");
	}
}

#if 0
static int get_battery_cacpacity(struct axp173_charger *charger)
{
	int cap_chg = 0;
	int cap_dischg = 0;
	int i = 0;
	unsigned char temp[8] = {0,};
	int cap = 0;
	struct i2c_client *client = charger->axp173->client;

	if (!charger->battery_online)
		return 0;

	if (charger->battery_status == POWER_SUPPLY_STATUS_FULL)
		return 100;

	for (; i < 8; ++i)
		axp173_charger_read_reg(client,
					POWER_BAT_CHGCOULOMB3 + i, temp + i);

	cap_chg = ((temp[0] << 24) + (temp[1] << 16)
		   + (temp[2] << 8) + temp[3]);
	cap_dischg = ((temp[4] << 24) + (temp[5] << 16)
		      + (temp[6] << 8) + temp[7]);
	cap = ((charger->base_cpt + cap_chg - cap_dischg) * 4396 / 480 /
	       adc_freq_get(charger)) * 100 / charger->max_cpt;
	if (cap >= 100)
		cap = 99;
	else if (cap <= 0)
		cap = 0;
	charger->current_cpt = cap;
	low_power_detect(charger, cap);

	return cap;
}
#endif

static void init_battery_cpy(struct axp173_charger *charger)
{
	struct power_supply *psy = power_supply_get_by_name("battery-adc");
	struct jz_current_battery *battery = NULL;

	battery = container_of(psy, struct jz_current_battery, battery_adc);

	if (!charger->battery_online)
		return;

	charger->current_cpt = battery->get_battery_current_cpt(battery);
	if (charger->ac_online && (charger->current_cpt == 0))
		charger->current_cpt = 1;
	charger->real_voltage = battery->real_vol;
	AXP173_DEBUG_MSG("init cpt = %d\n", charger->current_cpt);
	charger->base_cpt = charger->max_cpt * charger->current_cpt *
			adc_freq_get(charger) / 100 * 480 / 4369;
	low_power_detect(charger, charger->current_cpt, charger->real_voltage);
}

#ifdef CONFIG_PRODUCT_X1000_ASLMOM
static int axp173_forbid_pwroff_by_pmu(struct axp173_charger *charger)
{
        unsigned char value = 0;
        struct i2c_client *client = charger->axp173->client;

        axp173_charger_read_reg(client, POWER_PEK_SET, &value);
        value &= ~(1 << 3);
        axp173_charger_write_reg(client, POWER_PEK_SET, value);

	axp173_read_reg(client, 0x36, &value);

	return 0;
}

static int axp173_enable_pwroff_by_pmu(struct axp173_charger *charger)
{
        unsigned char value = 0;
        struct i2c_client *client = charger->axp173->client;

        axp173_charger_read_reg(client, POWER_PEK_SET, &value);
        value |= (1 << 3);
        axp173_charger_write_reg(client, POWER_PEK_SET, value);

	axp173_read_reg(client, 0x36, &value);

	return 0;
}
#if 0
static int axp173_set_poweroff_time(struct axp173_charger *charger)
{
	unsigned char value = 0;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_PEK_SET, &value);
        value |= 3;
        axp173_charger_write_reg(client, POWER_PEK_SET, value);

	return 0;
}
#endif
static void axp173_battery_set_aps_warning(struct axp173_charger *charger)
{
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_write_reg(client, POWER_APS_WARNING1, 0xa7);/* set Vwarning1 3.8v */
}
#endif

static int axp173_set_poweroff_voff(struct axp173_charger *charger)
{
	unsigned char value = 0;
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_read_reg(client, POWER_VOFF_SET, &value);
	value &= ~0x7;/* set Voff 2.6v */
	axp173_charger_write_reg(client, POWER_VOFF_SET, value);

	return 0;
}
#if 0
static void axp173_read_reg_test(struct axp173_charger *charger, unsigned int reg)
{
	struct i2c_client *client = charger->axp173->client;
	unsigned char tmp;

	axp173_charger_read_reg(client, reg, &tmp);
	printk("========>in axp173_read_reg_test:read 0x%x = 0x%x\n",reg,tmp);
}
#endif
static void axp173_charger_work_int1(struct axp173_charger *charger,
				     unsigned char src)
{
	int battery_status = 0;
	unsigned char i = 0;
	unsigned char j = 0;
	unsigned char temp = 0;
	int chg_current[16] = {100, 190, 280, 360,
			       450, 550, 630, 700,
			       780, 880, 960, 1000,
			       1080, 1160, 1240, 1320};
	struct i2c_client *client = charger->axp173->client;

	for (; i < 8; ++i) {
		switch (src & (1<<i)) {
		case INT1_AC_IN:
			charger->ac_online = 1;
			for (j = 0; j < 16; ++j)
				if (charger->ac_chg_current <=  \
				    chg_current[j])
					break;
			axp173_charger_read_reg(client,
						POWER_CHARGE1, &temp);
			temp = (temp & 0xf0) | j;
			axp173_charger_write_reg(client,
						 POWER_CHARGE1, temp);
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
			axp173_forbid_pwroff_by_pmu(charger);
			axp173_enable_charge(charger);
#endif
			break;
		case INT1_AC_OUT:
			charger->ac_online = 0;
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
			axp173_enable_pwroff_by_pmu(charger);
			axp173_disable_charge(charger);
#endif
			break;
		case INT1_USB_IN:
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
			charger->usb_online = 0;
			break;
#endif
			charger->usb_online = 1;
			for (j = 0; j < 16; ++j)
				if (charger->usb_chg_current <= \
				    chg_current[j])
					break;
			axp173_charger_read_reg(client,
						POWER_CHARGE1, &temp);
			temp = (temp & 0xf0) | j;
			axp173_charger_write_reg(client,
						 POWER_CHARGE1, temp);
			break;
		case INT1_USB_OUT:
			charger->usb_online = 0;
			break;
		case INT1_VBUS_VHOLD:
			dev_info(&charger->pdev->dev,
				 "VBUS low, care USB communicate\n");
			break;
		default:
			break;
		}
	}
	battery_status = axp173_prop_status(charger, &charger->battery);
	SET_AXP173_CHARGER_AREA(charger, battery_status, battery_status);
}


static void axp173_charger_work_int2(struct axp173_charger *charger,
				     unsigned char src)
{
	char i = 7;
	int update = 1;

	for (; i >= 0; --i) {
		switch (src & (1<<i)) {
		case INT2_BATTERY_IN:
			charger->battery_online = 1;
			init_battery_cpy(charger);
			set_power_monitor(charger, 0);
			break;
		case INT2_BATTERY_OUT:
			charger->battery_online = 0;
			break;
		case INT2_BATTERY_CHARGING:
			charger->battery_status =
				POWER_SUPPLY_STATUS_CHARGING;
			update = 0;
			break;
		case INT2_BATTERY_CHARGED:
			charger->battery_status =
				POWER_SUPPLY_STATUS_FULL;
			charger->base_cpt = charger->max_cpt *
				adc_freq_get(charger) * 480 / 4369;
			axp173_clr_coulomb(charger);
			update = 0;
			break;
		case INT2_BATTERY_HIGH_TEMP:
			temp_key = 1;
			break;
		case INT2_BATTERY_LOW_TEMP:
			temp_key = 2;
			break;
		default:
			break;
		}
	}
	if (update)
		charger->battery_status =
				axp173_prop_status(charger, &charger->battery);
}

static void axp173_charger_work_int3(struct axp173_charger *charger,
				     unsigned char src)
{
}

static void axp173_charger_work_int4(struct axp173_charger *charger,
				     unsigned char src)
{
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	unsigned char i = 0;

	for (; i < 8; ++i) {
		switch (src & (1<<i)) {
		case INT4_LOWVOLTAGE_WARNING1:
			AXP173_DEBUG_MSG("======================>:open boost!!\n");
			power_supply_changed(&charger->battery);
			gpio_direction_output(GPIO_BOOST, 1);
			break;
		default:
			break;
		}
	}
#endif
}

static void axp173_charger_work_int5(struct axp173_charger *charger,
				     unsigned char src)
{
	set_power_monitor(charger, 0);
}

static void axp173_charger_work(struct work_struct *work)
{
	struct axp173_charger *charger = NULL;
	struct axp173 *axp173 = NULL;
	unsigned char mask1 = 0, mask2 = 0, mask3 = 0, mask4 = 0, mask5 = 0;
	unsigned char stat1 = 0, stat2 = 0, stat3 = 0, stat4 = 0, stat5 = 0;
	struct i2c_client *client = NULL;

	charger = container_of(work, struct axp173_charger, work.work);
	axp173 = charger->axp173;
	client = axp173->client;

#define HANDLE_IRQ(x)                                                   \
	axp173_charger_read_reg(client, POWER_INTEN##x, &(mask##x));    \
	axp173_charger_read_reg(client, POWER_INTSTS##x, &(stat##x));   \
	axp173_charger_work_int##x(charger, (mask##x) & (stat##x));     \
	axp173_charger_write_reg(client, POWER_INTSTS##x,               \
				 (mask##x) & (stat##x))

	mutex_lock(&charger->lock);
	HANDLE_IRQ(1);
	HANDLE_IRQ(2);
	HANDLE_IRQ(3);
	HANDLE_IRQ(4);
	HANDLE_IRQ(5);
#undef HANDLE_IRQ
	if (mask1 & stat1) {
		if ((INT1_AC_IN | INT1_AC_OUT) & mask1 & stat1)
			power_supply_changed(&charger->ac);
		if ((INT1_USB_IN | INT1_USB_OUT) & mask1 & stat1)
			power_supply_changed(&charger->usb);
	}

	if ((mask2 & stat2) || (mask5 & stat5))
		power_supply_changed(&charger->battery);
	mutex_unlock(&charger->lock);

	enable_irq(charger->irq);
}


static irqreturn_t axp173_charger_irq(int irq, void *data)
{
	struct axp173_charger *charger = data;

	disable_irq_nosync(charger->irq);
	schedule_delayed_work(&charger->work, msecs_to_jiffies(500));

	return IRQ_HANDLED;
}

static int __devinit axp173_charger_initialize(struct axp173_charger *charger)
{
	struct axp173 *axp173 = charger->axp173;
	struct i2c_client *client = axp173->client;
	unsigned char ips_set = 0,
			battery_dete = 0, adc_en1 = 0, adc_speed = 0;
	unsigned char int1 = 0, int2 = 0, int3 = 0, int4 = 0, int5 = 0;

/*	ips_set = VBUS_VHOLD_EN | VBUS_VHOLD_VAL(4) | VBUS_CL_EN;*/
	axp173_charger_write_reg(client, POWER_IPS_SET, ips_set);

	battery_dete = BATTERY_DETE;
	axp173_charger_write_reg(client, POWER_BATDETECT_CHGLED_REG,
				 battery_dete);

#ifdef CONFIG_AXP173_BAT_TEMP_DET
	adc_en1 = BAT_ADC_EN_V | BAT_ADC_EN_C | AC_ADC_EN_V |
			AC_ADC_EN_C | USB_ADC_EN_V | USB_ADC_EN_C |
			APS_ADC_EN_V | TS_ADC_EN;
#else
	adc_en1 = BAT_ADC_EN_V | BAT_ADC_EN_C | AC_ADC_EN_V |
			AC_ADC_EN_C | USB_ADC_EN_V | USB_ADC_EN_C |
			APS_ADC_EN_V;
#endif
	axp173_charger_write_reg(client, POWER_ADC_EN1, adc_en1);
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	adc_speed = ADC_SPEED_BIT1 | ADC_SPEED_BIT2 | ADC_TS_OUT(2) |
			ADC_TS_OUT_TYPE(3);
#else
	adc_speed = ADC_SPEED_BIT1 | ADC_SPEED_BIT2;
#endif
	axp173_charger_write_reg(client, POWER_ADC_SPEED, adc_speed);
	int1 = INT1_AC_IN | INT1_AC_OUT | INT1_USB_IN |
			INT1_USB_OUT | INT1_VBUS_VHOLD;
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	int1 &= ~(INT1_USB_IN | INT1_USB_OUT | INT1_VBUS_VHOLD);
#endif
	axp173_charger_write_reg(client, POWER_INTEN1, int1);
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	int2 = INT2_BATTERY_IN | INT2_BATTERY_OUT |
			INT2_BATTERY_CHARGING | INT2_BATTERY_CHARGED |
			INT2_BATTERY_HIGH_TEMP | INT2_BATTERY_LOW_TEMP;
#else
	int2 = INT2_BATTERY_IN | INT2_BATTERY_OUT |
			INT2_BATTERY_CHARGING | INT2_BATTERY_CHARGED;
#endif

#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	int4 = INT4_LOWVOLTAGE_WARNING1 | INT4_LOWVOLTAGE_WARNING2;
#endif
	axp173_charger_write_reg(client, POWER_INTEN2, int2);
	axp173_charger_write_reg(client, POWER_INTEN3, int3);
	axp173_charger_write_reg(client, POWER_INTEN4, int4);
	int5 = INT5_TIMER;
	axp173_charger_write_reg(client, POWER_INTEN5, int5);

	return 0;
}

static int __init
axp173_charger_late_initialize(struct axp173_charger *charger)
{
	axp173_clr_coulomb(charger);

	return 0;
}


static int axp173_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;
	struct axp173_charger *charger =
		container_of(psy, struct axp173_charger, battery);

	mutex_lock(&charger->lock);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (charger->battery_online)
			val->intval = charger->battery_status;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = charger->battery_online;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		if (charger->battery_online)
			val->intval = POWER_SUPPLY_TECHNOLOGY_LIPO;
		else
			val->intval = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (charger->battery_online)
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		else
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = charger->current_cpt;
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
		int tmp;
		if (charger->battery_online) {
			tmp = axp173_get_adc_data(charger,
							BAT_VOL, 0);
		} else
			tmp = 0;
		/* close boost */
		if(__gpio_get_value(GPIO_BOOST) && (tmp > 3800)) {
			gpio_direction_output(GPIO_BOOST, 0);
			AXP173_DEBUG_MSG("==================>close boost!\n");
		}
#endif
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if (charger->battery_online) {
			val->intval = axp173_get_adc_data(charger,
							  BAT_VOL, 0);
		} else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->battery_online)
			val->intval = axp173_get_adc_data(charger,
							  BAT_CUR, 0);
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_ENERGY_EMPTY:
		break;
	case POWER_SUPPLY_PROP_ENERGY_EMPTY_DESIGN:
		break;
	case POWER_SUPPLY_PROP_TEMP:
		if(charger->battery_online) {
			if(!temp_key)
				val->intval = POWER_SUPPLY_TEMP_NORMAL;
			else if(temp_key == 1)
				val->intval = POWER_SUPPLY_TEMP_HIGH_TEMPERATURE;
			else if(temp_key == 2)
				val->intval = POWER_SUPPLY_TEMP_LOW_TEMPERATURE;
			else
				val->intval = POWER_SUPPLY_TEMP_UNKNOWN;
		}
		else
			val->intval = POWER_SUPPLY_TEMP_UNKNOWN;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&charger->lock);

	return ret;
}

static int axp173_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	int ret = 0;
	struct axp173_charger *charger = NULL;

	if (psy->type == POWER_SUPPLY_TYPE_USB)
		charger = container_of(psy, struct axp173_charger, usb);
	else if (psy->type == POWER_SUPPLY_TYPE_MAINS)
		charger = container_of(psy, struct axp173_charger, ac);
	else
		return -EINVAL;

	mutex_lock(&charger->lock);
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_USB)
			val->intval = charger->usb_online;
		else
			val->intval = charger->ac_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if ((psy->type == POWER_SUPPLY_TYPE_USB) &&     \
		    charger->usb_online)
			val->intval = axp173_get_adc_data(charger,
							  VBUS_VOL, 0);
		else if ((psy->type == POWER_SUPPLY_TYPE_MAINS) && \
			 charger->ac_online)
			val->intval = axp173_get_adc_data(charger,
							  ACIN_VOL, 0);
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if ((psy->type == POWER_SUPPLY_TYPE_USB) &&     \
		    charger->usb_online)
			val->intval = axp173_get_adc_data(charger,
							  VBUS_CUR, 0);
		else if ((psy->type == POWER_SUPPLY_TYPE_MAINS) &&
			 charger->ac_online)
			val->intval = axp173_get_adc_data(charger,
							  ACIN_CUR, 0);
		else
			val->intval = 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&charger->lock);

	return ret;
}

static int axp173_battery_set_property(struct power_supply *psy,
                                       enum power_supply_property psp,
                                       const union power_supply_propval *val)
{
	int ret = 0;
	struct axp173_charger *charger =
		container_of(psy, struct axp173_charger, battery);

        mutex_lock(&charger->lock);
        switch (psp) {
        case POWER_SUPPLY_PROP_ENERGY_EMPTY:
		if (charger->battery_online) {
			printk("Low power warning!\n");
			axp173_chgled_blink_enable(charger);
		} else {
			dev_err(&charger->pdev->dev, "There's no battery!\n");
			ret = -EINVAL;
		}
		break;
        case POWER_SUPPLY_PROP_ENERGY_EMPTY_DESIGN:
		if (charger->battery_online) {
			printk("Power is normal.\n");
			axp173_chgled_blink_disable(charger);
		} else {
			dev_err(&charger->pdev->dev, "There's no battery!\n");
			ret = -EINVAL;
		}
		break;
        default:
                ret = -EPERM;
		break;
        }
	mutex_unlock(&charger->lock);

        return ret;
}

static int axp173_battery_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
        switch (psp) {
        case POWER_SUPPLY_PROP_ENERGY_EMPTY:
	case POWER_SUPPLY_PROP_ENERGY_EMPTY_DESIGN:
                return 1;

        default:
                break;
        }

        return 0;
}

static void axp173_battery_external_power_changed(struct power_supply *psy)
{
	struct axp173_charger *charger =
			container_of(psy, struct axp173_charger, battery);

	BATTERY_CHANGED_REPORT(charger);
}

static int __devinit axp173_power_supply_initialize(struct axp173_charger *charger)
{
	int ret = 0;

#define DEF_POWER(PSY, NAME, TYPE, SUPPLY,                              \
		  PROPERTIES, GET_PROPERTY, SET_PROPERTY, 		\
		  WRITEABLE, CHANGED, APM)				\
	charger->PSY.name = NAME;                                       \
	charger->PSY.type = TYPE;                                       \
	if (SUPPLY) {                                                   \
		charger->PSY.supplied_to = supply_list;                 \
		charger->PSY.num_supplicants = ARRAY_SIZE(supply_list); \
	}                                                               \
	charger->PSY.properties = PROPERTIES;                           \
	charger->PSY.num_properties = ARRAY_SIZE(PROPERTIES);           \
	charger->PSY.get_property = GET_PROPERTY;			\
	charger->PSY.set_property = SET_PROPERTY;			\
	charger->PSY.property_is_writeable = WRITEABLE;			\
	charger->PSY.external_power_changed = CHANGED;			\
	charger->PSY.use_for_apm = APM

	DEF_POWER(battery, "battery", POWER_SUPPLY_TYPE_BATTERY, 0,
		  axp173_battery_power_properties,
		  axp173_battery_get_property,
		  axp173_battery_set_property,
		  axp173_battery_property_is_writeable,
		  axp173_battery_external_power_changed, 1);
	DEF_POWER(usb, "usb", POWER_SUPPLY_TYPE_USB, 1,
		  axp173_power_properties, axp173_get_property,
		  NULL, NULL, NULL, 0);
	DEF_POWER(ac, "ac", POWER_SUPPLY_TYPE_MAINS, 1,
		  axp173_power_properties, axp173_get_property,
		  NULL, NULL, NULL, 0);
#undef DEF_POWER

	ret = power_supply_register(&charger->pdev->dev, &charger->battery);
	if (ret < 0) {
		dev_err(&charger->pdev->dev,
			"Power battery register failed, ret = %d\n", ret);
		goto battery_reg_err;
	}
	ret = power_supply_register(&charger->pdev->dev, &charger->usb);
	if (ret < 0)
		dev_warn(&charger->pdev->dev,
			 "Power usb register failed, ret = %d\n", ret);
	ret = power_supply_register(&charger->pdev->dev, &charger->ac);
	if (ret < 0)
		dev_warn(&charger->pdev->dev,
			 "Power ac register failed, ret = %d\n", ret);

	return 0;

battery_reg_err:
	return ret;
}

static void axp173_power_supply_remove(struct axp173_charger *charger)
{
	power_supply_unregister(&charger->battery);
	power_supply_unregister(&charger->usb);
	power_supply_unregister(&charger->ac);
}

static int get_pmu_charging(void *pmu_interface)
{
	struct axp173_charger *charger = (struct axp173_charger *)pmu_interface;

	return charger->battery_status == POWER_SUPPLY_STATUS_CHARGING
			? CHARGING_ON : CHARGING_OFF;
}

static int get_audio_volume(void)
{

	int cur = 0;
	int i = 0;

	AXP173_DEBUG_MSG("=>>>>>>>>in %s:volume = %d\n", __func__, volume);
	if(audio_playing) {
		for (; i < ARRAY_SIZE(cur2vol); ++i) {
			if (volume >= cur2vol[i].vol)
				break;
		}
		cur = cur2vol[i].cur;
	}else
		cur = 170;

	return cur;
}

static int get_pmu_current(void *pmu_interface)
{
	return get_audio_volume();
}

static int get_pmu_voltage(void *pmu_interface)
{
	struct axp173_charger *charger =
			(struct axp173_charger *)pmu_interface;

	return axp173_get_adc_data(charger, BAT_VOL, 0);
}


static void axp173_battery_capacity_rising(struct axp173_charger *charger)
{
	charger->next_scan_time = 60;

	if(charger->current_cpt >= 100) {
		charger->current_cpt = 99;
		return;
	}

	if (charger->current_cpt == 99)
		return;

	if((charger->real_voltage < 3700) && (charger->real_voltage > 3485)
		&& (charger->real_cpt < 30))
		charger->next_scan_time = 15;
	else if(charger->real_cpt == 100)
		charger->next_scan_time = 5 * 60;

	if(charger->real_cpt > charger->current_cpt)
		charger->current_cpt++;
	AXP173_DEBUG_MSG("=>>>>>>in %s:after++,current = %d\n",__func__,charger->current_cpt);
}

static void axp173_battery_capacity_falling(struct axp173_charger *charger)
{
	charger->next_scan_time = 60;

	if((charger->real_voltage < 3700) && (charger->real_voltage > 3485)
		&& (charger->real_cpt < 30))
		charger->next_scan_time = 15;
	else if(charger->real_cpt == 100)
		charger->next_scan_time = 5 * 60;

	if(charger->real_cpt < charger->current_cpt)
		charger->current_cpt--;

	if(charger->current_cpt < 0)
		charger->current_cpt = 0;
	AXP173_DEBUG_MSG("=>>>>>>in %s:after--,current = %d\n",__func__,charger->current_cpt);
}

static void axp173_battery_capacity_full(struct axp173_charger *charger)
{
	if (charger->current_cpt >= 99) {
		charger->current_cpt = 100;
		charger->next_scan_time = 5 * 60;
	} else {
		charger->next_scan_time = 60;
		charger->current_cpt++;
	}
	AXP173_DEBUG_MSG("=>>>>>>in %s:after++,current = %d\n",__func__,charger->current_cpt);
}

static void axp173_battery_set_scan_time(struct axp173_charger *charger)
{
	switch (charger->battery_status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		axp173_battery_capacity_rising(charger);
		break;
	case POWER_SUPPLY_STATUS_FULL:
		axp173_battery_capacity_full(charger);
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		axp173_battery_capacity_falling(charger);
		break;
	case POWER_SUPPLY_STATUS_UNKNOWN:
		charger->next_scan_time = 60;
		break;
	}
}

static void axp173_battery_update_cpt_work(struct axp173_charger *charger)
{
	struct power_supply *psy = power_supply_get_by_name("battery-adc");
	struct jz_current_battery *battery = NULL;

	battery = container_of(psy, struct jz_current_battery, battery_adc);

	charger->real_cpt = battery->get_battery_current_cpt(battery);
	AXP173_DEBUG_MSG("=>>>>>>in %s:real_cpt = %d\n",__func__,charger->real_cpt);
	charger->real_voltage = battery->real_vol;
	low_power_detect(charger, charger->real_cpt, charger->real_voltage);
	axp173_battery_set_scan_time(charger);
}

static void axp173_battery_update_work(struct work_struct *work)
{
	struct axp173_charger *charger = NULL;

	charger = container_of(work, struct axp173_charger, update_work.work);

	axp173_battery_update_cpt_work(charger);

	AXP173_DEBUG_MSG("=>>>>>>>>in %s:Next check time is %ds. audio_playing = %d\n",
		__func__, charger->next_scan_time, audio_playing);
	schedule_delayed_work(&charger->update_work, charger->next_scan_time * HZ);
}


#ifdef CONFIG_AXP173_BAT_TEMP_DET
static void axp173_battery_temp_det(struct axp173_charger *charger)
{
	struct i2c_client *client = charger->axp173->client;

	axp173_charger_write_reg(client, POWER_VLTF_CHGSET,
					charger->low_temp_chg);
	axp173_charger_write_reg(client, POWER_VHTF_CHGSET,
					charger->high_temp_chg);
	axp173_charger_write_reg(client, POWER_VLTF_DISCHGSET,
					charger->low_temp_dischg);
	axp173_charger_write_reg(client, POWER_VHTF_DISCHGSET,
					charger->high_temp_dischg);
}
#endif
static void __devinit axp173_charger_callback(struct axp173_charger *charger)
{
	struct power_supply *psy = power_supply_get_by_name("battery-adc");
	struct jz_current_battery *battery = NULL;
	battery = container_of(psy, struct jz_current_battery, battery_adc);

	battery->pmu_interface = charger;
	battery->get_pmu_charging = get_pmu_charging;
	battery->get_pmu_current = get_pmu_current;
	battery->get_pmu_voltage = get_pmu_voltage;
}

static void __init axp173_charger_get_info(struct axp173_charger *charger)
{
	struct power_supply *psy = power_supply_get_by_name("battery-adc");
	struct jz_current_battery *battery = NULL;
	unsigned char power_src = 0;
	battery = container_of(psy, struct jz_current_battery, battery_adc);

	charger->battery_online = axp173_prop_online(charger,
						     &charger->battery);
	charger->usb_online = axp173_prop_online(charger, &charger->usb);
	charger->ac_online = axp173_prop_online(charger, &charger->ac);
	charger->battery_status = axp173_prop_status(charger,
						     &charger->battery);
	charger->max_cpt = battery->get_battery_max_cpt(battery);
	charger->ac_chg_current = battery->get_battery_ac_chg_current(battery);
	charger->usb_chg_current =
			battery->get_battery_usb_chg_current(battery);
	charger->sample_count = battery->get_battery_sample_count(battery);
	charger->suspend_current =
			battery->get_battery_suspend_current(battery);
	if (charger->ac_online)
		power_src |= INT1_AC_IN;
	if (charger->usb_online)
		power_src |= INT1_USB_IN;
	axp173_charger_work_int1(charger, power_src);
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	charger->low_temp_chg = battery->get_battery_low_temp_chg(battery);
	charger->high_temp_chg = battery->get_battery_high_temp_chg(battery);
	charger->low_temp_dischg = battery->get_battery_low_temp_dischg(battery);
	charger->high_temp_dischg = battery->get_battery_high_temp_dischg(battery);
	axp173_battery_temp_det(charger);
#endif

	AXP173_DEBUG_MSG("%s: battery is online %d\n",
			 __func__, charger->battery_online);
	AXP173_DEBUG_MSG("%s: usb is online %d\n",
			 __func__, charger->usb_online);
	AXP173_DEBUG_MSG("%s: ac is online %d\n",
			 __func__, charger->ac_online);
	AXP173_DEBUG_MSG("%s: battery status is %d\n",
			 __func__, charger->battery_status);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void axp173_charger_early_suspend(struct early_suspend *early_suspend)
{
}

static void axp173_charger_late_resume(struct early_suspend *early_suspend)
{
}
#endif

#ifdef CONFIG_PM
static int axp173_charger_suspend(struct platform_device *pdev,
				  pm_message_t state)
{
	struct axp173_charger *charger = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&charger->work);
	/* NOTE: The App should stop everything, just like backlight */
	set_power_monitor(charger, 1);

	return 0;
}

static int axp173_charger_resume(struct platform_device *pdev)
{
	struct axp173_charger *charger = platform_get_drvdata(pdev);
        struct power_supply *psy = power_supply_get_by_name("battery-adc");
        struct jz_current_battery *battery = NULL;

        battery = container_of(psy, struct jz_current_battery, battery_adc);


	/* NOTE: If the battery has power, the App should suspend again */
//	get_battery_cacpacity(charger);
	charger->real_cpt = battery->get_battery_current_cpt(battery);
	charger->real_voltage = battery->real_vol;
	low_power_detect(charger, charger->real_cpt, charger->real_voltage);

	return 0;
}
#endif

static int axp173_battery_notifier_handler_play(struct jz_notifier *nb,void* data)
{
	int *tmp = data;

	audio_playing = 1;

	if(tmp) {
		volume = *tmp;
	}

	return 0;
}

static int axp173_battery_notifier_handler_stop(struct jz_notifier *nb,void* data)
{
	audio_playing = 0;

	return 0;
}

static int axp173_register_battery_notifier_play(struct jz_notifier *nb)
{
	return jz_notifier_register(nb,NOTEFY_PROI_NORMAL);
}

static int axp173_register_battery_notifier_stop(struct jz_notifier *nb)
{
	return jz_notifier_register(nb,NOTEFY_PROI_LOW);
}

static int __devinit axp173_charger_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct axp173 *axp173 = NULL;
	struct pmu_platform_data *pdata = NULL;
	struct axp173_charger *charger = NULL;

	AXP173_DEBUG_MSG("%s in\n", __func__);

	axp173 = dev_get_drvdata(pdev->dev.parent);
	if (!axp173) {
		dev_err(&pdev->dev, "No platform drvdata\n");
		return -ENXIO;
	}
	pdata = dev_get_platdata(axp173->dev);
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		return -ENXIO;
	}

	charger = kzalloc(sizeof(struct axp173_charger), GFP_KERNEL);
	if (!charger) {
		dev_err(&pdev->dev, "Failed to allocate driver structure\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&charger->work, axp173_charger_work);
	INIT_DELAYED_WORK(&charger->update_work, axp173_battery_update_work);

	gpio_request(GPIO_BOOST, "boost");

	if (gpio_request_one(pdata->gpio, GPIOF_DIR_IN, "axp173-int")) {
		dev_err(&pdev->dev, "No irq detect pin available\n");
		pdata->gpio = -EBUSY;
		ret = -ENODEV;
		goto gpio_req_err;
	}
	charger->irq = gpio_to_irq(pdata->gpio);
	if (charger->irq < 0) {
		ret = charger->irq;
		dev_err(&pdev->dev,
			"Failed to get platform irq, ret = %d\n", ret);
		goto get_irq_err;
	}
	ret = request_irq(charger->irq, axp173_charger_irq,
			  IRQF_TRIGGER_LOW | IRQF_DISABLED,
			  "charger-detect", charger);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq, ret = %d\n", ret);
		goto request_irq_err;
	}
	enable_irq_wake(charger->irq);
	disable_irq(charger->irq);
	charger->axp173 = axp173;
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	charger->battery_notifier_play.jz_notify = axp173_battery_notifier_handler_play;
	charger->battery_notifier_play.level = NOTEFY_PROI_HIGH;
	charger->battery_notifier_play.msg = JZ_POST_HIBERNATION;

	charger->battery_notifier_stop.jz_notify = axp173_battery_notifier_handler_stop;
	charger->battery_notifier_stop.level = NOTEFY_PROI_HIGH;
	charger->battery_notifier_stop.msg = JZ_POST_HIBERNATION;
#endif
	charger->next_scan_time = 60;
	charger->pdev = pdev;
	platform_set_drvdata(pdev, charger);
	mutex_init(&charger->lock);

	ret = axp173_charger_initialize(charger);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to init axp173, ret = %d\n", ret);
		goto axp173_init_err;
	}
	ret = axp173_power_supply_initialize(charger);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"Failed to reg power supply, ret = %d\n", ret);
		goto pwr_reg_err;
	}
	axp173_charger_callback(charger);

#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	ret = axp173_register_battery_notifier_play(&(charger->battery_notifier_play));
	if (ret) {
		dev_err(&pdev->dev, "axp173_register_battery_notifier_play failed\n");
		goto pwr_reg_err;
	}

	ret = axp173_register_battery_notifier_stop(&(charger->battery_notifier_stop));
	if (ret) {
		dev_err(&pdev->dev, "axp173_register_battery_notifier_stop failed\n");
		goto pwr_reg_err;
	}

	axp173_disable_charge(charger);
	schedule_delayed_work(&charger->update_work, 5 * HZ);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	charger->early_suspend.suspend = axp173_charger_early_suspend;
	charger->early_suspend.resume = axp173_charger_late_resume;
	charger->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	register_early_suspend(&charger->early_suspend);
#endif

	ret = sysfs_create_group(&pdev->dev.kobj, &axp173_attr_group);
	if (ret < 0)
		dev_warn(&pdev->dev, "Failed to create sysfs group!\n");

	g_axp173_charger = charger;
	AXP173_DEBUG_MSG("%s out\n", __func__);

	return 0;
pwr_reg_err:
axp173_init_err:
	free_irq(charger->irq, charger);
request_irq_err:
get_irq_err:
	gpio_free(pdata->gpio);
gpio_req_err:
	kfree(charger);

	return ret;
}

static int __devexit axp173_charger_remove(struct platform_device *pdev)
{
	struct axp173_charger *charger = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&charger->work);
	cancel_delayed_work_sync(&charger->update_work);
	sysfs_remove_group(&pdev->dev.kobj, &axp173_attr_group);
	axp173_power_supply_remove(charger);
	free_irq(charger->irq, charger);
	kfree(charger);

	return 0;
}

static struct platform_driver axp173_charger_driver = {
	.probe		= axp173_charger_probe,
	.remove		= __devexit_p(axp173_charger_remove),
	.driver = {
		.name = "axp173-charger",
		.owner = THIS_MODULE,
	},
#ifdef CONFIG_PM
	.suspend	= axp173_charger_suspend,
	.resume		= axp173_charger_resume,
#endif
};

static int __init axp173_charger_init(void)
{
	return platform_driver_register(&axp173_charger_driver);
}
module_init(axp173_charger_init);

static void __exit axp173_charger_exit(void)
{
	platform_driver_unregister(&axp173_charger_driver);
}
module_exit(axp173_charger_exit);

static int __init axp173_charger_late_init(void)
{
#if 0
	axp173_set_poweroff_time(g_axp173_charger);
#endif
#ifdef CONFIG_PRODUCT_X1000_ASLMOM
	axp173_set_poweroff_voff(g_axp173_charger);
	axp173_battery_set_aps_warning(g_axp173_charger);
//	axp173_read_reg_test(g_axp173_charger, 0x31);
//	axp173_read_reg_test(g_axp173_charger, 0x3A);
#endif

	axp173_charger_get_info(g_axp173_charger);
	axp173_charger_late_initialize(g_axp173_charger);
	init_battery_cpy(g_axp173_charger);
	set_power_monitor(g_axp173_charger, 0);
	enable_irq(g_axp173_charger->irq);

	return 0;
}

late_initcall(axp173_charger_late_init);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Mingli Feng <mingli.feng@ingenic.com>");
MODULE_DESCRIPTION("axp173 charger driver for JZ battery");
