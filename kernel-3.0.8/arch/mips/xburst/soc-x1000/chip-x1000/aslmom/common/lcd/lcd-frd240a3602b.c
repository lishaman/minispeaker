/*
 * Copyright (c) 2015 Engenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * JZ_x1000 fpga board lcd setup routines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include <linux/regulator/consumer.h>
#include <mach/jzfb.h>
#include "../board_base.h"
#include "board.h"

/*ifdef is 18bit,6-6-6 ,ifndef default 5-6-6*/
//#define CONFIG_SLCD_TRULY_18BIT

#ifdef	CONFIG_SLCD_TRULY_18BIT
static int slcd_inited = 1;
#else
static int slcd_inited = 0;
#endif

struct frd240a3602b_power{
	struct regulator *vlcdio;
	struct regulator *vlcdvcc;
	int inited;
};

static struct frd240a3602b_power lcd_power = {
	NULL,
    NULL,
    0
};

int frd240a3602b_power_init(struct lcd_device *ld)
{
    int ret ;
    if(GPIO_LCD_RST > 0){
        ret = gpio_request(GPIO_LCD_RST, "lcd rst");
        if (ret) {
            printk(KERN_ERR "can's request lcd rst\n");
            return ret;
        }
    }
    if(GPIO_LCD_CS > 0){
        ret = gpio_request(GPIO_LCD_CS, "lcd cs");
        if (ret) {
            printk(KERN_ERR "can's request lcd cs\n");
            return ret;
        }
    }
    if(GPIO_LCD_RD > 0){
        ret = gpio_request(GPIO_LCD_RD, "lcd rd");
        if (ret) {
            printk(KERN_ERR "can's request lcd rd\n");
            return ret;
        }
    }
    lcd_power.inited = 1;
    return 0;
}

int frd240a3602b_power_reset(struct lcd_device *ld)
{
	if (!lcd_power.inited)
		return -EFAULT;
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(10);
	gpio_direction_output(GPIO_LCD_RST, 0);
	mdelay(500);
	gpio_direction_output(GPIO_LCD_RST, 1);
	mdelay(520);

	return 0;
}

int frd240a3602b_power_on(struct lcd_device *ld, int enable)
{
	if (!lcd_power.inited && frd240a3602b_power_init(ld))
		return -EFAULT;

	if (enable) {
		gpio_direction_output(GPIO_LCD_CS, 1);
		gpio_direction_output(GPIO_LCD_RD, 1);

		frd240a3602b_power_reset(ld);

		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		mdelay(5);

	} else {
		mdelay(5);
		gpio_direction_output(GPIO_LCD_CS, 0);
		gpio_direction_output(GPIO_LCD_RD, 0);
		gpio_direction_output(GPIO_LCD_RST, 0);
		slcd_inited = 0;
	}
	return 0;
}

static struct lcd_platform_data frd240a3602b_pdata = {
	 .reset = frd240a3602b_power_reset,
	 .power_on = frd240a3602b_power_on,
};

/* LCD Panel Device */
struct platform_device frd240a3602b_device = {
	.name = "frd240a3602b_slcd",
	.dev = {
		.platform_data = &frd240a3602b_pdata,
		},
};

static struct smart_lcd_data_table frd240a3602b_data_table[] = {
#if 1
	{SMART_CONFIG_CMD,	0x00cf},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x00c1},
								{SMART_CONFIG_DATA,	0x0030},
	{SMART_CONFIG_CMD,	0x00ed},{SMART_CONFIG_DATA,	0x0064},
								{SMART_CONFIG_DATA,	0x0003},
								{SMART_CONFIG_DATA,	0x0012},
								{SMART_CONFIG_DATA,	0x0081},
	{SMART_CONFIG_CMD,	0x00e8},{SMART_CONFIG_DATA,	0x0085},
								{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0079},
	{SMART_CONFIG_CMD,	0x00cb},{SMART_CONFIG_DATA,	0x0039},
								{SMART_CONFIG_DATA,	0x002c},
								{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0034},
								{SMART_CONFIG_DATA,	0x0002},
	{SMART_CONFIG_CMD,	0x00f7},{SMART_CONFIG_DATA,	0x0020},
	{SMART_CONFIG_CMD,	0x00ea},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00c0},{SMART_CONFIG_DATA,	0x001c},
	{SMART_CONFIG_CMD,	0x00c1},{SMART_CONFIG_DATA,	0x0012},
	{SMART_CONFIG_CMD,	0x00c5},{SMART_CONFIG_DATA,	0x0020},
								{SMART_CONFIG_DATA,	0x003f},
	{SMART_CONFIG_CMD,	0x00c7},{SMART_CONFIG_DATA,	0x009a},
	{SMART_CONFIG_CMD,	0x003a},{SMART_CONFIG_DATA,	0x0066},//0x0055 : 16bit 0x0066 : 18bit
	{SMART_CONFIG_CMD,	0x0036},{SMART_CONFIG_DATA,	0x00c8},//0x0008 : rgb  0x0000 : bgr
	{SMART_CONFIG_CMD,	0x00b1},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0017},
	{SMART_CONFIG_CMD,	0x00b6},{SMART_CONFIG_DATA,	0x000a},
								{SMART_CONFIG_DATA,	0x00a2},
	{SMART_CONFIG_CMD,	0x0026},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x00e0},{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x0015},
								{SMART_CONFIG_DATA,	0x001c},
								{SMART_CONFIG_DATA,	0x001b},
								{SMART_CONFIG_DATA,	0x0008},
								{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x0048},
								{SMART_CONFIG_DATA,	0x00b8},
								{SMART_CONFIG_DATA,	0x0034},
								{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x0001},
								{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x000c},
								{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00e1},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x002a},
								{SMART_CONFIG_DATA,	0x0024},
								{SMART_CONFIG_DATA,	0x0007},
								{SMART_CONFIG_DATA,	0x0010},
								{SMART_CONFIG_DATA,	0x0007},
								{SMART_CONFIG_DATA,	0x0038},
								{SMART_CONFIG_DATA,	0x0047},
								{SMART_CONFIG_DATA,	0x004b},
								{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x0010},
								{SMART_CONFIG_DATA,	0x000e},
								{SMART_CONFIG_DATA,	0x0030},
								{SMART_CONFIG_DATA,	0x0033},
								{SMART_CONFIG_DATA,	0x000f},
	{SMART_CONFIG_CMD,	0x0011},{SMART_CONFIG_UDELAY,120000},
	{SMART_CONFIG_CMD,	0x0029},
#else

	{SMART_CONFIG_CMD,	0x00cf},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x00c1},
								{SMART_CONFIG_DATA,	0x0030},
	{SMART_CONFIG_CMD,	0x00ed},{SMART_CONFIG_DATA,	0x0064},
								{SMART_CONFIG_DATA,	0x0003},
								{SMART_CONFIG_DATA,	0x0012},
								{SMART_CONFIG_DATA,	0x0081},
	{SMART_CONFIG_CMD,	0x00e8},{SMART_CONFIG_DATA,	0x0085},
								{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0078},
	{SMART_CONFIG_CMD,	0x00cb},{SMART_CONFIG_DATA,	0x0039},
								{SMART_CONFIG_DATA,	0x002c},
								{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0034},
								{SMART_CONFIG_DATA,	0x0002},
	{SMART_CONFIG_CMD,	0x003a},{SMART_CONFIG_DATA,	0x0066},
	{SMART_CONFIG_CMD,	0x00ea},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00b1},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x001a},
	{SMART_CONFIG_CMD,	0x00b6},{SMART_CONFIG_DATA,	0x000a},
								{SMART_CONFIG_DATA,	0x00a2},
	{SMART_CONFIG_CMD,	0x00c0},{SMART_CONFIG_DATA,	0x001b},
	{SMART_CONFIG_CMD,	0x00c1},{SMART_CONFIG_DATA,	0x0012},
	{SMART_CONFIG_CMD,	0x00c2},{SMART_CONFIG_DATA,	0x0011},
	{SMART_CONFIG_CMD,	0x00c5},{SMART_CONFIG_DATA,	0x0044},
								{SMART_CONFIG_DATA,	0x0040},
	{SMART_CONFIG_CMD,	0x00c7},{SMART_CONFIG_DATA,	0x00b0},
	{SMART_CONFIG_CMD,	0x0036},{SMART_CONFIG_DATA,	0x0008},
	{SMART_CONFIG_CMD,	0x00f2},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00f7},{SMART_CONFIG_DATA,	0x0020},
	{SMART_CONFIG_CMD,	0x0026},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x00e0},{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x002c},
								{SMART_CONFIG_DATA,	0x0028},
								{SMART_CONFIG_DATA,	0x000b},
								{SMART_CONFIG_DATA,	0x000d},
								{SMART_CONFIG_DATA,	0x0005},
								{SMART_CONFIG_DATA,	0x004e},
								{SMART_CONFIG_DATA,	0x0085},
								{SMART_CONFIG_DATA,	0x0040},
								{SMART_CONFIG_DATA,	0x000a},
								{SMART_CONFIG_DATA,	0x0017},
								{SMART_CONFIG_DATA,	0x0009},
								{SMART_CONFIG_DATA,	0x0010},
								{SMART_CONFIG_DATA,	0x0006},
								{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x00e1},{SMART_CONFIG_DATA,	0x0000},
								{SMART_CONFIG_DATA,	0x0010},
								{SMART_CONFIG_DATA,	0x0013},
								{SMART_CONFIG_DATA,	0x0004},
								{SMART_CONFIG_DATA,	0x000f},
								{SMART_CONFIG_DATA,	0x0005},
								{SMART_CONFIG_DATA,	0x0022},
								{SMART_CONFIG_DATA,	0x0022},
								{SMART_CONFIG_DATA,	0x0035},
								{SMART_CONFIG_DATA,	0x0001},
								{SMART_CONFIG_DATA,	0x0008},
								{SMART_CONFIG_DATA,	0x0006},
								{SMART_CONFIG_DATA,	0x002a},
								{SMART_CONFIG_DATA,	0x002e},
								{SMART_CONFIG_DATA,	0x000f},
	{SMART_CONFIG_CMD,	0x0011},{SMART_CONFIG_UDELAY,120000},
	{SMART_CONFIG_CMD,	0x0029},
#endif
};

unsigned long frd240a3602b_cmd_buf[] = {
	0x2c2c2c2c,
};

struct fb_videomode jzfb_videomode = {
	.name = "240x320",
	.refresh = 60,
	.xres = 240,
	.yres = 320,
	.pixclock = KHZ2PICOS(5760),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzfb_platform_data jzfb_pdata = {

	.num_modes = 1,
	.modes = &jzfb_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 18,
	.width = 37,
	.height = 49,
	.pinmd = 0,

	.smart_config.rsply_cmd_high = 0,
	.smart_config.csply_active_high = 0,
	.smart_config.newcfg_fmt_conv = 1,


	.smart_config.write_gram_cmd = frd240a3602b_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(frd240a3602b_cmd_buf),
	.smart_config.bus_width = 8,
	.smart_config.data_table = frd240a3602b_data_table,
	.smart_config.length_data_table = ARRAY_SIZE(frd240a3602b_data_table),
	.dither_enable = 0,
};

#ifdef CONFIG_BACKLIGHT_PWM
static int backlight_init(struct device *dev)
{
#if 0
	int ret;
	ret = gpio_request(GPIO_LCD_PWM, "Backlight");
	if (ret) {
		printk(KERN_ERR "failed to request GPF for PWM-OUT1\n");
		return ret;
	}
#endif
#if 0
	int ret;
	ret = gpio_request(GPIO_BL_PWR_EN, "BL PWR");
	if (ret) {
		printk(KERN_ERR "failed to reqeust BL PWR\n");
		return ret;
	}
	gpio_direction_output(GPIO_BL_PWR_EN, 1);
#endif
	return 0;
}

static int backlight_notify(struct device *dev, int brightness)
{
#if 0
	if (brightness)
		gpio_direction_output(GPIO_BL_PWR_EN, 1);
	else
		gpio_direction_output(GPIO_BL_PWR_EN, 0);
#endif
	return brightness;
}

static void backlight_exit(struct device *dev)
{
	gpio_free(GPIO_LCD_PWM);
}

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id     = 4,
	.max_brightness = 255,
	.dft_brightness = 120,
	.pwm_period_ns  = 30000,
	.init       = backlight_init,
	.exit       = backlight_exit,
	.notify		= backlight_notify,
};
struct platform_device backlight_device = {
	.name       = "pwm-backlight",
	.dev        = {
		.platform_data  = &backlight_data,
	},
};
#endif
