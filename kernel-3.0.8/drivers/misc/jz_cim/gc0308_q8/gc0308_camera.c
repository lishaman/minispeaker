#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "gc0308_camera.h"
#include "gc0308_set.h"
#include "../cim_reg.h"

//#define GC0308_KERNEL_PRINT

static struct frm_size gc0308_capture_table[]= {
	{640,480},
	{352,288},
	{320,240},
	{176,144},
};

static struct frm_size gc0308_preview_table[]= {
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

int gc0308_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
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

unsigned char gc0308_read_reg(struct i2c_client *client,unsigned char reg)
{
	int ret;
	unsigned char retval;

	ret = sensor_i2c_master_send(client, &reg, 1);

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

void gc0308_read_all_regs(struct cim_sensor *sensor_info) {
	unsigned char i;
	unsigned char p0_regs[256] = {0};
	unsigned char p1_regs[256] = {0};

	struct gc0308_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;

	gc0308_write_reg(client, 0xfd, 0);
	for(i = 0; i < 0xff; i++) {
		p0_regs[i] = gc0308_read_reg(client, i);
	}

	gc0308_write_reg(client, 0xfd, 1);
	for(i = 0; i < 0xff; i++) {
		p1_regs[i] = gc0308_read_reg(client, i);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P0: reg[0x%x] = 0x%x\n", i, p0_regs[i]);
	}

	for(i = 0; i < 0xff; i++) {
		printk("P1: reg[0x%x] = 0x%x\n", i, p1_regs[i]);
	}
}

int gc0308_power_up(struct cim_sensor *sensor_info)
{
	struct gc0308_sensor *s;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
#ifdef GC0308_KERNEL_PRINT
	dev_info(&s->client->dev,"gc0308 power up\n");
#endif
	gpio_set_value(s->gpio_en, 0);
	msleep(100);
	return 0;
}

int gc0308_power_down(struct cim_sensor *sensor_info)
{
	struct gc0308_sensor *s;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
#ifdef GC0308_KERNEL_PRINT
	dev_info(&s->client->dev,"gc0308 power down\n");
#endif
	gpio_set_value(s->gpio_en, 1);
	msleep(100);
	return 0;
}

int gc0308_reset(struct cim_sensor *sensor_info)
{
	struct gc0308_sensor *s;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
#ifdef GC0308_KERNEL_PRINT
	dev_info(&s->client->dev,"gc0308 reset %x\n",s->gpio_rst);
#endif
	gpio_set_value(s->gpio_rst, 0);
	msleep(100);
	gpio_set_value(s->gpio_rst, 1);
	msleep(100);
	return 0;
}

int gc0308_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct gc0308_sensor *s;
	printk("gc0308_sensor_probe_2 ------\n");
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	gc0308_power_up(sensor_info);
	gc0308_reset(sensor_info);

	retval=gc0308_read_reg(s->client,0x00);
	gc0308_power_down(sensor_info);

	if(retval == 0x9b){//read id,gc0308 id is 0x9b
		printk("gc0308_sensor_probe_2 success------\n");
		return 0;
	}
	dev_err(&s->client->dev,"gc0308 sensor probe fail %x\n",retval);
	return -1;
}

int gc0308_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int gc0308_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int gc0308_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct gc0308_sensor * s;
	struct gc0308_sensor * s_back;
	struct gc0308_platform_data *pdata;
	s = kzalloc(sizeof(struct gc0308_sensor), GFP_KERNEL);
	s_back = kzalloc(sizeof(struct gc0308_sensor), GFP_KERNEL);
	printk("gc0308_double_probe==========\n");

	strcpy(s->cs.name , "gc0308");
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_VY1UY0 | CIM_CFG_ORDER_YUYV | CIM_CFG_PCP;// ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = gc0308_preview_table,
		s->cs.capture_size = gc0308_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(gc0308_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(gc0308_capture_table);

	s->cs.probe = gc0308_sensor_probe;
	s->cs.init = gc0308_init;
	s->cs.reset = gc0308_reset;
	s->cs.power_on = gc0308_power_up;
	s->cs.shutdown = gc0308_power_down;
	s->cs.af_init = gc0308_none;
	s->cs.start_af = gc0308_none;
	s->cs.stop_af = gc0308_none;
	s->cs.read_all_regs = gc0308_read_all_regs;

	s->cs.set_preivew_mode = gc0308_preview_set;
	s->cs.set_capture_mode = gc0308_capture_set;
	s->cs.set_video_mode = gc0308_none,
		s->cs.set_resolution = gc0308_size_switch;
	s->cs.set_balance = gc0308_set_balance;
	s->cs.set_effect = gc0308_set_effect;
	s->cs.set_antibanding = gc0308_set_antibanding;
	s->cs.set_flash_mode = gc0308_none2;
	s->cs.set_scene_mode = gc0308_none2;
	s->cs.set_focus_mode = gc0308_none2;
	s->cs.set_fps = gc0308_none2;
	s->cs.set_nightshot = gc0308_none2;
	s->cs.set_luma_adaption = gc0308_none2;
	s->cs.set_brightness = gc0308_none2;
	s->cs.set_contrast = gc0308_none2;

	s->cs.private  = NULL,
	s->client = client;

	s_back->cs = s->cs;
	s_back->cs.private = NULL,
	s_back->client = client;

	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(gpio_request(pdata->gpio_en_f, "gc0308_en_f") || gpio_request(pdata->gpio_rst, "gc0308_rst")){
		dev_err(&client->dev,"request gc0308 gpio rst and en fail\n");
		return -1;
	}
	s->gpio_en= pdata->gpio_en_f;
	s->gpio_rst = pdata->gpio_rst;

	if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
		dev_err(&client->dev," gc0308  gpio rst and en_f is invalid\n");
		return -1;
	}

	if(gpio_request(pdata->gpio_en_b,"gc0308_en_b")){
		dev_err(&client->dev,"request gc0308 gc0308_en_b  fail\n");
		return -1;
	}
	s_back->gpio_rst = pdata->gpio_rst;
	s_back->gpio_en = pdata->gpio_en_b;


	if(gpio_is_valid(s_back->gpio_en))
		dev_info(&s->client->dev, "s_back->gpio_en invailed\n");

	gpio_direction_output(s->gpio_rst, 0);
	gpio_direction_output(s->gpio_en, 1);
	gpio_direction_output(s_back->gpio_en, 1);

	s->cs.facing = pdata->facing_f;
	s->cs.orientation = pdata->orientation_f;
	s_back->cs.facing = pdata->facing_b;
	s_back->cs.orientation = pdata->orientation_b;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
    s_back->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set gc0308 i2c speed : 400khz
	camera_sensor_register(&s->cs);
	camera_sensor_register(&s_back->cs);
	return 0;
}

static const struct i2c_device_id gc0308_id[] = {
	{ "gc0308", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, gc0308_id);

static struct i2c_driver gc0308_driver = {
	.probe		= gc0308_probe,
	.id_table	= gc0308_id,
	.driver	= {
		.name = "gc0308",
	},
};

static int __init gc0308_register(void)
{
	printk("gc0308_init==\n");
	return i2c_add_driver(&gc0308_driver);
}
module_init(gc0308_register);


