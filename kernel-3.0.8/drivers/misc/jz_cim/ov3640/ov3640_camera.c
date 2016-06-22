#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "ov3640_camera.h"
#include "ov3640_set.h"
#include "../cim_reg.h"

//#define OV3640_KERNEL_PRINT

static struct frm_size ov3640_capture_table[]= {
	{640,480},
	{352,288},
	{320,240},
	{176,144},
};

static struct frm_size ov3640_preview_table[]= {
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

int ov3640_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val)
{
	unsigned char msg[3], tmp;
	int ret;

	memcpy(&msg[0], &reg, 2);
	memcpy(&msg[2], &val, 1);

	tmp = msg[0];
	msg[0] = msg[1];
	msg[1] = tmp;

	ret = sensor_i2c_master_send(client, msg, 3);

	if (ret < 0)
	{
		printk("RET<0\n");
		return ret;
	}
	if (ret < 2)
	{
		printk("RET<3\n");
		return -EIO;
	}

	return 0;
}

unsigned char ov3640_read_reg(struct i2c_client *client,unsigned short reg)
{
	int ret;
	unsigned char retval;
	unsigned char msg[2], tmp;

	printk("------reg = 0x%x \n", reg);
	memcpy(&msg[0], &reg, 2);

	tmp = msg[0];
	msg[0] = msg[1];
	msg[1] = tmp;

	ret = sensor_i2c_master_send(client, msg, 2);

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

void ov3640_read_all_regs(struct cim_sensor *sensor_info) {
	unsigned char i;
	unsigned char p0_regs[256] = {0};
	unsigned char p1_regs[256] = {0};

	struct ov3640_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;

	ov3640_write_reg(client, 0xfd, 0);
	for(i = 0; i < 0xff; i++) {
		p0_regs[i] = ov3640_read_reg(client, i);
	}

	ov3640_write_reg(client, 0xfd, 1);
	for(i = 0; i < 0xff; i++) {
		p1_regs[i] = ov3640_read_reg(client, i);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P0: reg[0x%x] = 0x%x\n", i, p0_regs[i]);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P1: reg[0x%x] = 0x%x\n", i, p1_regs[i]);
	}
}

int ov3640_power_up(struct cim_sensor *sensor_info)
{
	struct ov3640_sensor *s;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
#ifdef OV3640_KERNEL_PRINT
	dev_info(&s->client->dev,"ov3640 power up\n");
#endif
	gpio_set_value(s->gpio_en,0);
	msleep(50);
	return 0;
}

int ov3640_power_down(struct cim_sensor *sensor_info)
{
	struct ov3640_sensor *s;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
#ifdef OV3640_KERNEL_PRINT
	dev_info(&s->client->dev,"ov3640 power down\n");
#endif
	gpio_set_value(s->gpio_en,1);
	msleep(50);
	return 0;
}

int ov3640_reset(struct cim_sensor *sensor_info)
{
	struct ov3640_sensor *s;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
#ifdef OV3640_KERNEL_PRINT
	dev_info(&s->client->dev,"ov3640 reset %x\n",s->gpio_rst);
#endif
	gpio_set_value(s->gpio_rst,0);
	msleep(250);
	gpio_set_value(s->gpio_rst,1);
	msleep(250);
	return 0;
}

int ov3640_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct ov3640_sensor *s;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	ov3640_power_up(sensor_info);
	ov3640_reset(sensor_info);

	retval=ov3640_read_reg(s->client,0x300a);
	ov3640_power_down(sensor_info);

	if(retval == 0x36)//read id,ov3640 id is 0x36
		return 0;

	dev_err(&s->client->dev,"ov3640 sensor probe fail 0x%x\n",retval);
	return -1;
}

int ov3640_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

void ov3640_none1(struct cim_sensor  *sensor_info)
{
}

int ov3640_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int ov3640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ov3640_sensor * s;
	struct cam_sensor_plat_data *pdata;
	s = kzalloc(sizeof(struct ov3640_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "ov3640");
	s->cs.cim_cfg = CIM_CFG_DSM_GCM | CIM_CFG_PACK_Y1VY0U | CIM_CFG_ORDER_UYVY;//CIM_CFG_PCP ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = ov3640_preview_table,
		s->cs.capture_size = ov3640_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(ov3640_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(ov3640_capture_table);

	s->cs.probe = ov3640_sensor_probe;
	s->cs.init = ov3640_init;
	s->cs.reset = ov3640_reset;
	s->cs.power_on = ov3640_power_up;
	s->cs.shutdown = ov3640_power_down;
	s->cs.af_init = ov3640_none;
	s->cs.start_af = ov3640_none;
	s->cs.stop_af = ov3640_none;
	s->cs.read_all_regs = ov3640_none1;

	s->cs.set_preivew_mode = ov3640_preview_set;
	s->cs.set_capture_mode = ov3640_capture_set;
	s->cs.set_video_mode = ov3640_none,
		s->cs.set_resolution = ov3640_size_switch;
	s->cs.set_balance = ov3640_set_balance;
	s->cs.set_effect = ov3640_set_effect;
	s->cs.set_antibanding = ov3640_set_antibanding;
	s->cs.set_flash_mode = ov3640_none2;
	s->cs.set_scene_mode = ov3640_none2;
	s->cs.set_focus_mode = ov3640_none2;
	s->cs.set_fps = ov3640_none2;
	s->cs.set_nightshot = ov3640_none2;
	s->cs.set_luma_adaption = ov3640_none2;
	s->cs.set_brightness = ov3640_none2;
	s->cs.set_contrast = ov3640_none2;

	s->cs.private  = NULL,


	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(gpio_request(pdata->gpio_en, "ov3640_en") || gpio_request(pdata->gpio_rst, "ov3640_rst")){
		dev_err(&client->dev,"request ov3640 gpio rst and en fail\n");
		return -1;
	}
	s->gpio_en= pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;

	if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
		dev_err(&client->dev," ov3640  gpio is invalid\n");
		return -1;
	}

	gpio_direction_output(s->gpio_rst,0);
	gpio_direction_output(s->gpio_en,1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set ov3640 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id ov3640_id[] = {
	{ "ov3640", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, ov3640_id);

static struct i2c_driver ov3640_driver = {
	.probe		= ov3640_probe,
	.id_table	= ov3640_id,
	.driver	= {
		.name = "ov3640",
		.owner = THIS_MODULE,
	},
};

static int __init ov3640_register(void)
{
	return i2c_add_driver(&ov3640_driver);
}

module_init(ov3640_register);

