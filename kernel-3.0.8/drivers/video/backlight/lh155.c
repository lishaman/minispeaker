/* linux/drivers/video/exynos/lh155.c
 *
 * MIPI-DSI based lh155 AMOLED lcd 4.65 inch panel driver.
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

struct lh155 {
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

struct dsi_cmd_packet lh155_cmd_list[] = {
	{0x39, 0x03, 0x00, {0xf1, 0x5a, 0x5a}},
	{0x39, 0x12, 0x0, {0xf2, 0x00, 0xd7, 0x03, 0x22, 0x23, 0x00, 0x01, 0x01, 0x12, 0x01, 0x08, 0x57, 0x00, 0x00, 0xd7, 0x22, 0x23}},
	{0x39, 0xf, 0x0, {0xf4, 0x07, 0x00, 0x00, 0x00, 0x21, 0x4f, 0x01, 0x02, 0x2a, 0x66, 0x02, 0x2a, 0x00, 0x02}},
	{0x39, 0xb, 0x0,  {0xf5, 0x00, 0x38, 0x5c, 0x00, 0x00, 0x19, 0x00, 0x00, 0x04, 0x04}},
	{0x39, 0xa, 0x0, {0xf6, 0x03, 0x01, 0x06, 0x00, 0x0a, 0x04, 0x0a, 0xf4, 0x06}},
	{0x39, 0x2, 0x0, {0xf7, 0x40}},
	{0x39, 0x3, 0x0, {0xf8, 0x33, 0x00}},
	{0x39, 0x2, 0x0, {0x0, 0xf9, 0x00}},
	{0x39, 0x1d, 0x0, {0xfa, 0x00, 0x04, 0x0e, 0x03, 0x0e, 0x20, 0x2a, 0x30, 0x38, 0x3a,
							0x34, 0x37, 0x44, 0x3b, 0x00, 0x0b, 0x00, 0x07, 0x0b, 0x17, 0x1b,
							0x1a, 0x22, 0x2b, 0x34, 0x40, 0x50, 0x3f}},
	{0x39, 0x1d, 0x0, {0xfa, 0x00, 0x1b, 0x11, 0x03, 0x08, 0x16, 0x1d, 0x20, 0x29, 0x2c,
							0x29, 0x2d, 0x3d, 0x3c, 0x00, 0x0b, 0x12, 0x0e, 0x11, 0x1e, 0x22,
							0x25, 0x2e, 0x39, 0x40, 0x48, 0x40, 0x3f}},
	{0x39, 0xf, 0x0, {0xfa, 0x00, 0x27, 0x12, 0x02, 0x04, 0x0f, 0x15, 0x17, 0x21, 0x27,
							0x24, 0x29, 0x31, 0x3c}},
	{0x39, 0x10, 0x0, {0xfa, 0x00, 0x0c, 0x1e, 0x11, 0x17, 0x21, 0x27, 0x2b, 0x37, 0x43,
							0x49, 0x4f, 0x59, 0x3e, 0x00}},
	//{0x39, 0x1, 0x0, {0x11}},
	{0x39, 0x2, 0x0, {0x36, 0xd8}},
	{0x39, 0x2, 0x0, {0x3a, 0x07}},
	//{0x39, 0x1, 0x0, {0x29}},
	{0x39, 0x5, 0x0, {0x2a, 0x00, 0x00, (239 & 0xff00) >> 8, 239 & 0xff}},
	{0x39, 0x5, 0x0, {0x0, 0x2b, 0x00, 0x00, (239 & 0xff00) >> 8, (239 & 0xff)}},
	{0x39, 0x1, 0x0, {0x2c}},
};

static void lh155_regulator_enable(struct lh155 *lcd)
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

static void lh155_regulator_disable(struct lh155 *lcd)
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

static void lh155_sleep_in(struct lh155 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x10, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void lh155_sleep_out(struct lh155 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x11, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void lh155_display_on(struct lh155 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x29, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void lh155_display_off(struct lh155 *lcd)
{
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_cmd_packet data_to_send = {0x05, 0x28, 0x00};

	ops->cmd_write(lcd_to_master(lcd), data_to_send);
}

static void lh155_panel_init(struct lh155 *lcd)
{
	int  i;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	for(i = 0; i < ARRAY_SIZE(lh155_cmd_list); i++) {
		ops->cmd_write(dsi,  lh155_cmd_list[i]);
	}
}

static int lh155_set_power(struct lcd_device *ld, int power)
{
	int ret = 0;
#if 0
	struct lh155 *lcd = lcd_get_data(ld);
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

static int lh155_get_power(struct lcd_device *ld)
{
	struct lh155 *lcd = lcd_get_data(ld);

	return lcd->power;
}


static struct lcd_ops lh155_lcd_ops = {
	.set_power = lh155_set_power,
	.get_power = lh155_get_power,
};


static void lh155_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct lh155 *lcd = dev_get_drvdata(&dsim_dev->dev);

	/* lcd power on */
#if 0
	if (power)
		lh155_regulator_enable(lcd);
	else
		lh155_regulator_disable(lcd);

#endif
	if (lcd->ddi_pd->power_on)
		lcd->ddi_pd->power_on(lcd->ld, power);
	/* lcd reset */
	if (lcd->ddi_pd->reset)
		lcd->ddi_pd->reset(lcd->ld);
}

static void lh155_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct lh155 *lcd = dev_get_drvdata(&dsim_dev->dev);
	lh155_panel_init(lcd);
	lh155_sleep_out(lcd);
	msleep(120);
	lh155_display_on(lcd);
	msleep(10);
	lcd->power = FB_BLANK_UNBLANK;
}

static int lh155_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct lh155 *lcd;
	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct lh155), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate lh155 structure.\n");
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

	lcd->ld = lcd_device_register("lh155", lcd->dev, lcd,
			&lh155_lcd_ops);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);

	dev_dbg(lcd->dev, "probed lh155 panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static int lh155_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct lh155 *lcd = dev_get_drvdata(&dsim_dev->dev);

	lh155_sleep_in(lcd);
	msleep(lcd->ddi_pd->power_off_delay);
	lh155_display_off(lcd);

	//lh155_regulator_disable(lcd);

	return 0;
}

static int lh155_resume(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct lh155 *lcd = dev_get_drvdata(&dsim_dev->dev);

	lh155_sleep_out(lcd);
	msleep(lcd->ddi_pd->power_on_delay);

	//lh155_regulator_enable(lcd);
	lh155_set_sequence(dsim_dev);

	return 0;
}
#else
#define lh155_suspend		NULL
#define lh155_resume		NULL
#endif

static struct mipi_dsim_lcd_driver lh155_dsim_ddi_driver = {
	.name = "lh155-lcd",
	.id = 0,

	.power_on = lh155_power_on,
	.set_sequence = lh155_set_sequence,
	.probe = lh155_probe,
	.suspend = lh155_suspend,
	.resume = lh155_resume,
};

static int lh155_init(void)
{
	mipi_dsi_register_lcd_driver(&lh155_dsim_ddi_driver);
	return 0;
}

static void lh155_exit(void)
{
	return;
}

module_init(lh155_init);
module_exit(lh155_exit);

MODULE_DESCRIPTION("lh155 lcd driver");
MODULE_LICENSE("GPL");
