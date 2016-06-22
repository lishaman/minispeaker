#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "sp0838_camera.h"
#include "sp0838_set.h"
#include "../cim_reg.h"

//#define SP0838_KERNEL_PRINT

static struct frm_size sp0838_capture_table[]= {
	{640,480},
	{352,288},
	{320,240},
	{176,144},
};

static struct frm_size sp0838_preview_table[]= {
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

int sp0838_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
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

unsigned char sp0838_read_reg(struct i2c_client *client,unsigned char reg)
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

void sp0838_read_all_regs(struct cim_sensor *sensor_info) {
	unsigned char i;
	unsigned char p0_regs[256] = {0};
	unsigned char p1_regs[256] = {0};
	
	struct sp0838_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
	
	sp0838_write_reg(client, 0xfd, 0);
	for(i = 0; i < 0xff; i++) {
		p0_regs[i] = sp0838_read_reg(client, i);
	}
	
	sp0838_write_reg(client, 0xfd, 1);
	for(i = 0; i < 0xff; i++) {
		p1_regs[i] = sp0838_read_reg(client, i);
	}
	
	for(i = 0; i < 0xff; i++) {
		printk("P0: reg[0x%x] = 0x%x\n", i, p0_regs[i]);
	}
	
	for(i = 0; i < 0xff; i++) {
		printk("P1: reg[0x%x] = 0x%x\n", i, p1_regs[i]);
	}
}

int sp0838_power_up(struct cim_sensor *sensor_info)
{
	struct sp0838_sensor *s;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
#ifdef SP0838_KERNEL_PRINT	
	dev_info(&s->client->dev,"sp0838 power up\n");
#endif	
	gpio_set_value(s->gpio_en,0);
	msleep(50);
	return 0;
}

int sp0838_power_down(struct cim_sensor *sensor_info)
{
	struct sp0838_sensor *s;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
#ifdef SP0838_KERNEL_PRINT	
	dev_info(&s->client->dev,"sp0838 power down\n");
#endif	
	gpio_set_value(s->gpio_en,1);
	msleep(50);
	return 0;
}

int sp0838_reset(struct cim_sensor *sensor_info)
{
	struct sp0838_sensor *s;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
#ifdef SP0838_KERNEL_PRINT
	dev_info(&s->client->dev,"sp0838 reset %x\n",s->gpio_rst);
#endif
	gpio_set_value(s->gpio_rst,0);
	msleep(250);
	gpio_set_value(s->gpio_rst,1);
	msleep(250);
	return 0;
}

int sp0838_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct sp0838_sensor *s;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	sp0838_power_up(sensor_info);
	sp0838_reset(sensor_info);
	
	retval=sp0838_read_reg(s->client,0x02);
	sp0838_power_down(sensor_info);

	if(retval == 0x27)//read id,sp0838 id is 0x27
		return 0;
	dev_err(&s->client->dev,"sp0838 sensor probe fail %x\n",retval);
	return -1;
}

int sp0838_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int sp0838_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int sp0838_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sp0838_sensor * s;
	struct cam_sensor_plat_data *pdata;
	s = kzalloc(sizeof(struct sp0838_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "sp0838");
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_Y1VY0U | CIM_CFG_ORDER_UYVY;//CIM_CFG_PCP ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT 
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = sp0838_preview_table,
		s->cs.capture_size = sp0838_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(sp0838_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(sp0838_capture_table);

	s->cs.probe = sp0838_sensor_probe;
	s->cs.init = sp0838_init;
	s->cs.reset = sp0838_reset;
	s->cs.power_on = sp0838_power_up;
	s->cs.shutdown = sp0838_power_down;
	s->cs.af_init = sp0838_none;
	s->cs.start_af = sp0838_none;
	s->cs.stop_af = sp0838_none;
	s->cs.read_all_regs = sp0838_read_all_regs;

	s->cs.set_preivew_mode = sp0838_preview_set;
	s->cs.set_capture_mode = sp0838_capture_set;
	s->cs.set_video_mode = sp0838_none,
		s->cs.set_resolution = sp0838_size_switch;
	s->cs.set_balance = sp0838_set_balance;
	s->cs.set_effect = sp0838_set_effect;
	s->cs.set_antibanding = sp0838_set_antibanding;
	s->cs.set_flash_mode = sp0838_none2;
	s->cs.set_scene_mode = sp0838_none2;
	s->cs.set_focus_mode = sp0838_none2;
	s->cs.set_fps = sp0838_none2;
	s->cs.set_nightshot = sp0838_none2;
	s->cs.set_luma_adaption = sp0838_none2;
	s->cs.set_brightness = sp0838_none2;
	s->cs.set_contrast = sp0838_none2;

	s->cs.private  = NULL,


	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(gpio_request(pdata->gpio_en, "sp0838_en") || gpio_request(pdata->gpio_rst, "sp0838_rst")){
		dev_err(&client->dev,"request sp0838 gpio rst and en fail\n");
		return -1;
	}
	s->gpio_en= pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;

	if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
		dev_err(&client->dev," sp0838  gpio is invalid\n");
		return -1;
	}
	
	gpio_direction_output(s->gpio_rst,0);
	gpio_direction_output(s->gpio_en,1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set sp0838 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id sp0838_id[] = {
	{ "sp0838", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, sp0838_id);

static struct i2c_driver sp0838_driver = {
	.probe		= sp0838_probe,
	.id_table	= sp0838_id,
	.driver	= {
		.name = "sp0838",
	},
};

static int __init sp0838_register(void)
{
	return i2c_add_driver(&sp0838_driver);
}

module_init(sp0838_register);

