#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "gt2005_camera.h"
#include "gt2005_set.h"
#include "../cim_reg.h"

static struct frm_size gt2005_capture_table[]={
	{640,480},
	{352,288},
	{320,240},
	{176,144},
};

static struct frm_size gt2005_preview_table[]={
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
//	printk("i2c send : msg.addr = 0x%x\n",msg.addr);
//	printk("i2c send : ret = %d\n",ret);
//	printk("i2c_send : count = %d\n",count);

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
//	printk("i2c recv : msg.addr = 0x%x\n",msg.addr);
//	printk("i2c recv : ret = %d\n",ret);
//	printk("i2c_recv : count = %d\n",count);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

int gt2005_write_reg16(struct i2c_client *client,unsigned short reg, unsigned char val)
{
        unsigned char msg[3],tmp;
        int ret;

        memcpy(&msg[0], &reg, 2);
        memcpy(&msg[2], &val, 1);

        tmp=msg[0];
        msg[0]=msg[1];
        msg[1]=tmp;

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

unsigned char gt2005_read_reg16(struct i2c_client *client,unsigned short reg)
{
        unsigned char retval;
        unsigned char msg[2],tmp;
        int ret;

        memcpy(&msg[0], &reg, 2);

        tmp=msg[0];
        msg[0]=msg[1];
        msg[1]=tmp;


        ret = sensor_i2c_master_send(client,msg,2);

        if (ret < 0)
                return ret;
        if (ret != 2)
                return -EIO;

        ret = sensor_i2c_master_recv(client, &retval, 1);
        if (ret < 0)
                return ret;
        if (ret != 1)
                return -EIO;

        printk("read_value = %x\n",(unsigned int)retval);
        return retval;
}

void gt2005_read_all_regs(struct cim_sensor *sensor_info)
{

}

int gt2005_power_up(struct cim_sensor *sensor_info)
{
	struct gt2005_sensor *s;
	s = container_of(sensor_info, struct gt2005_sensor, cs);

	gpio_set_value(s->gpio_en, 1);
	msleep(100);
	return 0;
}

int gt2005_power_down(struct cim_sensor *sensor_info)
{
	struct gt2005_sensor *s;
	s = container_of(sensor_info, struct gt2005_sensor, cs);
	
	gpio_set_value(s->gpio_en, 0);
	msleep(100);
	return 0;
}

int gt2005_reset(struct cim_sensor *sensor_info)
{
	struct gt2005_sensor *s;
	s = container_of(sensor_info, struct gt2005_sensor, cs);
//#ifdef GT2005_KERNEL_PRINT
	dev_info(&s->client->dev,"gt2005 reset %x\n",s->gpio_rst);
//#endif
	gpio_set_value(s->gpio_rst, 0);
	msleep(100);
	gpio_set_value(s->gpio_rst, 1);
	msleep(100);
	return 0;
}

int gt2005_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct gt2005_sensor *s;
	printk("gt2005_sensor_probe_2 ------\n");
	s = container_of(sensor_info, struct gt2005_sensor, cs);
	gt2005_power_up(sensor_info);
	gt2005_reset(sensor_info);

	int sensor_id = 0;
        sensor_id = (gt2005_read_reg16(s->client,0x0000) << 8) | gt2005_read_reg16(s->client,0x0001);
        printk("-----------------------------gt2005 read is 0x%04x\n",sensor_id);
	//while(1){}
        gt2005_power_down(sensor_info);
        if(sensor_id == 0x5138)
                return 0;
        return -1;

}

int gt2005_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int gt2005_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}

static int gt2005_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct gt2005_sensor *s;
	struct gt2005_platform_data *pdata;
	s = kzalloc(sizeof(struct gt2005_sensor),GFP_KERNEL);
	
	strcpy(s->cs.name , "gt2005");
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_Y1VY0U | CIM_CFG_ORDER_YUYV; // | CIM_CFG_PCP | CIM_CFG_VSP;
	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = gt2005_preview_table,
		s->cs.capture_size = gt2005_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(gt2005_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(gt2005_capture_table);

	s->cs.probe = gt2005_sensor_probe;
	s->cs.init = gt2005_init;
	s->cs.reset = gt2005_reset;
	s->cs.power_on = gt2005_power_up;
	s->cs.shutdown = gt2005_power_down;
	s->cs.af_init = gt2005_none;
	s->cs.start_af = gt2005_none;
	s->cs.stop_af = gt2005_none;
	s->cs.read_all_regs = gt2005_read_all_regs;

	s->cs.set_preivew_mode = gt2005_preview_set;
	s->cs.set_capture_mode = gt2005_capture_set;
	s->cs.set_video_mode = gt2005_none,
	s->cs.set_resolution = gt2005_size_switch;
	s->cs.set_balance = gt2005_set_balance;
	s->cs.set_effect = gt2005_set_effect;
	s->cs.set_antibanding = gt2005_set_antibanding;
	s->cs.set_flash_mode = gt2005_none2;
	s->cs.set_scene_mode = gt2005_none2;
	s->cs.set_focus_mode = gt2005_none2;
	s->cs.set_fps = gt2005_none2;
	s->cs.set_nightshot = gt2005_none2;
	s->cs.set_luma_adaption = gt2005_none2;
	s->cs.set_brightness = gt2005_none2;
	s->cs.set_contrast = gt2005_none2;

	s->cs.private  = NULL,
	s->client = client;


	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	printk("gpio_en = %d ******  gpio_rst = %d\n",pdata->gpio_en,pdata->gpio_rst);
	//if(gpio_request(pdata->gpio_en, "gt2005_en") || gpio_request(pdata->gpio_rst, "gt2005_rst")){
	//	dev_err(&client->dev,"request gt2005 gpio rst and en fail\n");
	//	return -1;
	//}
	
	if(gpio_request(pdata->gpio_rst, "gt2005_rst")){
		dev_err(&client->dev,"request gt2005 gpio rst fail\n");
                return -1;
	}
	
	if(gpio_request(pdata->gpio_en, "gt2005_en")){
		dev_err(&client->dev,"request gt2005 gpio en fail\n");
		return -1;
	}


	s->gpio_en= pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;

	if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
		dev_err(&client->dev," gt2005  gpio is invalid\n");
		return -1;
	}




	gpio_direction_output(s->gpio_rst, 0);
	gpio_direction_output(s->gpio_en, 1);

	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set gt2005 i2c speed : 400khz
	camera_sensor_register(&s->cs);
	return 0;

}

static const struct i2c_device_id gt2005_id[] = {
	{ "gt2005", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, gt2005_id);

static struct i2c_driver gt2005_driver = {
	.probe		= gt2005_probe,
	.id_table	= gt2005_id,
	.driver	= {
		.name = "gt2005",
	},
};

static int __init gt2005_register(void)
{
	printk("gt2005_init==\n");
	return i2c_add_driver(&gt2005_driver);
}
module_init(gt2005_register);
