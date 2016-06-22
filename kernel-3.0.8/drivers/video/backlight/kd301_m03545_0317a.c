#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/regulator/consumer.h>
#include <soc/gpio.h>

#include <linux/kd301_m03545_0317a.h>

struct kd301_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_kd301_data *pdata;
	struct regulator *lcd_vcc_reg;
};

#define kd301_clear_pin(pin) gpio_set_value(kdata->pdata->gpio_##pin, 0)
#define kd301_set_pin(pin) gpio_set_value(kdata->pdata->gpio_##pin, 1)

static int __lcd_write_cmd(struct kd301_data *kdata, unsigned char cmd)
{
	int i;

	kd301_clear_pin(lcd_spi_ce);
	udelay(2);
	kd301_clear_pin(lcd_spi_clk);
	udelay(5);
	kd301_clear_pin(lcd_spi_dt);
	udelay(5);
	kd301_set_pin(lcd_spi_clk);
	udelay(5);
	for( i = 0; i < 8 ;i++ )
	{
		udelay(5);
		kd301_clear_pin(lcd_spi_clk);
		udelay(5);
		if( cmd & 0x80 )
			kd301_set_pin(lcd_spi_dt);
		else
			kd301_clear_pin(lcd_spi_dt);

		udelay( 5 );
		kd301_set_pin(lcd_spi_clk);
		udelay( 5 );
		cmd <<= 1;
	}
	kd301_set_pin(lcd_spi_ce);
	udelay(5);
	return 0;
}

#define lcd_write_cmd(cmd) __lcd_write_cmd(kdata, (cmd))

static int __lcd_write_data(struct kd301_data *kdata, unsigned char data)
{
	int i;

	kd301_clear_pin(lcd_spi_ce);
	udelay(2);
	kd301_clear_pin(lcd_spi_clk);
	udelay(5);
	kd301_set_pin(lcd_spi_dt);
	udelay(5);
	kd301_set_pin(lcd_spi_clk);
	udelay(5);
	for( i = 0; i < 8 ;i++ )
	{
		udelay(5);
		kd301_clear_pin(lcd_spi_clk);
		udelay(5);
		if( data & 0x80 )
			kd301_set_pin(lcd_spi_dt);
		else
			kd301_clear_pin(lcd_spi_dt);

		udelay(5);
		kd301_set_pin(lcd_spi_clk);
		udelay( 5 );
		data <<= 1;
	}
	kd301_set_pin(lcd_spi_ce);
	udelay(2);
	return 0;
}

#define lcd_write_data(d) __lcd_write_data(kdata, (d))

static void kd301_pin_init(struct kd301_data *kdata)
{
	lcd_write_cmd(0x01);

	lcd_write_cmd(0xC0);
	lcd_write_data(0x15);  //lcd_write_data(0x15);
	lcd_write_data(0x15); //lcd_write_data(0x15);

	lcd_write_cmd(0xC1);
	lcd_write_data(0x45);
	lcd_write_data(0x07);

	lcd_write_cmd(0xC5);
	lcd_write_data(0x00);
	lcd_write_data(0x42);
	lcd_write_data(0x80);

	lcd_write_cmd(0xC2);
	lcd_write_data(0x33);

	lcd_write_cmd(0xB1);
	lcd_write_data(0xD0);
	lcd_write_data(0x11);

	lcd_write_cmd(0xB4);
	lcd_write_data(0x02);

	lcd_write_cmd(0xB6);
	lcd_write_data(0x00);
	lcd_write_data(0x22); //lcd_write_data(0x02);
	lcd_write_data(0x3B);

	lcd_write_cmd(0xB7);
	lcd_write_data(0x07);//07

	lcd_write_cmd(0xF0);
	lcd_write_data(0x36);
	lcd_write_data(0xA5);
	lcd_write_data(0xD3);

	lcd_write_cmd(0xE5);
	lcd_write_data(0x80);

	lcd_write_cmd(0xE5);
	lcd_write_data(0x01);

	lcd_write_cmd(0xB3);
	lcd_write_data(0x00);

	lcd_write_cmd(0xE5);
	lcd_write_data(0x00);

	lcd_write_cmd(0xF0);
	lcd_write_data(0x36);
	lcd_write_data(0xA5);
	lcd_write_data(0x53);

	lcd_write_cmd(0xE0);
	lcd_write_data(0x13);
	lcd_write_data(0x36);
	lcd_write_data(0x21);
	lcd_write_data(0x00);
	lcd_write_data(0x00);
	lcd_write_data(0x00);
	lcd_write_data(0x13);
	lcd_write_data(0x36);
	lcd_write_data(0x21);
	lcd_write_data(0x00);
	lcd_write_data(0x04); //lcd_write_data(0x04);
	lcd_write_data(0x04); //lcd_write_data(0x04);

	lcd_write_cmd(0x36);
	lcd_write_data(0x08);

	lcd_write_cmd(0xEE);
	lcd_write_data(0x00);

	lcd_write_cmd(0x3A);
	lcd_write_data(0x66);

	lcd_write_cmd(0xB0);
	lcd_write_data(0x86);
	lcd_write_cmd(0xB6);
	lcd_write_data(0x32);

	lcd_write_cmd(0x20);

	lcd_write_cmd(0x11);
	udelay(110);

	lcd_write_cmd(0x29);
	lcd_write_cmd(0x2C);
}

static void kd301_hard_reset(struct kd301_data *kdata)
{
	kd301_set_pin(lcd_reset);
	msleep(15);
	kd301_clear_pin(lcd_reset);
	msleep(15);
	kd301_set_pin(lcd_reset);
}

static void kd301_panel_init(struct kd301_data *kdata)
{
	kd301_hard_reset(kdata);
	kd301_pin_init(kdata);
}

static void kd301_on(struct kd301_data *kdata) {
	if (!IS_ERR(kdata->lcd_vcc_reg)) {
		if (regulator_is_enabled(kdata->lcd_vcc_reg))
			goto ok;
		else {
			regulator_enable(kdata->lcd_vcc_reg);
			udelay(100);
		}
	}

	kd301_panel_init(kdata);

ok:
	kdata->lcd_power = 1;
}

static void kd301_off(struct kd301_data *kdata) {
	if (!IS_ERR(kdata->lcd_vcc_reg)) {
		if (regulator_is_enabled(kdata->lcd_vcc_reg)) {
			regulator_disable(kdata->lcd_vcc_reg);
		} else if (kdata->lcd_power == 0)
			return;
	}

	kd301_clear_pin(lcd_reset);
	msleep(15);

	kdata->lcd_power = 0;
}

static int kd301_set_power(struct lcd_device *lcd, int power) {
	struct kd301_data *kdata = lcd_get_data(lcd);

	if (!power && !(kdata->lcd_power)) {
                kd301_on(kdata);
        } else if (power && (kdata->lcd_power)) {
                kd301_off(kdata);
        }
	return 0;
}

static int kd301_get_power(struct lcd_device *lcd) {
	struct kd301_data *kdata= lcd_get_data(lcd);

	return kdata->lcd_power;
}

static int kd301_set_mode(struct lcd_device *lcd, struct fb_videomode *mode) {
	return 0;
}

static struct lcd_ops kd301_ops = {
	.set_power = kd301_set_power,
	.get_power = kd301_get_power,
	.set_mode = kd301_set_mode,
};

static int kd301_probe(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	struct platform_kd301_data *pdata = dev->platform_data;
	struct kd301_data *kdata;
	int ret;

	if (!gpio_is_valid(pdata->gpio_lcd_spi_dr) ||
		!gpio_is_valid(pdata->gpio_lcd_spi_dt) ||
		!gpio_is_valid(pdata->gpio_lcd_spi_clk) ||
		!gpio_is_valid(pdata->gpio_lcd_spi_ce) ||
		!gpio_is_valid(pdata->gpio_lcd_reset)) {
		return -EINVAL;
	}

	kdata = kzalloc(sizeof(struct kd301_data), GFP_KERNEL);
	if (!kdata)
		return -ENOMEM;

	kdata->pdata = pdata;

	dev_set_drvdata(dev, kdata);

	if (pdata->v33_reg_name) {
		kdata->lcd_vcc_reg = regulator_get(NULL, pdata->v33_reg_name);
		if (IS_ERR(kdata->lcd_vcc_reg)) {
			dev_err(dev, "failed to get V3.3 regulator\n");
			ret = PTR_ERR(kdata->lcd_vcc_reg);
			goto free_kdata;
		}
	}

	ret = gpio_request(pdata->gpio_lcd_spi_dr, "lcd_spi_dr");
	if (ret != 0) {
		dev_err(dev, "failed to request gpio lcd_spi_dr.\n");
		goto free_reg;
	}
	gpio_direction_output(pdata->gpio_lcd_spi_dr, 1);

	ret = gpio_request(pdata->gpio_lcd_spi_dt, "lcd_spi_dt");
	if (ret != 0) {
		dev_err(dev, "failed to request gpio lcd_spi_dt.\n");
		goto free_spi_dr;
	}
	gpio_direction_output(pdata->gpio_lcd_spi_dt, 1);

	ret = gpio_request(pdata->gpio_lcd_spi_clk, "lcd_spi_clk");
	if (ret != 0) {
		dev_err(dev, "failed to request gpio lcd_spi_clk.\n");
		goto free_spi_dt;
	}
	gpio_direction_output(pdata->gpio_lcd_spi_clk, 1);

	ret = gpio_request(pdata->gpio_lcd_spi_ce, "lcd_spi_ce");
	if (ret != 0) {
		dev_err(dev, "failed to request gpio lcd_spi_ce.\n");
		goto free_spi_clk;
	}
	gpio_direction_output(pdata->gpio_lcd_spi_ce, 1);

	ret = gpio_request(pdata->gpio_lcd_reset, "lcd_reset");
	if (ret != 0) {
		dev_err(dev, "failed to request gpio lcd_reset.\n");
		goto free_spi_ce;
	}
	gpio_direction_output(pdata->gpio_lcd_reset, 1);

	kd301_on(kdata);

	kdata->lcd = lcd_device_register("kd301-lcd", dev, kdata, &kd301_ops);
	if (IS_ERR(kdata->lcd)) {
		ret = PTR_ERR(kdata->lcd);
		dev_err(dev, "failed to register lcd device\n");
		goto free_lcd_reset;
	} else
		dev_info(dev, "register lcd device success.\n");

	return 0;

free_lcd_reset:
	gpio_free(pdata->gpio_lcd_reset);

free_spi_ce:
	gpio_free(pdata->gpio_lcd_spi_ce);

free_spi_clk:
	gpio_free(pdata->gpio_lcd_spi_clk);

free_spi_dt:
	gpio_free(pdata->gpio_lcd_spi_dt);

free_spi_dr:
	gpio_free(pdata->gpio_lcd_spi_dr);

free_reg:
	regulator_put(kdata->lcd_vcc_reg);

free_kdata:
	kfree(kdata);

	return ret;
}

static int __devinit
kd301_remove(struct platform_device *pdev) {
	struct kd301_data *kdata = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(kdata->lcd);
	kd301_off(kdata);

	if (!IS_ERR(kdata->lcd_vcc_reg))
		regulator_put(kdata->lcd_vcc_reg);

	gpio_free(kdata->pdata->gpio_lcd_reset);
	gpio_free(kdata->pdata->gpio_lcd_spi_ce);
	gpio_free(kdata->pdata->gpio_lcd_spi_clk);
	gpio_free(kdata->pdata->gpio_lcd_spi_dt);
	gpio_free(kdata->pdata->gpio_lcd_spi_dr);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(kdata);

	return 0;
}

#ifdef CONFIG_PM
static int kd301_suspend(struct platform_device *pdev,
			pm_message_t state)
{
	return 0;
}

static int kd301_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define kd301_suspend	NULL
#define kd301_resume	NULL
#endif

static struct platform_driver kd301_driver = {
	.driver = {
		.name = "kd301_m03545_0317a",
		.owner = THIS_MODULE,
	},
	.probe = kd301_probe,
	.remove = kd301_remove,
	.suspend = kd301_suspend,
	.resume = kd301_resume,
};

static int __init kd301_init(void) {
	return platform_driver_register(&kd301_driver);
}

static void __exit kd301_exit(void) {
	platform_driver_unregister(&kd301_driver);
}

module_init(kd301_init);
module_exit(kd301_exit);

MODULE_DESCRIPTION("KD301_M03545_0317A lcd driver");
MODULE_LICENSE("GPL");
