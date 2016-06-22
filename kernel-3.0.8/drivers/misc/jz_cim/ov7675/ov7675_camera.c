#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "ov7675_camera.h"
#include "ov7675_set.h"
#include "../cim_reg.h"

static struct frm_size ov7675_capture_table[]= {
	{640,480},
	{352,288},
	{320,240},
	{176,144},
};

static struct frm_size ov7675_preview_table[]= {
	{640,480},
	{352,288},
	{320,240},
	{176,144},
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

int ov7675_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
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

unsigned char ov7675_read_reg(struct i2c_client *client,unsigned char reg)
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

int ov7675_power_up(struct cim_sensor *sensor_info)
{
	struct ov7675_sensor *s;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	dev_info(&s->client->dev,"ov7675-----------power up\n");
    mdelay(50);
	gpio_set_value(s->gpio_en,0);
	return 0;
}

int ov7675_power_down(struct cim_sensor *sensor_info)
{
	struct ov7675_sensor *s;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	dev_info(&s->client->dev,"ov7675-----------power down\n");
	gpio_set_value(s->gpio_en,1);
	mdelay(20);
	return 0;
}

int ov7675_reset(struct cim_sensor *sensor_info)
{
	return 0;
}

int ov7675_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct ov7675_sensor *s;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	ov7675_power_up(sensor_info);
    mdelay(50);
    ov7675_write_reg(s->client,0x12,0x80);
    mdelay(10);
    retval=ov7675_read_reg(s->client,0x0a);
    printk("retval = 0x%x\n", retval);
    ov7675_power_down(sensor_info);

	if(retval == 0x76)//read id,ov7675 id is 0x76
		return 0;
	dev_info(&s->client->dev,"ov7675 sensor probe fail %x\n",retval);

	return -1;
}

int ov7675_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int ov7675_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int ov7675_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ov7675_sensor * s;
	struct ov7675_platform_data *pdata;
	s = kzalloc(sizeof(struct ov7675_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "ov7675");
	//s->cs.cim_cfg = CIM_CFG_DSM_GCM |CIM_CFG_VSP |CIM_CFG_PACK_Y1VY0U;//CIM_CFG_PCP |
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_Y1VY0U | CIM_CFG_ORDER_UYVY;//CIM_CFG_PCP ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT 
							| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA|EFFECT_AQUA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = ov7675_preview_table,
	s->cs.capture_size = ov7675_capture_table,
	s->cs.prev_resolution_nr = ARRAY_SIZE(ov7675_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(ov7675_capture_table);
	
	s->cs.probe = ov7675_sensor_probe;
	s->cs.init = ov7675_init;
	s->cs.reset = ov7675_reset;
	s->cs.power_on = ov7675_power_up;
	s->cs.shutdown = ov7675_power_down;
	s->cs.af_init = ov7675_none;
	s->cs.start_af = ov7675_none;
	s->cs.stop_af = ov7675_none;
	
	s->cs.set_preivew_mode = ov7675_preview_set;
	s->cs.set_capture_mode = ov7675_capture_set;
	s->cs.set_video_mode = ov7675_none,
	s->cs.set_resolution = ov7675_size_switch;
	s->cs.set_balance = ov7675_set_balance;
	s->cs.set_effect = ov7675_set_effect;
	s->cs.set_antibanding = ov7675_set_antibanding;
	s->cs.set_flash_mode = ov7675_none2;
	s->cs.set_scene_mode = ov7675_none2;
	s->cs.set_focus_mode = ov7675_none2;
	s->cs.set_fps = ov7675_none2;
	s->cs.set_nightshot = ov7675_none2;
	s->cs.set_luma_adaption = ov7675_none2;
	s->cs.set_brightness = ov7675_none2;
	s->cs.set_contrast = ov7675_none2;
   
	s->cs.private  = NULL,
				
				
	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(!gpio_request(pdata->gpio_en, "ov7675_en"))
		s->gpio_en= pdata->gpio_en;

	gpio_direction_output(s->gpio_en,1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set ov7675 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id ov7675_id[] = {
	{ "ov7675", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, ov7675_id);

static struct i2c_driver ov7675_driver = {
	.probe		= ov7675_probe,
	.id_table	= ov7675_id,
	.driver	= {
		.name = "ov7675",
	},
};

static int __init ov7675_register(void)
{
	return i2c_add_driver(&ov7675_driver);
}

module_init(ov7675_register);

