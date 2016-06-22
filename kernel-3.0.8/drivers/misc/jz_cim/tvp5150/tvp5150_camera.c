#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "tvp5150_camera.h"
#include "tvp5150_reg.h"
#include "tvp5150_set.h"
#include "../cim_reg.h"

#define TVP5150_KERNEL_PRINT


static struct frm_size tvp5150_capture_table[]= {
		{720,576},
};

static struct frm_size tvp5150_preview_table[]= {
		{720,576},
};

static inline int sensor_i2c_master_send(struct i2c_client *client,const char *buf ,int count)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
	ret = i2c_transfer(adap, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static inline int sensor_i2c_master_recv(struct i2c_client *client, char *buf ,int count)
{
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;
	ret = i2c_transfer(adap, &msg, 1);
	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

int tvp5150_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
{
	unsigned char msg[2];
	int ret;

	memcpy(&msg[0], &reg, 1);
	memcpy(&msg[1], &val, 1);

	ret = sensor_i2c_master_send(client, msg, 2);

	if (ret < 0)
		{
			printk("RET<0\n");
			return ret;
		}
	if (ret < 2)
		{
			printk("RET<2\n");
			return -EIO;
		}

	return 0;
}

unsigned char tvp5150_read_reg(struct i2c_client *client,unsigned char reg)
{
	int ret;
	unsigned char retval;

	ret = sensor_i2c_master_send(client,&reg,1);

	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;

	ret = sensor_i2c_master_recv(client, &retval, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;
	return retval;
}

int tvp5150_power_up(struct cim_sensor *sensor_info)
{
	struct tvp5150_sensor *s;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
#ifdef TVP5150_KERNEL_PRINT
	printk("tvp5150 power up-----------\n");
#endif
	if (gpio_is_valid(s->gpio_en)) gpio_set_value(s->gpio_en,1);
	mdelay(50);
	return 0;
}

int tvp5150_power_down(struct cim_sensor *sensor_info)
{
	struct tvp5150_sensor *s;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
#ifdef TVP5150_KERNEL_PRINT
	printk("tvp5150 power down-----------\n");
#endif
	if (gpio_is_valid(s->gpio_en)) gpio_set_value(s->gpio_en,0);
	mdelay(50);
	return 0;
}

int tvp5150_reset(struct cim_sensor *sensor_info)
{
	struct tvp5150_sensor *s;
	s = container_of(sensor_info, struct tvp5150_sensor, cs);
#ifdef TVP5150_KERNEL_PRINT
	printk("tvp5150 reset-----------------------\n");
#endif
	if (gpio_is_valid(s->gpio_rst)) gpio_set_value(s->gpio_rst,0);
	msleep(50);
	if (gpio_is_valid(s->gpio_rst)) gpio_set_value(s->gpio_rst,1);
	msleep(1);

	return 0;
}

int tvp5150_sensor_probe(struct cim_sensor *sensor_info)
{
	u8 msb_id, lsb_id, msb_rom, lsb_rom;

	struct tvp5150_sensor *s;
	struct i2c_client * client ;
#ifdef TVP5150_KERNEL_PRINT
	printk("tvp5150_sensor_probe------------------------\n");
#endif
	s = container_of(sensor_info, struct tvp5150_sensor, cs);

	client = s->client;

	tvp5150_power_up(sensor_info);
	tvp5150_reset(sensor_info);

	msb_id =  tvp5150_read_reg(s->client, TVP5150_MSB_DEV_ID);
	lsb_id =  tvp5150_read_reg(s->client, TVP5150_LSB_DEV_ID);
	msb_rom = tvp5150_read_reg(s->client, TVP5150_ROM_MAJOR_VER);
	lsb_rom = tvp5150_read_reg(s->client, TVP5150_ROM_MINOR_VER);

	if (msb_rom == 4 && lsb_rom == 0) { /* Is TVP5150AM1 */
		printk("tvp%02x%02xam1 detected.\n", msb_id, lsb_id);
		/* ITU-T BT.656.4 timing */
		tvp5150_write_reg(client, TVP5150_REV_SELECT, 0);
	} else {
		if (msb_rom == 3 || lsb_rom == 0x21) { /* Is TVP5150A */
			printk("tvp%02x%02xa detected.\n", msb_id, lsb_id);
		} else {
			printk("*** unknown tvp%02x%02x chip detected.\n",
				  msb_id, lsb_id);
			printk("*** Rom ver is %d.%d\n", msb_rom, lsb_rom);
		}
	}
	tvp5150_power_down(sensor_info);

	return 0;
}

int tvp5150_none(struct cim_sensor  *sensor_info)
{
	printk("tvp5150_none++++++++++++++++++++++++++++++\n");
	return 0;
}

int tvp5150_none2(struct cim_sensor * desc,unsigned short arg)
{
	printk("tvp5150_none2++++++++++++++++++++++++++++++\n");
	return 0;
}

static int tvp5150_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct tvp5150_sensor * s;
	struct tvp5150_platform_data *pdata;
	s = kzalloc(sizeof(struct tvp5150_sensor), GFP_KERNEL);
	printk("tvp5150_probe++++++++++++++++++++++++++++++\n");
	strcpy(s->cs.name , "tvp5150");
	s->cs.cim_cfg = CIM_CFG_DSM_CIM  | CIM_CFG_PACK_Y1VY0U | CIM_CFG_DF_ITU656 | CIM_CFG_ORDER_UYVY;
/*	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA|EFFECT_AQUA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
*/
	s->cs.preview_size = tvp5150_preview_table,
	s->cs.capture_size = tvp5150_capture_table,
	s->cs.prev_resolution_nr = ARRAY_SIZE(tvp5150_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(tvp5150_capture_table);

	s->cs.probe = tvp5150_sensor_probe;
	s->cs.init = tvp5150_init;
	s->cs.reset = tvp5150_reset;
	s->cs.power_on = tvp5150_power_up;
	s->cs.shutdown = tvp5150_power_down;
	s->cs.af_init = tvp5150_none;
	s->cs.start_af = tvp5150_none;
	s->cs.stop_af = tvp5150_none;
	s->cs.set_preivew_mode = tvp5150_preview_set;
	s->cs.set_capture_mode = tvp5150_capture_set;
	s->cs.set_video_mode = tvp5150_none,
	s->cs.set_resolution = tvp5150_size_switch;
	s->cs.set_balance = tvp5150_set_balance;
	s->cs.set_effect = tvp5150_set_effect;
	s->cs.set_antibanding = tvp5150_none2;
	s->cs.set_flash_mode = tvp5150_none2;
	s->cs.set_scene_mode = tvp5150_none2;
	s->cs.set_focus_mode = tvp5150_none2;
	s->cs.set_fps = tvp5150_none2;
	s->cs.set_nightshot = tvp5150_none2;
	s->cs.set_luma_adaption = tvp5150_none2;
	s->cs.set_brightness = tvp5150_none2;
	s->cs.set_contrast = tvp5150_none2;
	s->cs.private  = NULL;
	s->client = client;

	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}

	if (gpio_request(pdata->gpio_en, "tvp5150_en"))
		printk(" tvp5150 no PDN operation.\n");

	if (gpio_request(pdata->gpio_rst, "tvp5150_rst")) {
               	dev_err(&client->dev,"request tvp5150 gpio rst fail\n");
               	return -1;
	}
	
	s->gpio_en= pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;

	if (gpio_is_valid(s->gpio_rst)) gpio_direction_output(s->gpio_rst,0);
	if (gpio_is_valid(s->gpio_en)) gpio_direction_output(s->gpio_en,1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	s->cs.pos = pdata->pos;
	camera_sensor_register(&s->cs);
	printk("tvp5150_probe-----------------------------\n");
	return 0;
}

static const struct i2c_device_id tvp5150_id[] = {
	{ "tvp5150", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, tvp5150_id);

static struct i2c_driver tvp5150_driver = {
	.probe		= tvp5150_probe,
	.id_table	= tvp5150_id,
	.driver	= {
		.name = "tvp5150",
		.owner	= THIS_MODULE,
	},
};

static int __init tvp5150_register(void)
{
	int ret = 0;
	printk(KERN_INFO "add tvp5150 i2c driver\n");
	ret = i2c_add_driver(&tvp5150_driver);
	if (ret < 0){
		printk(KERN_INFO "add tvp5150 i2c driver failed\n");
		return -ENODEV;
	}
	return ret;
}

module_init(tvp5150_register);
