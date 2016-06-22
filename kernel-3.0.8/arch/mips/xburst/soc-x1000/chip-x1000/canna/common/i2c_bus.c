#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/jzsnd.h>

#ifdef CONFIG_EEPROM_AT24
#include <asm-generic/sizes.h>
#include <linux/i2c/at24.h>
#include <mach/platform.h>
#endif

#define SZ_16K	0x00004000

#ifdef CONFIG_EEPROM_AT24
static struct at24_platform_data at24c16 = {
	.byte_len = SZ_16K / 8,
	.page_size = 16,
};
#endif
#ifdef CONFIG_WM8594_CODEC_V12
static struct snd_codec_data wm8594_codec_pdata = {
	.codec_sys_clk = 1200000,
};
#endif

#ifdef CONFIG_BATTERY_CW2015
#include <linux/power/cw2015_battery.h>

static unsigned char config_info[SIZE_BATINFO] = {
	0x15, 0x6A, 0x68, 0x63, 0x5E,
	0x5C, 0x51, 0x4E, 0x4B, 0x49,
	0x47, 0x47, 0x42, 0x41, 0x3A,
	0x22, 0x10, 0x09, 0x08, 0x0C,
	0x15, 0x36, 0x55, 0x6F, 0x86,
	0x74, 0x0C, 0xCD, 0x21, 0x41,
	0x4E, 0x53, 0x56, 0x5D, 0x68,
	0x6C, 0x3C, 0x0E, 0x73, 0x07,
	0x00, 0x3D, 0x52, 0x87, 0x8F,
	0x91, 0x94, 0x52, 0x82, 0x8C,
	0x92, 0x96, 0x5D, 0x61, 0x87,
	0xCB, 0x2F, 0x7D, 0x72, 0xA5,
	0xB5, 0xC1, 0xAE, 0x11
};

static struct cw_bat_platform_data cw_bat_platdata = {
	.bat_low_pin = GPIO_PB(19),
	.bat_low_level = GPIO_LOW,
	.chg_ok_pin = GPIO_PD(4),
	.chg_ok_level = GPIO_HIGH,
	.usb_dete_pin = GPIO_USB_DETE,
	.usb_dete_level = GPIO_LOW,
	.is_usb_charge = 1,
	.is_dc_charge = 0,
	.cw_bat_config_info = config_info,
};
#endif

#if (defined(CONFIG_SOFT_I2C0_GPIO_V12_JZ) || defined(CONFIG_I2C0_V12_JZ))
struct i2c_board_info jz_i2c0_devs[] __initdata = {
#ifdef CONFIG_SENSORS_BMA2X2
	{
		I2C_BOARD_INFO("bma2x2", 0x18),
		.irq = GPIO_GSENSOR_INTR,
	},
#endif
#ifdef CONFIG_BATTERY_CW2015
	{
		I2C_BOARD_INFO("cw201x", 0x62),
		.platform_data = &cw_bat_platdata,
	},
#endif
};
int jz_i2c0_devs_size = ARRAY_SIZE(jz_i2c0_devs);
#endif

#if (defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ))
struct i2c_board_info jz_i2c1_devs[] __initdata = {
};
int jz_i2c1_devs_size = ARRAY_SIZE(jz_i2c1_devs);
#endif

#if (defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ))
struct i2c_board_info jz_i2c2_devs[] __initdata = {
#ifdef CONFIG_EEPROM_AT24
	{
		I2C_BOARD_INFO("at24",0x57),
		.platform_data  = &at24c16,
	},
#endif
#ifdef CONFIG_WM8594_CODEC_V12
	{
		I2C_BOARD_INFO("wm8594", 0x1a),
		.platform_data  = &wm8594_codec_pdata,
	},
#endif
};
int jz_i2c2_devs_size = ARRAY_SIZE(jz_i2c2_devs);
#endif

#ifdef CONFIG_EEPROM_AT24
struct i2c_client *at24_client;
static int  at24_dev_init(void)
{
	struct i2c_adapter *i2c_adap;

	i2c_adap = i2c_get_adapter(2);
	at24_client = i2c_new_device(i2c_adap, jz_i2c2_devs);
	i2c_put_adapter(i2c_adap);

	return 0;
}

static void  at24_dev_exit(void)
{
	 i2c_unregister_device(at24_client);
}

module_init(at24_dev_init);
module_exit(at24_dev_exit);
MODULE_LICENSE("GPL");
#endif /*CONFIG_EEPROM_AT24*/

#ifdef CONFIG_I2C_GPIO
#define DEF_GPIO_I2C(NO)                        \
    static struct i2c_gpio_platform_data i2c##NO##_gpio_data = {    \
        .sda_pin    = GPIO_I2C##NO##_SDA,           \
        .scl_pin    = GPIO_I2C##NO##_SCK,           \
     };                              \
    struct platform_device i2c##NO##_gpio_device = {        \
        .name   = "i2c-gpio",                   \
        .id = NO,                       \
        .dev    = { .platform_data = &i2c##NO##_gpio_data,},    \
    };
#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
DEF_GPIO_I2C(1);
#endif
#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
DEF_GPIO_I2C(0);
#endif
#endif /*CONFIG_I2C_GPIO*/
