#include <linux/gpio.h>
#include <linux/modem_pm.h>
#include <linux/delay.h>
#include <mach/jzmmc.h>

/* global variable */
enum li170_pwr_ctrl {
	PWR_ON  = 0,
	PWR_OFF = 1
};

static struct kobject *lte_kobj;
static struct modem_data *li170;
static bool inited = false;

/* pre define */
static void li170_poweron(struct modem_data *bp);
static void li170_poweroff(struct modem_data *bp);
static void li170_reset(struct modem_data *bp);

/* modem ops */
static void li170_init(struct modem_data *bp)
{
	if (bp == NULL) {
		printk("ERROR : %s --> modem data is NULL!\n",__func__);
		return;
	}

	li170 = bp;
	if (inited == false) {
		modem_gpio_request(&bp->bp_reset,   "li170 reset");
		modem_gpio_request(&bp->bp_pwr,     "li170 power");
		modem_gpio_request(&bp->bp_onoff,   "li170 on");
		modem_gpio_request(&bp->ap_wake_bp, "ap wake");
		inited = true;
	}
	/* bcz li170 module boot up time is too long about 23s
	 * so power on it when init for tmp use 
	 */
	li170_poweron(bp);
}

static void li170_reset(struct modem_data *bp)
{
	modem_gpio_out(&bp->bp_reset, BP_ACTIVE);
	msleep(20);
	modem_gpio_out(&bp->bp_reset, BP_DEACTIVE);
}

static void li170_suspend(struct modem_data *bp)
{
}

static void li170_resume(struct modem_data *bp)
{
}

static void li170_clockon(struct modem_data *bp)
{
	jzrtc_enable_clk32k();
}

static void li170_clockoff(struct modem_data *bp)
{
	jzrtc_disable_clk32k();
}

static void li170_poweron(struct modem_data *bp)
{
	modem_gpio_out(&bp->bp_reset, BP_ACTIVE);
	li170_clockon(bp);
	modem_gpio_out(&bp->bp_pwr, BP_ACTIVE);
	msleep(20);
	modem_gpio_out(&bp->bp_onoff, BP_ACTIVE);
	msleep(20);
	modem_gpio_out(&bp->bp_reset, BP_DEACTIVE);
}

static void li170_poweroff(struct modem_data *bp)
{
	modem_gpio_out(&bp->bp_onoff, BP_DEACTIVE);
	modem_gpio_out(&bp->bp_pwr, BP_DEACTIVE);
}

static void li170_wakeup(struct modem_data *bp)
{
	modem_gpio_out(&bp->ap_wake_bp, BP_ACTIVE);
	msleep(10);
	modem_gpio_out(&bp->ap_wake_bp, BP_DEACTIVE);
}

static struct modem_ops bp_ops = {
	.name     = "LI170",
	.init     = li170_init,
	.poweron  = li170_poweron,
	.poweroff = li170_poweroff,
	.wakeup   = li170_wakeup,
	.suspend  = li170_suspend,
	.resume   = li170_resume,
	.reset    = li170_reset,
};

ssize_t lte_power_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	bool vcc = false, pwron = false;
	int ret = 0;
	if (__gpio_get_value(li170->bp_pwr.gpio) == !!li170->bp_pwr.active_level) {
		vcc = true;
	}

	if (__gpio_get_value(li170->bp_onoff.gpio) == !!li170->bp_onoff.active_level) {
		pwron = true;
	}

	ret += sprintf(buf + ret, "VCC : %s, PWRON : %s\n", vcc ? "on" : "off", pwron ? "on" : "off");
	return ret;
}

ssize_t lte_power_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t n)
{
	enum li170_pwr_ctrl ctrl;

	/* control the lte module state */
	if ( !strncmp(buf, "on", strlen("on"))) {
		ctrl = PWR_ON;
	} else if (!strncmp(buf, "off", strlen("off"))) {
		ctrl = PWR_OFF;
	} else {
		printk("%s: input %s is invalid.\n", __FUNCTION__, buf);
		return n;
	}

	switch (ctrl) {
		case PWR_ON:
			li170_poweron(li170);
			printk("power on the modem!\n");
			break;
		case PWR_OFF:
			li170_poweroff(li170);
			printk("power off the modem!\n");
			break;
		default:
			printk("unknow control command!\n");
			break;
	}

	return n;
}

ssize_t lte_reset_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	bool reset = false;
	int ret = 0;

	if (__gpio_get_value(li170->bp_reset.gpio) == !!li170->bp_reset.active_level) {
		reset = true;
	}

	if (reset) {
		ret += sprintf(buf + ret, "reset\n");
	}else{
		ret += sprintf(buf + ret, "work\n");
	}

	return ret;
}

ssize_t lte_reset_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t n)
{
	/* reset the modem */
	if ( !strncmp(buf, "reset", strlen("reset"))) {
		li170_reset(li170);
		printk("Warnning: reset modem\n");
	}

	return n;
}

#define lte_attr(_name)                                   \
	static struct kobj_attribute _name##_attr = {     \
		.attr = {                                 \
			.name = __stringify(_name),       \
			.mode = 0644,                     \
		},                                        \
		.show   = lte_##_name##_show,             \
		.store  = lte_##_name##_store,            \
	}

lte_attr(power);
lte_attr(reset);

static struct attribute *g_attr[] = {
	&power_attr.attr,
	&reset_attr.attr,
	NULL
};

static struct attribute_group g_attr_group = {
	.attrs = g_attr,
};

struct kobject *lte_kobject_add(const char *name)
{
	struct kobject *kobj = NULL;
	if(lte_kobj){
		kobj = kobject_create_and_add(name, lte_kobj);
	}

	return kobj;
}

static int lte_sysfs_init(void)
{
	int ret = 0;
	lte_kobj = kobject_create_and_add("lte", kernel_kobj);
	if (!lte_kobj){
		ret = -ENOMEM;
		goto err_create_kobj;
	}

	ret = sysfs_create_group(lte_kobj, &g_attr_group);

	if(ret){
		printk("sysfs_create_group failed\n");
		goto err_sysfs_create_group; 
	}
	return 0;

err_sysfs_create_group:
err_create_kobj:
	return ret;
}

static int __init li170_register(void)
{
	int ret = 0;
	ret = lte_sysfs_init();
	if (ret != 0)
		printk("WARNING : %s --> create sysfs failed!",__func__);
	return modem_register_ops(&bp_ops);
}

module_init(li170_register);
