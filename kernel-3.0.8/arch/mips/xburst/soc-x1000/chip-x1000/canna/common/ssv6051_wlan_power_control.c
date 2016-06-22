#include <linux/mmc/host.h>
#include <linux/fs.h>
#include <linux/wlan_plat.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>

#include <gpio.h>
#include <soc/gpio.h>

#define GPIO_WIFI_WAKEUP     	GPIO_PC(16)
#define GPIO_WIFI_RST_N			GPIO_PC(17)
#define WLAN_SDIO_INDEX			1

#define RESET               		0
#define NORMAL              		1

struct wifi_data
{
    int wifi_reset;
}ssv_6xxx_data;

/* static int wl_pw_en = 0; */
extern void rtc32k_enable(void);
extern void rtc32k_disable(void);

void wlan_pw_en_enable(void)
{
}

void wlan_pw_en_disable(void)
{
}

int ssv_6xxx_wlan_init(void)
{
	int reset;
    pr_warn("%s: &&&&&&&&&&&&&&\n", __func__);

	reset = GPIO_WIFI_RST_N;
	if (gpio_request(reset, "wifi_reset")) {
		pr_err("ERROR: no wifi_reset pin available !!\n");
		goto err;
	} else {
	    gpio_direction_output(reset, 0);
        msleep(300);
		gpio_direction_output(reset, 1);
	}
	ssv_6xxx_data.wifi_reset = reset;

    /*
        WL_WAKE_HOST must be pulled down
    */
	if (gpio_request(GPIO_WIFI_WAKEUP, "wifi_wakeup")) {
		pr_err("ERROR: no wifi_wakeup pin available !!\n");
		goto err;
	} else {
	    gpio_direction_output(GPIO_WIFI_WAKEUP, 0);
	}

	return 0;
err:
	return -EINVAL;
}

/*
    It calls jzmmc_manual_detect() to re-scan SDIO card
*/
int ssv_wlan_power_on(int flag)
{
	int reset = ssv_6xxx_data.wifi_reset;

	if (!gpio_is_valid(reset))
		pr_warn("%s: invalid reset\n", __func__);
	else
		goto start;
	return -ENODEV;

start:
	printk("wlan power on:%d\n", flag);
	//rtc32k_enable();

	switch(flag) {
		case RESET:
			wlan_pw_en_enable();
			jzmmc_clk_ctrl(WLAN_SDIO_INDEX, 1);

			gpio_direction_output(reset, 0);
			msleep(100);
			gpio_direction_output(reset, 1);

			break;
		case NORMAL:
			wlan_pw_en_enable();

			//gpio_direction_output(reset, 0);
			msleep(200);
			gpio_direction_output(reset, 1);

			jzmmc_manual_detect(WLAN_SDIO_INDEX, 1);

			break;
	}
	return 0;
}

int ssv_wlan_power_off(int flag)
{
	int reset = ssv_6xxx_data.wifi_reset;

    if (!gpio_is_valid(reset))
		pr_warn("%s: invalid reset\n", __func__);
	else
		goto start;
	return -ENODEV;
start:
	pr_debug("wlan power off:%d\n", flag);
	switch(flag) {
		case RESET:
			gpio_direction_output(reset, 0);
			wlan_pw_en_disable();

			break;
		case NORMAL:
			gpio_direction_output(reset, 0);
			wlan_pw_en_disable();

			break;
	}

	//rtc32k_disable();

	return 0;
}

#if 0
static ssize_t bcm_ap6212_manual_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
        int ret = 0;
        /* rlt8723bs default state is power on */
        static int init_state = 1;

        if (strncmp(buf, "0", 1) == 0) {
                if (!init_state)
                        goto out;
                ret = ssv_wlan_power_off(NORMAL);
                init_state = 0;
        } else if (strncmp(buf, "1", 1) == 0) {
                if (init_state)
                        goto out;
                ret = ssv_wlan_power_on(NORMAL);
                init_state = 1;
        } else
                ret = -1;
        if (ret)
                pr_warn("===>%s bcm_ap6212 power ctl fail\n", __func__);

out:
	return count;
}
#endif

EXPORT_SYMBOL(ssv_6xxx_wlan_init);
EXPORT_SYMBOL(ssv_wlan_power_on);
EXPORT_SYMBOL(ssv_wlan_power_off);
