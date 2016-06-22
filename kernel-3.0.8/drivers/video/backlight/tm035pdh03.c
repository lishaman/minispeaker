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

#include <linux/tm035pdh03.h>

struct tm035_data {
	int lcd_power;
	struct lcd_device *lcd;
	struct platform_tm035_data *pdata;
	struct regulator *lcd_vcc_reg;
};

#define tm035_clear_pin(pin) gpio_set_value(kdata->pdata->gpio_##pin, 0)
#define tm035_set_pin(pin) gpio_set_value(kdata->pdata->gpio_##pin, 1)

static int __lcd_write_cmd(struct tm035_data *kdata, unsigned char cmd)
{
	int i;

	tm035_clear_pin(lcd_spi_ce);
	udelay(2);
	tm035_clear_pin(lcd_spi_clk);
	udelay(5);
	tm035_clear_pin(lcd_spi_dt);
	udelay(5);
	tm035_set_pin(lcd_spi_clk);
	udelay(5);
	for(i = 0; i < 8; i++) {
		udelay(5);
		tm035_clear_pin(lcd_spi_clk);
		udelay(5);
		if(cmd & 0x80)
			tm035_set_pin(lcd_spi_dt);
		else
			tm035_clear_pin(lcd_spi_dt);

		udelay(5);
		tm035_set_pin(lcd_spi_clk);
		udelay(5);
		cmd <<= 1;
	}
	tm035_set_pin(lcd_spi_ce);
	udelay(5);
	return 0;
}

#define lcd_write_cmd(cmd) __lcd_write_cmd(kdata, (cmd))

static int __lcd_write_data(struct tm035_data *kdata, unsigned char data)
{
	int i;

	tm035_clear_pin(lcd_spi_ce);
	udelay(2);
	tm035_clear_pin(lcd_spi_clk);
	udelay(5);
	tm035_set_pin(lcd_spi_dt);
	udelay(5);
	tm035_set_pin(lcd_spi_clk);
	udelay(5);
	for(i = 0; i < 8; i++) {
		udelay(5);
		tm035_clear_pin(lcd_spi_clk);
		udelay(5);
		if(data & 0x80)
			tm035_set_pin(lcd_spi_dt);
		else
			tm035_clear_pin(lcd_spi_dt);

		udelay(5);
		tm035_set_pin(lcd_spi_clk);
		udelay(5);
		data <<= 1;
	}
	tm035_set_pin(lcd_spi_ce);
	udelay(2);
	return 0;
}

#define lcd_write_data(d) __lcd_write_data(kdata, (d))

static void tm035_pin_init(struct tm035_data *kdata)
{
	lcd_write_cmd(0x11);
	mdelay(150);

	lcd_write_cmd(0xB9);
	lcd_write_data(0xFF);
	lcd_write_data(0x83);
	lcd_write_data(0x57);

	mdelay(10);

	lcd_write_cmd(0xB6);
//	lcd_write_data(0x4D);
//	lcd_write_data(0x4A);
	lcd_write_data(0x55);

	lcd_write_cmd(0x3A);
	lcd_write_data(0x67);	//diff	0x05 and 0x60

	lcd_write_cmd(0xCC);
	lcd_write_data(0x09);	//diff 0x09 and 0xB0

	lcd_write_cmd(0xB3);
	lcd_write_data(0x43);	//not set bypass(bit6).
	lcd_write_data(0x08);
	lcd_write_data(0x06);	//set Hsync falling to first valid data
	lcd_write_data(0x06);	//set Vsync ....

	lcd_write_cmd(0xB1);             //
	lcd_write_data(0x00);                //don't into deep standby mode
	lcd_write_data(0x14);                //BT  //15
	lcd_write_data(0x1C);                //VSPR
	lcd_write_data(0x1C);                //VSNR
	lcd_write_data(0x83);                //AP
	lcd_write_data(0x48);                //FS

	lcd_write_cmd(0x53);
	lcd_write_data(0x24);

	lcd_write_cmd(0x51);
	lcd_write_data(0xff);

	lcd_write_cmd(0x52);
	lcd_write_data(0xff);

	lcd_write_cmd(0xC0);             //STBA
	lcd_write_data(0x70);                //OPON
	lcd_write_data(0x50);                //OPON
	lcd_write_data(0x01);                //
	lcd_write_data(0x3C);                //
	lcd_write_data(0xC8);                //
	lcd_write_data(0x08);                //GEN

	lcd_write_cmd(0xB4);             //
	lcd_write_data(0x02);                //NW
	lcd_write_data(0x40);                //RTN
	lcd_write_data(0x00);                //DIV
	lcd_write_data(0x2A);                //DUM
	lcd_write_data(0x2A);                //DUM
	lcd_write_data(0x0D);                //GDON
	lcd_write_data(0x47);                //GDOFF

	lcd_write_cmd(0xE0);             //
	lcd_write_data(0x02);                //0
	lcd_write_data(0x04);                //1
	lcd_write_data(0x0a);                //2
	lcd_write_data(0X18);                //4
	lcd_write_data(0x28);                //6
	lcd_write_data(0x38);                //13
	lcd_write_data(0x42);                //20
	lcd_write_data(0x4A);                //27
	lcd_write_data(0x4D);                //36
	lcd_write_data(0x46);                //43
	lcd_write_data(0x42);                //50
	lcd_write_data(0x37);                //57
	lcd_write_data(0x33);                //59
	lcd_write_data(0x2C);                //61
	lcd_write_data(0x29);                //62
	lcd_write_data(0x10);                //63
	lcd_write_data(0x02);                //0
	lcd_write_data(0x04);                //1
	lcd_write_data(0x0a);                //2
	lcd_write_data(0X12);                //4
	lcd_write_data(0x27);                //6
	lcd_write_data(0x39);                //13
	lcd_write_data(0x43);                //20
	lcd_write_data(0x4A);                //27
	lcd_write_data(0x4F);                //36
	lcd_write_data(0x48);                //43
	lcd_write_data(0x42);                //50
	lcd_write_data(0x37);                //57
	lcd_write_data(0x35);                //59
	lcd_write_data(0x2E);                //61
	lcd_write_data(0x2B);                //62
	lcd_write_data(0x10);                //63
	lcd_write_data(0x00);                //33
	lcd_write_data(0x01);                //34

	lcd_write_cmd(0x29);             // Display On
	mdelay(10);
	lcd_write_cmd(0x2C);
	mdelay(10);
}

static void tm035_hard_reset(struct tm035_data *kdata)
{
	tm035_set_pin(lcd_reset);
	mdelay(10);
	tm035_set_pin(lcd_reset);
	mdelay(30);
	tm035_set_pin(lcd_reset);
	mdelay(100);
}

static void tm035_panel_init(struct tm035_data *kdata)
{
	tm035_hard_reset(kdata);
	tm035_pin_init(kdata);
}

static void tm035_on(struct tm035_data *kdata)
{
	if (!IS_ERR(kdata->lcd_vcc_reg)) {
		if (regulator_is_enabled(kdata->lcd_vcc_reg))
			goto ok;
		else {
			regulator_enable(kdata->lcd_vcc_reg);
			udelay(100);
		}
	}
	tm035_panel_init(kdata);
ok:
	kdata->lcd_power = 1;
}

static void tm035_off(struct tm035_data *kdata) {
	if (!IS_ERR(kdata->lcd_vcc_reg)) {
		if (regulator_is_enabled(kdata->lcd_vcc_reg)) {
			regulator_disable(kdata->lcd_vcc_reg);
		} else if (kdata->lcd_power == 0)
			return;
	}

	tm035_clear_pin(lcd_reset);
	msleep(15);

	kdata->lcd_power = 0;
}

static int tm035_set_power(struct lcd_device *lcd, int power) {
	struct tm035_data *kdata = lcd_get_data(lcd);

	if (!power && !(kdata->lcd_power)) {
                tm035_on(kdata);
        } else if (power && (kdata->lcd_power)) {
                tm035_off(kdata);
        }
	return 0;
}

static int tm035_get_power(struct lcd_device *lcd)
{
	struct tm035_data *kdata= lcd_get_data(lcd);
	return kdata->lcd_power;
}

static int tm035_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops tm035_ops = {
	.set_power = tm035_set_power,
	.get_power = tm035_get_power,
	.set_mode = tm035_set_mode,
};

static int tm035_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct platform_tm035_data *pdata = dev->platform_data;
	struct tm035_data *kdata;
	int ret;

	if (!gpio_is_valid(pdata->gpio_lcd_spi_dr) ||
		!gpio_is_valid(pdata->gpio_lcd_spi_dt) ||
		!gpio_is_valid(pdata->gpio_lcd_spi_clk) ||
		!gpio_is_valid(pdata->gpio_lcd_spi_ce) ||
		!gpio_is_valid(pdata->gpio_lcd_reset)) {
		return -EINVAL;
	}

	kdata = kzalloc(sizeof(struct tm035_data), GFP_KERNEL);
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

	tm035_on(kdata);
	
	kdata->lcd = lcd_device_register("tm035-lcd", dev, kdata, &tm035_ops);
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

static int __devinit tm035_remove(struct platform_device *pdev)
{
	struct tm035_data *kdata = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(kdata->lcd);
	tm035_off(kdata);

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
static int tm035_suspend(struct platform_device *pdev,
			pm_message_t state)
{
	return 0;
}

static int tm035_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define tm035_suspend	NULL
#define tm035_resume	NULL
#endif

static struct platform_driver tm035_driver = {
	.driver = {
		.name = "tm035pdh03",
		.owner = THIS_MODULE,
	},
	.probe = tm035_probe,
	.remove = tm035_remove,
	.suspend = tm035_suspend,
	.resume = tm035_resume,
};

static int __init tm035_init(void) {
	return platform_driver_register(&tm035_driver);
}

static void __exit tm035_exit(void) {
	platform_driver_unregister(&tm035_driver);
}

module_init(tm035_init);
module_exit(tm035_exit);

MODULE_DESCRIPTION("TM035PDH03 lcd driver");
MODULE_LICENSE("GPL");
