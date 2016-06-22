#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
//#include <linux/tsc.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/jz-axp173-adc.h>
#include <linux/power/jz-current-battery.h>
//#include <linux/android_pmem.h>
#include <mach/platform.h>
#include <mach/jzsnd.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <mach/jz_efuse.h>
#include <gpio.h>
#include <linux/jz_dwc.h>
#include <linux/interrupt.h>
//#include <sound/jz-aic.h>
#include "board_base.h"

#ifdef CONFIG_LEDS_GPIO
#include <linux/leds.h>
#endif

#ifdef CONFIG_LEDS_GPIO
static struct gpio_led gpio_leds[] = {
	{
		.name = "led-blue",
		.gpio = GPIO_PB(6),
		.active_low = false,
		.default_state = LEDS_GPIO_DEFSTATE_ON,
		.retain_state_suspended = true,
		.default_trigger = NULL,
	},
	{
		.name = "led-white",
		.gpio = GPIO_PB(7),
		.active_low = false,
		.default_state = LEDS_GPIO_DEFSTATE_ON,
		.retain_state_suspended = true,
		.default_trigger = NULL,
	},
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds = gpio_leds,
	.num_leds = ARRAY_SIZE(gpio_leds),
};

struct platform_device jz_leds_gpio = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data  = &gpio_led_info,
	},
};
#endif

#ifdef CONFIG_JZ_EFUSE_V11
struct jz_efuse_platform_data jz_efuse_pdata = {
            /* supply 2.5V to VDDQ */
            .gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif

#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
	    .num = GPIO_USB_ID,
		    .enable_level = GPIO_USB_ID_LEVEL,
};
#endif


#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
	.num = GPIO_USB_DETE,
	.enable_level = GPIO_USB_DETE_LEVEL,
};
#endif


#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
	    .num = GPIO_USB_DRVVBUS,
		    .enable_level = GPIO_USB_DRVVBUS_LEVEL,
};
#endif

/* Battery Info */
#if defined CONFIG_JZ_CURRENT_BATTERY && defined CONFIG_MFD_AXP173_SADC
struct jz_current_battery_info youth_battery_pdata = {
        .max_vol = 4245,
        .min_vol = 3445,
        .inter_resist = 251,
        .sample_count = 10,
        .adc_sw_adjust = 20,
        .ac_chg_current = 340,
        .usb_chg_current = 1000,
        .suspend_current = 30,
        .battery_max_cpt = 663,
#ifdef CONFIG_AXP173_BAT_TEMP_DET
	.low_temp_chg = 0x81,
	.high_temp_chg = 0x18,
	.low_temp_dischg = 0xCE,
	.high_temp_dischg = 0xF,
#endif
};
struct axp173_adc_platform_data adc_platform_data = {
	.battery_info = &youth_battery_pdata,
};
#endif
