/* linux/drivers/video/exynos/truly_tdo_hd0499k.c
 *
 * MIPI-DSI based truly_tdo_hd0499k AMOLED lcd 4.65 inch panel driver.
 *
 * Inki Dae, <inki.dae@samsung.com>
 * Donghwa Lee, <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <mach/jz_dsim.h>

#define POWER_IS_ON(pwr)	((pwr) == FB_BLANK_UNBLANK)
#define POWER_IS_OFF(pwr)	((pwr) == FB_BLANK_POWERDOWN)
#define POWER_IS_NRM(pwr)	((pwr) == FB_BLANK_NORMAL)

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

struct truly_tdo_hd0499k {
	struct device	*dev;
	unsigned int			power;
	unsigned int			id;

	struct lcd_device	*ld;
	struct backlight_device	*bd;

	struct mipi_dsim_lcd_device	*dsim_dev;
	struct lcd_platform_data	*ddi_pd;
	struct mutex			lock;
	struct regulator *lcd_vcc_reg;
	bool  enabled;
};

static struct dsi_cmd_packet truly_tdo_hd0499k_cmd_list[] = {
	{0x15, 0xFF,0x00},
	{0x15, 0xBA,0x01},//01:2lane  03:4lane
	{0x15, 0xC2,0x03},//Video Mode
	{0x15, 0xFF,0x01},
	{0x15, 0x00,0x3A},//720*1280
	{0x15, 0x01,0x33},
	{0x15, 0x02,0x53},

	{0x15, 0x09,0x85},//VGL=-6V  0x03->0x85 20121108
	{0x15, 0x36,0x7B},
	{0x15, 0xB0,0x80},
	{0x15, 0xB1,0x02},
	{0x15, 0x71,0x2C},

	{0x15, 0xFF,0x05},

	{0x15, 0x01,0x00},
	{0x15, 0x02,0x8D},
	{0x15, 0x03,0x8D},
	{0x15, 0x04,0x8D},
	{0x15, 0x05,0x30},
	{0x15, 0x06,0x33},

	{0x15, 0x07,0x77},
	{0x15, 0x08,0x00},
	{0x15, 0x09,0x00},
	{0x15, 0x0A,0x00},
	{0x15, 0x0B,0x80},
	{0x15, 0x0C,0xC8},
	{0x15, 0x0D,0x00},
	{0x15, 0x0E,0x1B},

	{0x15, 0x0F,0x07},
	{0x15, 0x10,0x57},
	{0x15, 0x11,0x00},
	{0x15, 0x12,0x00},
	{0x15, 0x13,0x1E},
	{0x15, 0x14,0x00},
	{0x15, 0x15,0x1A},
	{0x15, 0x16,0x05},

	{0x15, 0x17,0x00},
	{0x15, 0x18,0x1E},
	{0x15, 0x19,0xFF},
	{0x15, 0x1A,0x00},
	{0x15, 0x1B,0xFC},
	{0x15, 0x1C,0x80},
	{0x15, 0x1D,0x00},
	{0x15, 0x1E,0x10},

	{0x15, 0x1F,0x77},
	{0x15, 0x20,0x00},
	{0x15, 0x21,0x00},//Gate inverse 0x06
	{0x15, 0x22,0x55},
	{0x15, 0x23,0x0D},
	{0x15, 0x31,0xA0},
	{0x15, 0x32,0x00},
	{0x15, 0x33,0xB8},

	{0x15, 0x34,0xBB},
	{0x15, 0x35,0x11},
	{0x15, 0x36,0x01},
	{0x15, 0x37,0x0B},
	{0x15, 0x38,0x01},
	{0x15, 0x39,0x0B},
	{0x15, 0x44,0x08},
	{0x15, 0x45,0x80},

	{0x15, 0x46,0xCC},
	{0x15, 0x47,0x04},
	{0x15, 0x48,0x00},
	{0x15, 0x49,0x00},
	{0x15, 0x4A,0x01},
	{0x15, 0x6C,0x03},
	{0x15, 0x6D,0x03},
	{0x15, 0x6E,0x2F},

	{0x15, 0x43,0x00},
	{0x15, 0x4B,0x23},
	{0x15, 0x4C,0x01},
	{0x15, 0x50,0x23},
	{0x15, 0x51,0x01},
	{0x15, 0x58,0x23},
	{0x15, 0x59,0x01},
	{0x15, 0x5D,0x23},

	{0x15, 0x5E,0x01},
	{0x15, 0x62,0x23},
	{0x15, 0x63,0x01},
	{0x15, 0x67,0x23},
	{0x15, 0x68,0x01},
	{0x15, 0x89,0x00},
	{0x15, 0x8D,0x01},
	{0x15, 0x8E,0x64},

	{0x15, 0x8F,0x20},
	{0x15, 0x97,0x8E},
	{0x15, 0x82,0x8C},
	{0x15, 0x83,0x02},
	{0x15, 0xBB,0x0A},
	{0x15, 0xBC,0x0A},
	{0x15, 0x24,0x25},
	{0x15, 0x25,0x55},

	{0x15, 0x26,0x05},
	{0x15, 0x27,0x23},
	{0x15, 0x28,0x01},
	{0x15, 0x29,0x31},
	{0x15, 0x2A,0x5D},
	{0x15, 0x2B,0x01},
	{0x15, 0x2F,0x00},
	{0x15, 0x30,0x10},

	{0x15, 0xA7,0x12},
	{0x15, 0x2D,0x03},
	{0x15, 0xFF,0x00},
	{0x15, 0xFB,0x01},
	{0x15, 0xFF,0x01},
	{0x15, 0xFB,0x01},
	{0x15, 0xFF,0x02},
	{0x15, 0xFB,0x01},

	{0x15, 0xFF,0x03},
	{0x15, 0xFB,0x01},
	{0x15, 0xFF,0x04},
	{0x15, 0xFB,0x01},
	{0x15, 0xFF,0x05},
	{0x15, 0xFB,0x01},
	{0x15, 0xFF,0x00},
//SSD2825_DCS_wr},);ite_1A_1P(0x51,0xFF);//NOT OPEN CABC
////SSD2825_DCS_write_1A_1P(0x53,0x2C);
////SSD2825_DCS_write_1A_1P(0x55,0x00);
//
////SSD2825_Read_Register(0xF4,2);//¶ÁID
};

static void truly_tdo_hd0499k_regulator_enable(struct truly_tdo_hd0499k *lcd)
{
	int ret = 0;
	struct lcd_platform_data *pd = NULL;

	pd = lcd->ddi_pd;
	mutex_lock(&lcd->lock);
	if (!lcd->enabled) {
		regulator_enable(lcd->lcd_vcc_reg);
		if (ret)
			goto out;

		lcd->enabled = true;
	}
	msleep(pd->power_on_delay);
out:
	mutex_unlock(&lcd->lock);
}

static void truly_tdo_hd0499k_regulator_disable(struct truly_tdo_hd0499k *lcd)
{
	int ret = 0;

	mutex_lock(&lcd->lock);
	if (lcd->enabled) {
		regulator_disable(lcd->lcd_vcc_reg);
		if (ret)
			goto out;

		lcd->enabled = false;
	}
out:
	mutex_unlock(&lcd->lock);
}

static void truly_tdo_hd0499k_sleep_in(struct truly_tdo_hd0499k *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void truly_tdo_hd0499k_sleep_out(struct truly_tdo_hd0499k *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void truly_tdo_hd0499k_display_on(struct truly_tdo_hd0499k *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void truly_tdo_hd0499k_display_off(struct truly_tdo_hd0499k *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void truly_tdo_hd0499k_panel_init(struct truly_tdo_hd0499k *lcd)
{
	int  i;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	for(i = 0; i < ARRAY_SIZE(truly_tdo_hd0499k_cmd_list); i++) {
		ops->cmd_write(dsi,  truly_tdo_hd0499k_cmd_list[i]);
	}
}

static int truly_tdo_hd0499k_set_power(struct lcd_device *ld, int power)
{
	int ret = 0;
#if 0
	struct truly_tdo_hd0499k *lcd = lcd_get_data(ld);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);

	if (power != FB_BLANK_UNBLANK && power != FB_BLANK_POWERDOWN &&
			power != FB_BLANK_NORMAL) {
		dev_err(lcd->dev, "power value should be 0, 1 or 4.\n");
		return -EINVAL;
	}

	if ((power == FB_BLANK_UNBLANK) && ops->set_blank_mode) {
		/* LCD power on */
		if ((POWER_IS_ON(power) && POWER_IS_OFF(lcd->power))
			|| (POWER_IS_ON(power) && POWER_IS_NRM(lcd->power))) {
			ret = ops->set_blank_mode(lcd_to_master(lcd), power);
			if (!ret && lcd->power != power)
				lcd->power = power;
		}
	} else if ((power == FB_BLANK_POWERDOWN) && ops->set_early_blank_mode) {
		/* LCD power off */
		if ((POWER_IS_OFF(power) && POWER_IS_ON(lcd->power)) ||
		(POWER_IS_ON(lcd->power) && POWER_IS_NRM(power))) {
			ret = ops->set_early_blank_mode(lcd_to_master(lcd),
							power);
			if (!ret && lcd->power != power)
				lcd->power = power;
		}
	}

#endif
	return ret;
}

static int truly_tdo_hd0499k_get_power(struct lcd_device *ld)
{
	struct truly_tdo_hd0499k *lcd = lcd_get_data(ld);

	return lcd->power;
}


static struct lcd_ops truly_tdo_hd0499k_lcd_ops = {
	.set_power = truly_tdo_hd0499k_set_power,
	.get_power = truly_tdo_hd0499k_get_power,
};


static void truly_tdo_hd0499k_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct truly_tdo_hd0499k *lcd = dev_get_drvdata(&dsim_dev->dev);

	/* lcd power on */
#if 0
	if (power)
		truly_tdo_hd0499k_regulator_enable(lcd);
	else
		truly_tdo_hd0499k_regulator_disable(lcd);

#endif
	if (lcd->ddi_pd->power_on)
		lcd->ddi_pd->power_on(lcd->ld, power);
	/* lcd reset */
	if (lcd->ddi_pd->reset)
		lcd->ddi_pd->reset(lcd->ld);
}

static void truly_tdo_hd0499k_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_tdo_hd0499k *lcd = dev_get_drvdata(&dsim_dev->dev);
	truly_tdo_hd0499k_panel_init(lcd);
	truly_tdo_hd0499k_sleep_out(lcd);
	mdelay(120);
	truly_tdo_hd0499k_display_on(lcd);
	mdelay(10);
	lcd->power = FB_BLANK_UNBLANK;
}

static int truly_tdo_hd0499k_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_tdo_hd0499k *lcd;
	printk("----------------%s\n", __func__);
	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct truly_tdo_hd0499k), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate truly_tdo_hd0499k structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);
#if 0
	lcd->lcd_vcc_reg = regulator_get(NULL, "vlcd");
	if (IS_ERR(lcd->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(lcd->lcd_vcc_reg);
	}
#endif

	lcd->ld = lcd_device_register("truly_tdo_hd0499k", lcd->dev, lcd,
			&truly_tdo_hd0499k_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);

	dev_dbg(lcd->dev, "probed truly_tdo_hd0499k panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static int truly_tdo_hd0499k_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_tdo_hd0499k *lcd = dev_get_drvdata(&dsim_dev->dev);

	truly_tdo_hd0499k_sleep_in(lcd);
	msleep(lcd->ddi_pd->power_off_delay);
	truly_tdo_hd0499k_display_off(lcd);

	//truly_tdo_hd0499k_regulator_disable(lcd);

	return 0;
}

static int truly_tdo_hd0499k_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_tdo_hd0499k *lcd = dev_get_drvdata(&dsim_dev->dev);

	truly_tdo_hd0499k_sleep_out(lcd);
	msleep(lcd->ddi_pd->power_on_delay);

	//truly_tdo_hd0499k_regulator_enable(lcd);
	truly_tdo_hd0499k_set_sequence(dsim_dev);

	return 0;
}
#else
#define truly_tdo_hd0499k_suspend		NULL
#define truly_tdo_hd0499k_resume		NULL
#endif

static struct mipi_dsim_lcd_driver truly_tdo_hd0499k_dsim_ddi_driver = {
	.name = "truly_tdo_hd0499k-lcd",
	.id = 0,

	.power_on = truly_tdo_hd0499k_power_on,
	.set_sequence = truly_tdo_hd0499k_set_sequence,
	.probe = truly_tdo_hd0499k_probe,
	.suspend = truly_tdo_hd0499k_suspend,
	.resume = truly_tdo_hd0499k_resume,
};

static int truly_tdo_hd0499k_init(void)
{
	mipi_dsi_register_lcd_driver(&truly_tdo_hd0499k_dsim_ddi_driver);
	return 0;
}

static void truly_tdo_hd0499k_exit(void)
{
	return;
}

module_init(truly_tdo_hd0499k_init);
module_exit(truly_tdo_hd0499k_exit);

MODULE_DESCRIPTION("truly_tdo_hd0499k lcd driver");
MODULE_LICENSE("GPL");
