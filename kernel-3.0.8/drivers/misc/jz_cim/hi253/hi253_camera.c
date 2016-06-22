#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "hi253_camera.h"
#include "hi253_set.h"
#include "../cim_reg.h"

//#define HI253_KERNEL_PRINT

static struct frm_size hi253_capture_table[]= {
	{1600,1200},
	{1280,1024},
	{1024,768},
	{800,600},
	{640,480},
	{352,288},
	{176,144},
};

static struct frm_size hi253_preview_table[]= {
	{1600,1200},
	{1280,1024},
	{1024,768},
	{800,600},
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

int hi253_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
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

unsigned char hi253_read_reg(struct i2c_client *client,unsigned char reg)
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

void hi253_read_all_regs(struct cim_sensor *sensor_info) {
	unsigned char i;
	unsigned char p0_regs[256] = {0};
	unsigned char p1_regs[256] = {0};

	struct hi253_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;

	hi253_write_reg(client, 0xfd, 0);
	for(i = 0; i < 0xff; i++) {
		p0_regs[i] = hi253_read_reg(client, i);
	}

	hi253_write_reg(client, 0xfd, 1);
	for(i = 0; i < 0xff; i++) {
		p1_regs[i] = hi253_read_reg(client, i);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P0: reg[0x%x] = 0x%x\n", i, p0_regs[i]);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P1: reg[0x%x] = 0x%x\n", i, p1_regs[i]);
	}
}

int hi253_power_up(struct cim_sensor *sensor_info)
{
	struct hi253_sensor *s;
	s = container_of(sensor_info, struct hi253_sensor, cs);
#ifdef HI253_KERNEL_PRINT
	dev_info(&s->client->dev,"hi253 power up\n");
#endif
	gpio_set_value(s->gpio_en,0);
	msleep(50);
	return 0;
}

int hi253_power_down(struct cim_sensor *sensor_info)
{
	struct hi253_sensor *s;
	s = container_of(sensor_info, struct hi253_sensor, cs);
#ifdef HI253_KERNEL_PRINT
	dev_info(&s->client->dev,"hi253 power down\n");
#endif
	gpio_set_value(s->gpio_en, 1);
	msleep(50);
	return 0;
}

int hi253_reset(struct cim_sensor *sensor_info)
{
	struct hi253_sensor *s;
	s = container_of(sensor_info, struct hi253_sensor, cs);
#ifdef HI253_KERNEL_PRINT
	dev_info(&s->client->dev,"hi253 reset %x\n",s->gpio_rst);
#endif
	gpio_set_value(s->gpio_rst, 0);
	msleep(50);
	gpio_set_value(s->gpio_rst, 1);
	msleep(50);
	return 0;
}

int hi253_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct hi253_sensor *s;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	hi253_power_up(sensor_info);
	hi253_reset(sensor_info);

	hi253_write_reg(s->client, 0x03, 0x00);
	retval = hi253_read_reg(s->client, 0x04);
	hi253_power_down(sensor_info);

	if(retval == 0x92)//read id,hi253 id is 0x92
		return 0;
	dev_err(&s->client->dev,"hi253 sensor probe fail %x\n",retval);
	return -1;
}

int hi253_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int hi253_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int hi253_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct hi253_sensor * s;
	struct cam_sensor_plat_data *pdata;
	s = kzalloc(sizeof(struct hi253_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "hi253");
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_VY1UY0 | CIM_CFG_ORDER_YUYV | CIM_CFG_PCP;// ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = hi253_preview_table,
		s->cs.capture_size = hi253_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(hi253_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(hi253_capture_table);

	s->cs.probe = hi253_sensor_probe;
	s->cs.init = hi253_init;
	s->cs.reset = hi253_reset;
	s->cs.power_on = hi253_power_up;
	s->cs.shutdown = hi253_power_down;
	s->cs.af_init = hi253_none;
	s->cs.start_af = hi253_none;
	s->cs.stop_af = hi253_none;
	s->cs.read_all_regs = hi253_read_all_regs;

	s->cs.set_preivew_mode = hi253_preview_set;
	s->cs.set_capture_mode = hi253_capture_set;
	s->cs.set_video_mode = hi253_none,
		s->cs.set_resolution = hi253_size_switch;
	s->cs.set_balance = hi253_set_balance;
	s->cs.set_effect = hi253_set_effect;
	s->cs.set_antibanding = hi253_set_antibanding;
	s->cs.set_flash_mode = hi253_none2;
	s->cs.set_scene_mode = hi253_none2;
	s->cs.set_focus_mode = hi253_none2;
	s->cs.set_fps = hi253_none2;
	s->cs.set_nightshot = hi253_none2;
	s->cs.set_luma_adaption = hi253_none2;
	s->cs.set_brightness = hi253_none2;
	s->cs.set_contrast = hi253_none2;

	s->cs.private  = NULL,


	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(gpio_request(pdata->gpio_en, "hi253_en")){
		dev_err(&client->dev,"request hi253 gpio and en fail\n");
		return -1;
	}
	s->gpio_en= pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;

	if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
		dev_err(&client->dev," hi253  gpio is invalid\n");
		return -1;
	}

	gpio_direction_output(s->gpio_rst, 0);
	gpio_direction_output(s->gpio_en, 1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set hi253 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id hi253_id[] = {
	{ "hi253", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, hi253_id);

static struct i2c_driver hi253_driver = {
	.probe		= hi253_probe,
	.id_table	= hi253_id,
	.driver	= {
		.name = "hi253",
	},
};

static int __init hi253_register(void)
{
	return i2c_add_driver(&hi253_driver);
}

module_init(hi253_register);

