#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "hi704_camera.h"
#include "hi704_set.h"
#include "../cim_reg.h"

//#define HI704_KERNEL_PRINT

static struct frm_size hi704_capture_table[]= {
	{640,480},
	{352,288},
	{176,144},
};

static struct frm_size hi704_preview_table[]= {
	{640,480},
	{352,288},
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

int hi704_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
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

unsigned char hi704_read_reg(struct i2c_client *client,unsigned char reg)
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

void hi704_read_all_regs(struct cim_sensor *sensor_info) {
	unsigned char i;
	unsigned char p0_regs[256] = {0};
	unsigned char p1_regs[256] = {0};

	struct hi704_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;

	hi704_write_reg(client, 0xfd, 0);
	for(i = 0; i < 0xff; i++) {
		p0_regs[i] = hi704_read_reg(client, i);
	}

	hi704_write_reg(client, 0xfd, 1);
	for(i = 0; i < 0xff; i++) {
		p1_regs[i] = hi704_read_reg(client, i);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P0: reg[0x%x] = 0x%x\n", i, p0_regs[i]);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P1: reg[0x%x] = 0x%x\n", i, p1_regs[i]);
	}
}

int hi704_power_up(struct cim_sensor *sensor_info)
{
	struct hi704_sensor *s;
	s = container_of(sensor_info, struct hi704_sensor, cs);
#ifdef HI704_KERNEL_PRINT
	dev_info(&s->client->dev,"hi704 power up\n");
#endif
	gpio_set_value(s->gpio_en,0);
	msleep(50);
	return 0;
}

int hi704_power_down(struct cim_sensor *sensor_info)
{
	struct hi704_sensor *s;
	s = container_of(sensor_info, struct hi704_sensor, cs);
#ifdef HI704_KERNEL_PRINT
	dev_info(&s->client->dev,"hi704 power down\n");
#endif
	gpio_set_value(s->gpio_en, 1);
	msleep(50);
	return 0;
}

int hi704_reset(struct cim_sensor *sensor_info)
{
	struct hi704_sensor *s;
	s = container_of(sensor_info, struct hi704_sensor, cs);
#ifdef HI704_KERNEL_PRINT
	dev_info(&s->client->dev,"hi704 reset %x\n",s->gpio_rst);
#endif
	gpio_set_value(s->gpio_rst, 0);
	msleep(50);
	gpio_set_value(s->gpio_rst, 1);
	msleep(50);
	gpio_set_value(s->gpio_rst, 0);
	msleep(50);
	return 0;
}

int hi704_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct hi704_sensor *s;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	hi704_power_up(sensor_info);
	//hi704_reset(sensor_info);

	hi704_write_reg(s->client, 0x03, 0x00);
	retval = hi704_read_reg(s->client, 0x04);
	hi704_power_down(sensor_info);

	if(retval == 0x96)//read id,hi704 id is 0x96
		return 0;
	dev_err(&s->client->dev,"hi704 sensor probe fail %x\n",retval);
	return -1;
}

int hi704_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int hi704_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int hi704_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct hi704_sensor * s;
	struct cam_sensor_plat_data *pdata;
	s = kzalloc(sizeof(struct hi704_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "hi704");
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_VY1UY0 | CIM_CFG_ORDER_YUYV | CIM_CFG_PCP;// ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = hi704_preview_table,
		s->cs.capture_size = hi704_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(hi704_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(hi704_capture_table);

	s->cs.probe = hi704_sensor_probe;
	s->cs.init = hi704_init;
	s->cs.reset = hi704_reset;
	s->cs.power_on = hi704_power_up;
	s->cs.shutdown = hi704_power_down;
	s->cs.af_init = hi704_none;
	s->cs.start_af = hi704_none;
	s->cs.stop_af = hi704_none;
	s->cs.read_all_regs = hi704_read_all_regs;

	s->cs.set_preivew_mode = hi704_preview_set;
	s->cs.set_capture_mode = hi704_capture_set;
	s->cs.set_video_mode = hi704_none,
		s->cs.set_resolution = hi704_size_switch;
	s->cs.set_balance = hi704_set_balance;
	s->cs.set_effect = hi704_set_effect;
	s->cs.set_antibanding = hi704_set_antibanding;
	s->cs.set_flash_mode = hi704_none2;
	s->cs.set_scene_mode = hi704_none2;
	s->cs.set_focus_mode = hi704_none2;
	s->cs.set_fps = hi704_none2;
	s->cs.set_nightshot = hi704_none2;
	s->cs.set_luma_adaption = hi704_none2;
	s->cs.set_brightness = hi704_none2;
	s->cs.set_contrast = hi704_none2;

	s->cs.private  = NULL,


	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(gpio_request(pdata->gpio_en, "hi704_en")){
		dev_err(&client->dev,"request hi704 gpio and en fail\n");
		return -1;
	}
	s->gpio_en= pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;

	if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
		dev_err(&client->dev," hi704  gpio is invalid\n");
		return -1;
	}

	gpio_direction_output(s->gpio_rst, 0);
	gpio_direction_output(s->gpio_en, 1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set hi704 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id hi704_id[] = {
	{ "hi704", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, hi704_id);

static struct i2c_driver hi704_driver = {
	.probe		= hi704_probe,
	.id_table	= hi704_id,
	.driver	= {
		.name = "hi704",
	},
};

static int __init hi704_register(void)
{
	return i2c_add_driver(&hi704_driver);
}

module_init(hi704_register);

