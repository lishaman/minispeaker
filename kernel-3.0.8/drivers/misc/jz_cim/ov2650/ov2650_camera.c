#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "ov2650_camera.h"
#include "ov2650_set.h"
#include "../cim_reg.h"

static struct frm_size ov2650_capture_table[]= {
	{2592, 1944},
	{2048, 1536},
	{1600,1200},
	{1280,1024},
	{1280,960},
	{1024,768},
	{800,600},
	{640,480},
	{352,288},
	{320,240},
	{176,144},
};

static struct frm_size ov2650_preview_table[]= {
	{1024,768},
	{800,600},
	{640,480},
	/* {352,288}, */
	{320,240},
	{176,144},
};

int cam_t_j = 0, cam_t_i = 0;
unsigned long long cam_t0_buf[10];
unsigned long long cam_t1_buf[10];

static inline int sensor_i2c_master_send(struct i2c_client *client,const char *buf ,int count)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
#if 1
	{
		if (cam_t_i < 10)
			cam_t0_buf[cam_t_i] = cpu_clock(smp_processor_id());
	}
#endif

	ret = i2c_transfer(adap, &msg, 1);

#if 1
	{
		if (cam_t_i < 10) { 
			cam_t1_buf[cam_t_i] = cpu_clock(smp_processor_id());
			cam_t_i++;
		}
		if (cam_t_i == 10) {
			cam_t_j = cam_t_i;
			cam_t_i = 11;
			while(--cam_t_j)
				printk("cam%d : i2c2_time 0  = %lld, i2c2_time 1  = %lld, time = %lld\n",cam_t_j, cam_t0_buf[cam_t_j], cam_t1_buf[cam_t_j], cam_t1_buf[cam_t_j] - cam_t0_buf[cam_t_j]);
		}
	}
#endif


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

int ov2650_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val)
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

unsigned char ov2650_read_reg(struct i2c_client *client,unsigned short reg)
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

int ov2650_power_up(struct cim_sensor *sensor_info)
{
	struct ov2650_sensor *s;
	s = container_of(sensor_info, struct ov2650_sensor, cs);
	dev_info(&s->client->dev,"ov2650-----------power up\n");
	gpio_set_value(s->gpio_en,0);
	mdelay(20);
	return 0;
}

int ov2650_power_down(struct cim_sensor *sensor_info)
{
	struct ov2650_sensor *s;
	s = container_of(sensor_info, struct ov2650_sensor, cs);
	dev_info(&s->client->dev,"ov2650-----------power down\n");
	gpio_set_value(s->gpio_en,1);
	mdelay(20);
	return 0;
}

int ov2650_reset(struct cim_sensor *sensor_info)
{
	struct ov2650_sensor *s;
	s = container_of(sensor_info, struct ov2650_sensor, cs);
	dev_info(&s->client->dev,"ov2650-----------reset %x\n",s->gpio_rst);
	gpio_set_value(s->gpio_rst,0);
	mdelay(100);
	gpio_set_value(s->gpio_rst,1);
	mdelay(100);
	return 0;
}

int ov2650_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct ov2650_sensor *s;
	s = container_of(sensor_info, struct ov2650_sensor, cs);
	ov2650_power_up(sensor_info);
	ov2650_reset(sensor_info);
	mdelay(10);
	retval=ov2650_read_reg(s->client,0x300A);
	ov2650_power_down(sensor_info);

	if(retval == 0x26)//read id,ov2650 id is 0x26
		return 0;
	dev_info(&s->client->dev,"ov2650 sensor probe fail %x\n",retval);
	return -1;
}

int ov2650_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int ov2650_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int ov2650_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ov2650_sensor * s;
	struct ov2650_platform_data *pdata;
	s = kzalloc(sizeof(struct ov2650_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "ov2650");
	//s->cs.cim_cfg = CIM_CFG_DSM_GCM |CIM_CFG_VSP |CIM_CFG_PACK_Y1VY0U;//CIM_CFG_PCP |
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_Y1VY0U | CIM_CFG_ORDER_UYVY;//CIM_CFG_PCP ||CIM_CFG_VSP
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT 
							| WHITE_BALANCE_INCANDESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA|EFFECT_AQUA|EFFECT_BLACKBOARD|EFFECT_PASTEL;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = ov2650_preview_table,
	s->cs.capture_size = ov2650_capture_table,
	s->cs.prev_resolution_nr = ARRAY_SIZE(ov2650_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(ov2650_capture_table);
	
	s->cs.probe = ov2650_sensor_probe;
	s->cs.init = ov2650_init;
	s->cs.reset = ov2650_reset;
	s->cs.power_on = ov2650_power_up;
	s->cs.shutdown = ov2650_power_down;
	s->cs.af_init = ov2650_none;
	s->cs.start_af = ov2650_none;
	s->cs.stop_af = ov2650_none;
	
	s->cs.set_preivew_mode = ov2650_preview_set;
	s->cs.set_capture_mode = ov2650_capture_set;
	s->cs.set_video_mode = ov2650_none,
	s->cs.set_resolution = ov2650_size_switch;
	s->cs.set_balance = ov2650_set_balance;
	s->cs.set_effect = ov2650_set_effect;
	s->cs.set_antibanding = ov2650_set_antibanding;
	s->cs.set_flash_mode = ov2650_none2;
	s->cs.set_scene_mode = ov2650_none2;
	s->cs.set_focus_mode = ov2650_none2;
	s->cs.set_fps = ov2650_none2;
	s->cs.set_nightshot = ov2650_none2;
	s->cs.set_luma_adaption = ov2650_none2;
	s->cs.set_brightness = ov2650_none2;
	s->cs.set_contrast = ov2650_none2;
   
	s->cs.private  = NULL,
				
				
	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		printk("err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(!gpio_request(pdata->gpio_en, "ov2650_en"))
		s->gpio_en= pdata->gpio_en;

	s->gpio_rst = pdata->gpio_rst;
	if(gpio_is_valid(s->gpio_rst))
		printk("  gpio is valid\n");
	if(gpio_request(pdata->gpio_rst, "ov2650_rst"))
	{	
		printk(" ------------------gpio fail\n");
		
	}
	gpio_direction_output(s->gpio_rst,0);
	gpio_direction_output(s->gpio_en,1);
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set ov2650 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id ov2650_id[] = {
	{ "ov2650", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, ov2650_id);

static struct i2c_driver ov2650_driver = {
	.probe		= ov2650_probe,
	.id_table	= ov2650_id,
	.driver	= {
		.name = "ov2650",
	},
};

static int __init ov2650_register(void)
{
	return i2c_add_driver(&ov2650_driver);
}

module_init(ov2650_register);

