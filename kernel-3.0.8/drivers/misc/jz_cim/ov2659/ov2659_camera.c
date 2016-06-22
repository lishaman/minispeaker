#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "ov2659_camera.h"
#include "ov2659_set.h"
#include "../cim_reg.h"

static struct frm_size ov2659_capture_table[]= {
	{1600,1200},
    //{1280, 720},
	{800, 600},
	{640,480},
};

static struct frm_size ov2659_preview_table[]= {
	{1280, 720},
	{800, 600},
	//{352,288},
	//{320,240},
	//{176,144},
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

int ov2659_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val)
{
	unsigned char msg[3];
	int ret;

    reg = cpu_to_be16(reg);

	memcpy(&msg[0], &reg, 2);
	memcpy(&msg[2], &val, 1);

	ret = sensor_i2c_master_send(client, msg, 3);

	if (ret < 0)
	{
		printk("RET<0\n");
		return ret;
	}
	if (ret < 3)
	{
		printk("RET<3\n");
		return -EIO;
	}

	return 0;
}

unsigned char ov2659_read_reg(struct i2c_client *client,unsigned short reg)
{
	int ret;
	unsigned char retval;
    u16 r = cpu_to_be16(reg);

	ret = sensor_i2c_master_send(client,(unsigned char*)&r,2);

	if (ret < 0)
		return ret;
	if (ret != 2)
		return -EIO;

	ret = sensor_i2c_master_recv(client, &retval, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;
	return retval;
}

int ov2659_power_up(struct cim_sensor *sensor_info)
{
	struct ov2659_sensor *s;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	printk("%s --------------power up\n", s->cs.name);
	gpio_set_value(s->gpio_en, 0);
	mdelay(20);
	return 0;
}

int ov2659_power_down(struct cim_sensor *sensor_info)
{
	struct ov2659_sensor *s;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	printk("%s --------------power down\n", s->cs.name);
	gpio_set_value(s->gpio_en, 1);
	mdelay(20);
	return 0;
}

int ov2659_reset(struct cim_sensor *sensor_info)
{
	struct ov2659_sensor *s;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	printk("%s --------------reset\n", s->cs.name);
	gpio_set_value(s->gpio_rst, 0);
	mdelay(50);
	gpio_set_value(s->gpio_rst, 1);
	mdelay(50);
	return 0;
}

int ov2659_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct ov2659_sensor *s;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	ov2659_power_up(sensor_info);
	ov2659_reset(sensor_info);

	retval = ov2659_read_reg(s->client, 0x300a);
	retval <<= 8;
	retval |= ov2659_read_reg(s->client, 0x300b);

	ov2659_power_down(sensor_info);
	if(retval == 0x2656)//read id,ov2659 id is 0x2656
		return 0;
	dev_info(&s->client->dev,"sensor probe fail %x\n",retval);
	return -1;
}

int ov2659_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int ov2659_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int ov2659_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ov2659_sensor *s_front;
	struct ov2659_sensor *s_back;
	struct ov2659_platform_data *pdata;

	s_front = kzalloc(sizeof(struct ov2659_sensor), GFP_KERNEL);
	s_back = kzalloc(sizeof(struct ov2659_sensor), GFP_KERNEL);

	strcpy(s_front->cs.name , "ov2659-front");
	s_front->cs.cim_cfg = CIM_CFG_DSM_GCM |CIM_CFG_VSP |CIM_CFG_PACK_Y1VY0U;//CIM_CFG_PCP |
	s_front->cs.modes.balance = WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
							| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s_front->cs.modes.effect = EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA|EFFECT_AQUA;
	s_front->cs.modes.antibanding = ANTIBANDING_50HZ | ANTIBANDING_60HZ ;

	//s_front->cs.modes.flash_mode = FLASH_MODE_OFF;
	s_front->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s_front->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s_front->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |

	s_front->cs.preview_size = ov2659_preview_table,
	s_front->cs.capture_size = ov2659_capture_table,
	s_front->cs.prev_resolution_nr = ARRAY_SIZE(ov2659_preview_table);
	s_front->cs.cap_resolution_nr = ARRAY_SIZE(ov2659_capture_table);

	s_front->cs.probe = ov2659_sensor_probe;
	s_front->cs.init = ov2659_init;
	s_front->cs.reset = ov2659_reset;
	s_front->cs.power_on = ov2659_power_up;
	s_front->cs.shutdown = ov2659_power_down;
	s_front->cs.af_init = ov2659_none;
	s_front->cs.start_af = ov2659_none;
	s_front->cs.stop_af = ov2659_none;

	s_front->cs.set_preivew_mode = ov2659_preview_set;
	s_front->cs.set_capture_mode = ov2659_capture_set;
	s_front->cs.set_video_mode = ov2659_none,
	s_front->cs.set_resolution = ov2659_size_switch;
	s_front->cs.set_balance = ov2659_set_balance;
	s_front->cs.set_effect = ov2659_set_effect;
	s_front->cs.set_antibanding = ov2659_set_antibanding;
	s_front->cs.set_flash_mode = ov2659_none2;
	s_front->cs.set_scene_mode = ov2659_none2;
	s_front->cs.set_focus_mode = ov2659_none2;
	s_front->cs.set_fps = ov2659_none2;
	s_front->cs.set_nightshot = ov2659_none2;
	s_front->cs.set_luma_adaption = ov2659_none2;
	s_front->cs.set_brightness = ov2659_none2;
	s_front->cs.set_contrast = ov2659_none2;

	s_front->cs.private = NULL,
	s_front->client = client;

	s_back->cs = s_front->cs;
	s_back->cs.private = NULL,
	s_back->client = client;
	strcpy(s_back->cs.name , "ov2659-back");

	pdata = client->dev.platform_data;
	if(client->dev.platform_data == NULL) {
		dev_info(&s_front->client->dev, "ov2659 has no platform data\n");
		return -1;
	}

	s_front->gpio_rst = pdata->gpio_rst;
	s_front->gpio_en = pdata->gpio_en_f;

	s_back->gpio_rst = pdata->gpio_rst;
	s_back->gpio_en = pdata->gpio_en_b;

	if(gpio_is_valid(s_front->gpio_rst))
		dev_info(&s_front->client->dev, "reset gpio invailed\n");

	if(gpio_is_valid(s_front->gpio_en))
		dev_info(&s_front->client->dev, "en gpio invailed\n");

	if(gpio_is_valid(s_back->gpio_en))
		dev_info(&s_front->client->dev, "en gpio invailed\n");

	if(gpio_request(pdata->gpio_rst, "ov2659_rst"))
		dev_info(&s_front->client->dev, "request reset gpio invailed\n");

	if(gpio_request(pdata->gpio_en_f, "ov2659_en_f"))
		dev_info(&s_front->client->dev, "request en gpio invailed\n");

	if(gpio_request(pdata->gpio_en_b, "ov2659_en_b"))
		dev_info(&s_front->client->dev, "request en gpio invailed\n");

	gpio_direction_output(s_front->gpio_rst, 0);
	gpio_direction_output(s_front->gpio_en, 1);
	gpio_direction_output(s_back->gpio_en, 1);

	s_front->cs.facing = pdata->facing_f;
	s_front->cs.orientation = pdata->orientation_f;
	s_back->cs.facing = pdata->facing_b;
	s_back->cs.orientation = pdata->orientation_b;

	camera_sensor_register(&s_front->cs);
	camera_sensor_register(&s_back->cs);

	return 0;
}

static const struct i2c_device_id ov2659_id[] = {
	{ "ov2659-module", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, ov2659_id);

static struct i2c_driver ov2659_driver = {
	.probe = ov2659_probe,
	.id_table = ov2659_id,
	.driver	= { .name = "ov2659-module",},
};

static int __init ov2659_register(void)
{
	return i2c_add_driver(&ov2659_driver);
}

module_init(ov2659_register);

