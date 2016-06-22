/*
 * kernel/drivers/video/backlight/panel_tl2796.c
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * tl2796 panel support, it's suitable for the lcd which drived by tl2796 ic
 * tl2796 via spi interface to control, using RGB interface.
 * 16/18/24-bit RGB Interface(VSYNC, HSYNC, ENABLE, DOTCLK, DB[23:0]) 
 * this file default to 24-bit.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/lcd.h>
#include <linux/backlight.h>

#include <linux/kthread.h>

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		255
#define power_is_on(pwr)	((pwr) <= FB_BLANK_NORMAL)

struct specific_tl2796 {
  const char *ld_name;			/* lcd device name  */
  const char *bd_name;			/* backlight device name  */
  int lcd_reset;				/* lcd reset pin */
  int spi_cs;					/* spi cs pin */
  int upper_margin;				/* see struct fb_videomode */
  int lower_margin;				/* 4 >= lower_margin <= 31 */
  int vsync;					/* 4 >= upper_margin + vsync <= 31 */
};

/** 
 * @spi: TL2796 using spi interface to control lcd panel state.
 * @power: Indicate the current power state of this lcd.
 * @lcd_reset: lcd reset pin
 */
struct tl2796 {
  struct spi_device *spi;
  struct lcd_platform_data *lcd_pd;
  struct specific_tl2796 *lcd_spec;
  struct lcd_device* ld;
  struct backlight_device *bd;
  struct device *dev;
  int lcd_reset;
  int spi_cs;
  int power;
};

static u8 tl2796_300[] = {0x00, 0x00, 0x00, 0x24, 0x24, 0x1E, 0x3F, 0x00, 0x00, 0x00, 0x23, 0x24, 0x1E, 0x3D, 0x00, 0x00, 0x00, 0x22, 0x22, 0x1A, 0x56};
static u8 tl2796_280[] = {0x00, 0x00, 0x00, 0x24, 0x24, 0x1F, 0x3D, 0x00, 0x00, 0x00, 0x23, 0x24, 0x1F, 0x3B, 0x00, 0x00, 0x00, 0x22, 0x21, 0x1B, 0x54};
static u8 tl2796_260[] = {0x00, 0x00, 0x00, 0x23, 0x24, 0x20, 0x3B, 0x00, 0x00, 0x00, 0x22, 0x24, 0x20, 0x39, 0x00, 0x00, 0x00, 0x22, 0x21, 0x1C, 0x51};
static u8 tl2796_240[] = {0x00, 0x00, 0x00, 0x23, 0x24, 0x21, 0x39, 0x00, 0x00, 0x00, 0x23, 0x25, 0x20, 0x37, 0x00, 0x00, 0x00, 0x22, 0x22, 0x1C, 0x4F};
static u8 tl2796_220[] = {0x00, 0x00, 0x0B, 0x25, 0x24, 0x21, 0x37, 0x00, 0x00, 0x00, 0x23, 0x25, 0x21, 0x35, 0x00, 0x00, 0x11, 0x22, 0x23, 0x1D, 0x4B};
static u8 tl2796_200[] = {0x00, 0x00, 0x11, 0x25, 0x24, 0x22, 0x34, 0x00, 0x00, 0x00, 0x23, 0x25, 0x22, 0x32, 0x00, 0x00, 0x11, 0x23, 0x22, 0x1E, 0x48};
static u8 tl2796_180[] = {0x00, 0x00, 0x11, 0x24, 0x25, 0x22, 0x32, 0x00, 0x00, 0x00, 0x23, 0x25, 0x22, 0x30, 0x00, 0x00, 0x11, 0x23, 0x23, 0x1E, 0x45};
static u8 tl2796_160[] = {0x00, 0x00, 0x17, 0x25, 0x25, 0x22, 0x2F, 0x00, 0x00, 0x00, 0x23, 0x26, 0x22, 0x22, 0x00, 0x00, 0x14, 0x24, 0x23, 0x1E, 0x43};
static u8 tl2796_140[] = {0x00, 0x00, 0x1A, 0x26, 0x25, 0x22, 0x2D, 0x00, 0x00, 0x00, 0x23, 0x25, 0x22, 0x2B, 0x00, 0x00, 0x1A, 0x24, 0x23, 0x1E, 0x3F};
static u8 tl2796_120[] = {0x00, 0x00, 0x1A, 0x26, 0x27, 0x24, 0x29, 0x00, 0x00, 0x00, 0x23, 0x26, 0x24, 0x28, 0x00, 0x00, 0x1A, 0x23, 0x25, 0x20, 0x3B};
static u8 tl2796_100[] = {0x00, 0x00, 0x1A, 0x26, 0x28, 0x23, 0x27, 0x00, 0x00, 0x00, 0x22, 0x27, 0x24, 0x25, 0x00, 0x00, 0x1A, 0x24, 0x26, 0x20, 0x37};
static u8 tl2796_40[] = {0x00, 0x00, 0x1A, 0x2C, 0x2A, 0x28, 0x19, 0x00, 0x00, 0x00, 0x22, 0x29, 0x28, 0x18, 0x00, 0x00, 0x1A, 0x29, 0x29, 0x25, 0x25};

static u8 tl2796_250[] = {0x00, 0x00, 0x00, 0x28, 0x25, 0x1F, 0x3B, 0x00, 0x00, 0x00, 0x26, 0x26, 0x20, 0x38, 0x00, 0x00, 0x00, 0x26, 0x24, 0x1C, 0x4F};
static u8 tl2796_110[] = {0x00, 0x00, 0x1A, 0x2A, 0x28, 0x24, 0x28, 0x00, 0x00, 0x00, 0x26, 0x28, 0x24, 0x27, 0x00, 0x00, 0x1A, 0x28, 0x26, 0x21, 0x38};
static u8 tl2796_105[] = {0x0F, 0x00, 0x00, 0x01, 0x07, 0x0D, 0x29, 0x00, 0x3F, 0x2C, 0x27, 0x24, 0x21, 0x2A, 0x0F, 0x3F, 0x0A, 0x07, 0x0A, 0xD, 0x37};
static u8 tl2796_60[] = {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x11, 0x00, 0x00, 0x00, 0x14, 0x1E, 0x1D, 0x14, 0x00, 0x00, 0x00, 0x1B, 0x1D, 0x1C, 0x1E};
static u8 tl2796_50[] = {0x00, 0x00, 0x1A, 0x29, 0x28, 0x27, 0x1B, 0x00, 0x00, 0x00, 0x21, 0x27, 0x27, 0x1A, 0x00, 0x00, 0x1A, 0x27, 0x26, 0x25, 0x29};

static int cd[12] = {0, 40, 120, 140, 180, 180, 200, 220, 240, 260, 280, 300}; //here
static const int  MAX_GAMMA_INDEX = sizeof(cd) / sizeof(int);

static unsigned int tl2796_gamma_reg[] = {
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 
};

/* 
 * Using SPI interface to write byte
 */
static inline void _tl2796_set(struct tl2796* lcd, int start_byte, int data) {
  u16 tx_buf = (start_byte << 8) | data;
  struct spi_message msg;
  struct spi_transfer xfer = {
	.len = 2,
	.tx_buf = &tx_buf,
  };
  
  spi_message_init(&msg);
  spi_message_add_tail(&xfer, &msg);
  
  spi_sync(lcd->spi, &msg);
}

/* 
 * Using SPI interface to read byte
 */
static inline int _tl2796_get(struct tl2796* lcd, int start_byte) {
  u16 tx_buf = (start_byte << 8) ;
  u16 rx_buf = 0;
  struct spi_message msg;
  struct spi_transfer xfer = {
	.len = 2,
	.tx_buf = &tx_buf,
	.rx_buf = &rx_buf, 
  };
  
  spi_message_init(&msg);
  spi_message_add_tail(&xfer, &msg);
  
  spi_sync(lcd->spi, &msg);

  return rx_buf & 0xff;
}

/* 
 * tl2796 communication functions
 */
static const int REG_S = 0x70;
static const int REG_R = 0x73;
static const int REG_W = 0x72;
static const int ID_R = 0x71;

static inline void _tl2796_set_register(struct tl2796* lcd, int reg) {
  _tl2796_set(lcd, REG_S, reg);
}

static inline void _tl2796_set_data(struct tl2796* lcd, int data) {
  _tl2796_set(lcd, REG_W, data);
}

static  void tl2796_write_register(struct tl2796* lcd, int reg, int data) {
  _tl2796_set_register(lcd, reg);
  _tl2796_set_data(lcd, data);
}

static  void tl2796_write_register_u16(struct tl2796* lcd, int reg, int data) {
  _tl2796_set_register(lcd, reg);
  _tl2796_set_data(lcd, data / 256);
  _tl2796_set_data(lcd, data % 256);
}

static  int tl2796_read_register(struct tl2796 *lcd, int reg) {  
  _tl2796_set_register(lcd, reg);
  return  _tl2796_get(lcd, REG_R);
}

static  int tl2796_read_id(struct tl2796 *lcd, int reg) {  
  _tl2796_set_register(lcd, reg);
  return  _tl2796_get(lcd, ID_R);
}

static void read_id(struct tl2796 *lcd){
  printk(KERN_ERR "ID:  0x00: 0x%x\n  0x01: 0x%x\n  0x02: 0x%x",
		 tl2796_read_id(lcd, 0x00),
		 tl2796_read_id(lcd, 0x01),
		 tl2796_read_id(lcd, 0x02));
}

static  int check_register(struct tl2796 *lcd , int reg, int data) {
  int ret =
	tl2796_read_register(lcd, reg);
  if (ret != data) {
	printk(KERN_ERR "TL2796: register 0x%x not match: data: 0x%x  ret: 0x%x \n", reg, data, ret);
  }
	
  return ret == data;
}

static void tl2796_gamma_ctl(struct tl2796 *lcd ,u8 *data_arr) {
  int i;
  /* Chosing this setting forever */
  tl2796_write_register(lcd, 0x39, 0x44);

  /* Now write gamma registers */
  for (i = 0; i < sizeof(tl2796_gamma_reg) / sizeof(int); i++) {
	tl2796_write_register(lcd, tl2796_gamma_reg[i], data_arr[i]);
  }
}

static int tl2796_gamma_setting(struct tl2796 *lcd, int idx) {
	printk(KERN_DEBUG "%s: gamma set to %d\n", __FUNCTION__, idx);

	if ((idx > MAX_GAMMA_INDEX) || (idx < 0))
		idx = 7;

	switch (cd[idx])
	{
		case	300 :	tl2796_gamma_ctl(lcd, tl2796_300);	break;
		case	280 :	tl2796_gamma_ctl(lcd, tl2796_280);	break;
		case	260 :	tl2796_gamma_ctl(lcd, tl2796_260);	break;
		case	250 :	tl2796_gamma_ctl(lcd, tl2796_250);	break;
		case	240 :	tl2796_gamma_ctl(lcd, tl2796_240);	break;
		case	220 :	tl2796_gamma_ctl(lcd, tl2796_220);	break;
		case	200 :	tl2796_gamma_ctl(lcd, tl2796_200);	break;
		case	180 :	tl2796_gamma_ctl(lcd, tl2796_180);	break;
		case	160 :	tl2796_gamma_ctl(lcd, tl2796_160);	break;
		case	140 :	tl2796_gamma_ctl(lcd, tl2796_140);	break;
		case	120 :	tl2796_gamma_ctl(lcd, tl2796_120);	break;
		case	110 :	tl2796_gamma_ctl(lcd, tl2796_110);	break;
		case	105 :	tl2796_gamma_ctl(lcd, tl2796_105);	break;
		case	100 :	tl2796_gamma_ctl(lcd, tl2796_100);	break;
		case	60	:	tl2796_gamma_ctl(lcd, tl2796_60);		break;
		case	50	:	tl2796_gamma_ctl(lcd, tl2796_50);		break;
		case	40	:	tl2796_gamma_ctl(lcd, tl2796_40);		break;
	}
	
	return 0;
}

static inline int brightness_to_index(int brightness) {
  return (brightness * 10) / 255; /* brightness / 25.5 */
}

static int tl2796_brightness_setting(struct tl2796 *lcd, int brightness) {
  int index = brightness_to_index(brightness);
  
  if ( brightness > MAX_BRIGHTNESS ||  brightness < MIN_BRIGHTNESS) {
	return 1;
  }
  
  /* NOTICE: tl2796 must be powered on */
  return tl2796_gamma_setting(lcd, index);
}

static int tl2796_power_off(struct tl2796 *lcd) {
  struct lcd_platform_data *pd = lcd->lcd_pd;
  /* display off */
  tl2796_write_register(lcd, 0x14, 0x00);
  /* wait for serveral frame */
  mdelay(60);
  /* stand by on */
  tl2796_write_register(lcd, 0x1d, 0xa1);
  mdelay(200);
  
  if (pd->power_on) {
	pd->power_on(lcd->ld, 0);
	mdelay(pd->power_off_delay);
  }
  return 0;
}

static int tl2796_power_on(struct tl2796 *lcd) {
	struct lcd_platform_data *pd = lcd->lcd_pd;
	struct specific_tl2796 *lcd_spec = (struct specific_tl2796 *)pd->pdata;
	
	int VFP = lcd_spec->lower_margin;
	int VBP = lcd_spec->upper_margin + lcd_spec->vsync;

	if (pd->power_on) {
	  pd->power_on(lcd->ld, 1);
	  mdelay(pd->power_on_delay);
	}

	printk(KERN_DEBUG " ****  tl2796_ldi_poweron. IN \n");
		
	//System Power On 1 (VBATT)
	//System Power On 2 (IOVCC)
	//System Power On 3 (VCI)
	//Wait for minimum 25ms

	gpio_direction_output(lcd->lcd_reset, 1);
	mdelay(25);
	gpio_direction_output(lcd->lcd_reset, 0);
	mdelay(1);
	gpio_direction_output(lcd->lcd_reset, 1);
	mdelay(20);
	
	// set panel condition
	tl2796_write_register(lcd, 0x31, 0x08);
	tl2796_write_register(lcd, 0x32, 0x14);
	tl2796_write_register(lcd, 0x30, 0x02);
	tl2796_write_register(lcd, 0x27, 0x01);
	
	// set display condition
	tl2796_write_register(lcd, 0x12, VBP);
	tl2796_write_register(lcd, 0x13, VFP);
	tl2796_write_register(lcd, 0x15, 0x00);
	tl2796_write_register(lcd, 0x16, 0x00);
	tl2796_write_register_u16(lcd, 0xEF, 0xD0E8);

	// Gamma Setting
	tl2796_gamma_setting(lcd, MAX_GAMMA_INDEX);

	//Analog poweron condition
	tl2796_write_register(lcd, 0x17, 0x22);
	tl2796_write_register(lcd, 0x18, 0x33);
	tl2796_write_register(lcd, 0x19, 0x03);
	tl2796_write_register(lcd, 0x1A, 0x01);
	tl2796_write_register(lcd, 0x22, 0xA4);
	tl2796_write_register(lcd, 0x23, 0x00);
	tl2796_write_register(lcd, 0x26, 0xA0);

	// STB off
	tl2796_write_register(lcd, 0x1D, 0xA0);
	mdelay(250);
	
	tl2796_write_register(lcd, 0x14, 0x03);

	printk(KERN_DEBUG "TL2796: Start check register\n");
	read_id(lcd);
/* 	// set panel condition */
	check_register(lcd, 0x31, 0x08);
	check_register(lcd, 0x32, 0x14);
	check_register(lcd, 0x30, 0x02);
	check_register(lcd, 0x27, 0x01);

/* 	// set display condition */
	check_register(lcd, 0x12, 0x08);
	check_register(lcd, 0x13, 0x08);
	check_register(lcd, 0x15, 0x00);
	check_register(lcd, 0x16, 0x00);
	
/* 	//Analog poweron condition */
	check_register(lcd, 0x17, 0x22);
	check_register(lcd, 0x18, 0x33);
	check_register(lcd, 0x19, 0x03);
	check_register(lcd, 0x1A, 0x01);
	check_register(lcd, 0x22, 0xA4);
	check_register(lcd, 0x23, 0x00);
	check_register(lcd, 0x26, 0xA0);

	check_register(lcd, 0x1D, 0xA0);
	check_register(lcd, 0x14, 0x03);
    printk(KERN_DEBUG "TL2796: End of check\n");

  return 0;
}

static int tl2796_power(struct tl2796 *lcd, int power) {
  int ret = 0;
  if (power_is_on(power) && !power_is_on(lcd->power))
	ret = tl2796_power_on(lcd);
  else if (!power_is_on(power) && power_is_on(lcd->power))
	ret = tl2796_power_off(lcd);
  
  if (!ret) 
	lcd->power = power;
  
  return ret;
}

/* lcd ops */

static int tl2796_set_power(struct lcd_device *ld, int power) {
  struct tl2796 *lcd = lcd_get_data(ld);

  if (power < FB_BLANK_UNBLANK || power > FB_BLANK_POWERDOWN) {
	dev_err(&ld->dev, "Not support %d mode, lcd blank mode should be %d to %d \n",
			power, FB_BLANK_UNBLANK, FB_BLANK_POWERDOWN);
	return -EINVAL;
  }
  
  return tl2796_power(lcd, power);  
}

static int tl2796_get_power(struct lcd_device *ld) {
  struct tl2796 *lcd = lcd_get_data(ld);

  return lcd->power;
}

static struct lcd_ops tl2796_lcd_ops = {
  .get_power = tl2796_get_power,
  .set_power = tl2796_set_power,  
};

/* brightness ops  */

static int tl2796_get_brightness(struct backlight_device *bd) {
  return bd->props.brightness;
}

static int tl2796_set_brightness(struct backlight_device *bd) {
  struct backlight_properties* bp= &bd->props;
  struct tl2796 *lcd = NULL;
  int ret = 0;

  if (bp->brightness > bp->max_brightness || 
	  bp->brightness < MIN_BRIGHTNESS) {
	dev_err(&bd->dev, "Backligt brightness should %d to %d\n", MIN_BRIGHTNESS, bp->max_brightness);
	return -EINVAL;
  }
  
  lcd = dev_get_drvdata(&bd->dev);

  

  ret = tl2796_brightness_setting(lcd, bp->brightness);
  if (ret) {
	ret = -EIO;
	dev_err(&bd->dev, "Lcd brightness setting failed,this should not be happened\n");
  } 
  
  return 0;
}

/* 
 *  test brightness, every 2 seconds changes one time. Run this following code to test.
 *  kthread_run(set_brightness_threadfun, lcd, lcd_spec->ld_name); 
 */

static int set_brightness_threadfun(void *data) {
  int i;
  struct tl2796 *lcd = (struct tl2796*)data;
  while (!kthread_should_stop()) {
	for (i = 0; i < MAX_GAMMA_INDEX; i++ ) {
	  msleep(2000);
	  tl2796_gamma_setting(lcd, i);
	}
  }
  return 0;
}

static struct backlight_ops tl2796_backlight_ops = {
  .get_brightness = tl2796_get_brightness,
  .update_status = tl2796_set_brightness,
};

/* spi driver */

static int tl2796_probe(struct spi_device *spi)
{
	int ret = 0;
	struct tl2796 *lcd = NULL;
	struct specific_tl2796 *lcd_spec = NULL;
	struct lcd_device *ld = NULL;
	struct backlight_device *bd = NULL;
	
	lcd = kzalloc(sizeof(struct tl2796), GFP_KERNEL);
	if (!lcd)
	  return -ENOMEM;

	/* ld9040 lcd panel uses 3-wire 16bits SPI Mode. */
	spi->bits_per_word = 16;

	ret = spi_setup(spi);
	if (ret < 0) {
		dev_err(&spi->dev, "spi setup failed.\n");
		goto out_free_lcd;
	}

	lcd->spi = spi;
	lcd->dev = &spi->dev;

	lcd->lcd_pd = spi->dev.platform_data;
	if (!lcd->lcd_pd) {
		dev_err(&spi->dev, "platform data is NULL.\n");
		goto out_free_lcd;
	}   	
	
	lcd_spec = (struct specific_tl2796*)lcd->lcd_pd->pdata;
	if (!lcd_spec) {
	  dev_err(&spi->dev, "Tl2796 no specific lcd data.\n");
	  ret =  -ENOMEM;
	  goto out_free_lcd;
	}	
	lcd->lcd_spec = lcd_spec;

	ret = gpio_request(lcd_spec->lcd_reset, "lcd-reset");
	if (ret) {
	  dev_err(&spi->dev, "Failed to requested gpio for lcd reset.\n");
	  goto out_free_lcd;
	}
	lcd->lcd_reset = lcd_spec->lcd_reset;

	ld = lcd_device_register(lcd_spec->ld_name, &spi->dev, lcd, &tl2796_lcd_ops);
	if (IS_ERR(ld)) {
		ret = PTR_ERR(ld);
		goto free_gpio;
	}

	lcd->ld = ld;

	bd = backlight_device_register(lcd_spec->bd_name, &spi->dev,
		lcd, &tl2796_backlight_ops, NULL);
	if (IS_ERR(bd)) {
		ret = PTR_ERR(bd);
		goto lcd_device_unregister;
	}

	bd->props.max_brightness = MAX_BRIGHTNESS;
	bd->props.brightness = MAX_BRIGHTNESS;
	bd->props.type = BACKLIGHT_RAW;
	lcd->bd = bd;

	dev_set_drvdata(&spi->dev, lcd);

	/*
	 * if lcd panel was on from bootloader like u-boot then
	 * do not lcd on.
	 */
	if (!lcd->lcd_pd->lcd_enabled) {
		/*
		 * if lcd panel was off from bootloader then
		 * current lcd status is powerdown and then
		 * it enables lcd panel.
		 */
		lcd->power = FB_BLANK_POWERDOWN;

		tl2796_power(lcd, FB_BLANK_UNBLANK);
	} else
		lcd->power = FB_BLANK_UNBLANK;

	dev_info(&spi->dev, "TL2796: %s panel driver has been probed.\n", lcd_spec->ld_name);

	return 0;
 
 lcd_device_unregister:
	lcd_device_unregister(ld);	
 free_gpio:
	gpio_free(lcd_spec->lcd_reset);
 out_free_lcd:
	kfree(lcd);
	return ret;
}


static int __devexit tl2796_remove(struct spi_device *spi) {
  int ret;
  struct tl2796 *lcd = dev_get_drvdata(&spi->dev);
  
  ret = tl2796_power(lcd, FB_BLANK_POWERDOWN);
  backlight_device_unregister(lcd->bd);
  lcd_device_unregister(lcd->ld);
  kfree(lcd);
  
  return ret;
}


static void __devexit tl2796_shutdown(struct spi_device *spi) {
  struct tl2796 *lcd = dev_get_drvdata(&spi->dev);
  
  tl2796_power(lcd, FB_BLANK_POWERDOWN);
}

#if defined(CONFIG_PM)

static int tl2796_suspend(struct spi_device *spi, pm_message_t mesg) {
  int ret ;
  struct tl2796 *lcd = dev_get_drvdata(&spi->dev);

  ret = tl2796_power(lcd, FB_BLANK_POWERDOWN);

  dev_dbg(&spi->dev, "lcd->power = %d\n", lcd->power);

  return ret;
}

static int tl2796_resume(struct spi_device *spi) {
  int ret ;
  struct tl2796 *lcd = dev_get_drvdata(&spi->dev);

  ret = tl2796_power(lcd, FB_BLANK_UNBLANK);

  dev_dbg(&spi->dev, "lcd->power = %d\n", lcd->power);

  return ret;
}

#else 

#define tl2796_suspend  NULL
#define tl2796_resume  NULL

#endif

static struct spi_driver tl2796_driver = {
  .driver = {
	.name = "tl2796",
	.bus = &spi_bus_type,
	.owner = THIS_MODULE,
  },
  .probe = tl2796_probe,
  .remove = tl2796_remove,
  .shutdown = tl2796_shutdown,
  .suspend = tl2796_suspend,
  .resume = tl2796_resume,
};

static int __init tl2796_init(void) {
  return spi_register_driver(&tl2796_driver);
}

static void __exit tl2796_exit(void) {
  spi_unregister_driver(&tl2796_driver);
}

module_init(tl2796_init);
module_exit(tl2796_exit);

MODULE_AUTHOR("jiao wu <wujiaososo@qq.com>");
MODULE_DESCRIPTION("tl2796 lcd dirver");
