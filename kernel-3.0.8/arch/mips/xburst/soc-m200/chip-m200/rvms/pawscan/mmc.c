#include <linux/mmc/host.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <mach/jzmmc.h>

#include "board.h"

extern int jzmmc_manual_detect(int index, int on);
extern int jzmmc_clk_ctrl(int index, int on);

struct wifi_data {
	struct wake_lock		wifi_wake_lock;
	struct regulator		*wifi_power;
	int				wifi_reset;
};

static struct wifi_data			iw8101_data;

int iw8101_wlan_init(void);
#define RESET				0
#define NORMAL				1

#ifndef CONFIG_NAND
#ifdef CONFIG_JZMMC_V12_MMC0

struct jzmmc_platform_data inand_pdata = {
	.removal  			= DONTCARE,
	.sdio_clk			= 0,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED \
                                          | MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE,
	.pm_flags			= 0,
	.max_freq			= CONFIG_MMC0_MAX_FREQ,
	.gpio				= NULL,
#ifdef CONFIG_MMC0_PIO_MODE
	.pio_mode			= 1,
#else
	.pio_mode			= 0,
#endif
	.private_init			= NULL,
};
#endif
#endif

#ifdef CONFIG_JZMMC_V12_MMC1
struct jzmmc_platform_data sdio_pdata = {
	.removal  			= MANUAL,
	.sdio_clk			= 1,
	.ocr_avail			= MMC_VDD_29_30 | MMC_VDD_30_31,//|MMC_VDD_165_195,
	.capacity  			= MMC_CAP_4_BIT_DATA,
	.pm_flags			= MMC_PM_IGNORE_PM_NOTIFY,
	.max_freq			= CONFIG_MMC1_MAX_FREQ,
	.recovery_info			= NULL,
	.gpio				= NULL,
#ifdef CONFIG_MMC1_PIO_MODE
	.pio_mode			= 1,
#else
	.pio_mode			= 0,
#endif
	.private_init			= iw8101_wlan_init,
};
#endif //CONFIG_JZMMC_V12_MMC1

int iw8101_wlan_init(void)
{
	static struct wake_lock	*wifi_wake_lock = &iw8101_data.wifi_wake_lock;
	struct regulator *power;
	int reset;

	power = regulator_get(NULL, "wifi_vddio_18");
	if (IS_ERR(power)) {
		pr_err("wifi regulator missing\n");
		return -EINVAL;
	}

	iw8101_data.wifi_power = power;

	reset = GPIO_WIFI_RST_N;
	if (gpio_request(GPIO_WIFI_RST_N, "wifi_reset")) {
		pr_err("no wifi_reset pin available\n");
		regulator_put(power);

		return -EINVAL;
	} else {
		gpio_direction_output(reset, 1);
	}
	 iw8101_data.wifi_reset = reset;

	wake_lock_init(wifi_wake_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");

	return 0;
}

int IW8101_wlan_power_on(int flag)
{
	static struct wake_lock	*wifi_wake_lock = &iw8101_data.wifi_wake_lock;
	struct regulator *power = iw8101_data.wifi_power;
	int reset = iw8101_data.wifi_reset;

	if (wifi_wake_lock == NULL)
		pr_warn("%s: invalid wifi_wake_lock\n", __func__);
	else if (power == NULL)
		pr_warn("%s: invalid power\n", __func__);
	else if (!gpio_is_valid(reset))
		pr_warn("%s: invalid reset\n", __func__);
	else
		goto start;

	return -ENODEV;
start:
	pr_info("wlan power on:%d\n", flag);

	jzrtc_enable_clk32k();
	msleep(200);

	switch(flag) {
		case RESET:
			regulator_enable(power);

			jzmmc_clk_ctrl(1, 1);

			gpio_set_value(reset, 0);
			msleep(200);

			gpio_set_value(reset, 1);
			msleep(200);

			break;

		case NORMAL:
			regulator_enable(power);

			gpio_set_value(reset, 0);
			msleep(200);

			gpio_set_value(reset, 1);

			msleep(200);
			jzmmc_manual_detect(1, 1);

			break;
	}
	wake_lock(wifi_wake_lock);

	return 0;
}

int IW8101_wlan_power_off(int flag)
{
	static struct wake_lock	*wifi_wake_lock = &iw8101_data.wifi_wake_lock;
	struct regulator *power = iw8101_data.wifi_power;
	int reset = iw8101_data.wifi_reset;

	if (wifi_wake_lock == NULL)
		pr_warn("%s: invalid wifi_wake_lock\n", __func__);
	else if (power == NULL)
		pr_warn("%s: invalid power\n", __func__);
	else if (!gpio_is_valid(reset))
		pr_warn("%s: invalid reset\n", __func__);
	else
		goto start;

	return -ENODEV;
start:
	pr_debug("wlan power off:%d\n", flag);
	switch(flag) {
		case RESET:
			gpio_set_value(reset, 0);

			regulator_disable(power);
			jzmmc_clk_ctrl(1, 0);
			break;

		case NORMAL:
			gpio_set_value(reset, 0);

			regulator_disable(power);

 			jzmmc_manual_detect(1, 0);
			break;
	}

	wake_unlock(wifi_wake_lock);

	jzrtc_disable_clk32k();

	return 0;
}

EXPORT_SYMBOL(IW8101_wlan_power_on);
EXPORT_SYMBOL(IW8101_wlan_power_off);
