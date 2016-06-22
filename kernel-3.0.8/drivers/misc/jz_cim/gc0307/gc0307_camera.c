#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "gc0307_camera.h"
#include "gc0307_set.h"
#include "../cim_reg.h"

//#define OV3640_KERNEL_PRINT

static struct frm_size gc0307_capture_table[]= {
	{640,480},
//	{352,288},
//	{320,240},
//	{176,144},
};

static struct frm_size gc0307_preview_table[]= {
	{640,480},
//	{352,288},
//	{320,240},
//	{176,144},
};

int gc0307_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val)
{
	 unsigned char buffer[2];
        int rc;

        buffer[0] = reg;
        buffer[1] = val;
        if (2 != (rc = i2c_master_send(client, buffer, 2)))
                printk("i2c i/o error: rc == %d (should be 2)\n", rc);

	return 0;
}

unsigned char gc0307_read_reg(struct i2c_client *client,unsigned char reg)
{
        unsigned char buffer[1];
        int rc;
	printk("reg = 0x%x\n",reg);
        buffer[0] = reg;
        if (1 != (rc = i2c_master_send(client, buffer, 1)))
                printk("i2c i/o error: rc == %x (should be 1)\n", rc);
        msleep(10);
        if (1 != (rc = i2c_master_recv(client, buffer, 1)))
                printk("i2c i/o error: rc == %x (should be 1)\n", rc);
        return (buffer[0]);
}

int gc0307_sensor_probe(struct cim_sensor *sensor_info)
{
	int retval = 0;
	struct gc0307_sensor *s;
	s = container_of(sensor_info, struct gc0307_sensor, cs);
	retval=gc0307_read_reg(s->client,0x0);
	if(retval == 0x99)//read id,gc0307 id is 0x99
		return 0;
	dev_err(&s->client->dev,"gc0307 sensor probe fail 0x%x\n",retval);
	return -1;
}

int gc0307_none(struct cim_sensor  *sensor_info)
{
	return 0;
}
int gc0307_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}


static int gc0307_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct gc0307_sensor * s;
	struct cam_sensor_plat_data *pdata;
	s = kzalloc(sizeof(struct gc0307_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "gc0307");
//	s->cs.cim_cfg = CIM_CFG_DSM_GCM | CIM_CFG_PACK_Y1VY0U ;//| CIM_CFG_ORDER_UYVY;//CIM_CFG_PCP ||CIM_CFG_VSP

	s->cs.cim_cfg = CIM_CFG_DSM_GCM | CIM_CFG_PACK_VY1UY0;//3 << 4;//CIM_CFG_ORDER_YUYV; //| (2 << 18) | (2 << 4);//CIM_CFG_PACK_VY1UY0; // | CIM_CFG_ORDER_UYVY;
        //s->cs.cim_cfg |= CIM_CFG_PCP;

	s->cs.modes.balance =  WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT
		| WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect =	EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA;
	s->cs.modes.antibanding =  ANTIBANDING_50HZ | ANTIBANDING_60HZ ;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED;
	s->cs.modes.fps = FPS_MODE_25;
	s->cs.preview_size = gc0307_preview_table,
		s->cs.capture_size = gc0307_capture_table,
		s->cs.prev_resolution_nr = ARRAY_SIZE(gc0307_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(gc0307_capture_table);

	s->cs.probe = gc0307_sensor_probe;
	s->cs.init = gc0307_init;
	s->cs.reset = gc0307_none;
	s->cs.power_on = gc0307_none;
	s->cs.shutdown = gc0307_none;
	s->cs.af_init = gc0307_none;
	s->cs.start_af = gc0307_none;
	s->cs.stop_af = gc0307_none;
	s->cs.read_all_regs = gc0307_none;

	s->cs.set_preivew_mode = gc0307_preview_set;
	s->cs.set_capture_mode = gc0307_capture_set;
	s->cs.set_video_mode = gc0307_none,
		s->cs.set_resolution = gc0307_size_switch;
	s->cs.set_balance = gc0307_set_balance;
	s->cs.set_effect = gc0307_set_effect;
	s->cs.set_antibanding = gc0307_set_antibanding;
	s->cs.set_flash_mode = gc0307_none2;
	s->cs.set_scene_mode = gc0307_none2;
	s->cs.set_focus_mode = gc0307_none2;
	s->cs.set_fps = gc0307_none2;
	s->cs.set_nightshot = gc0307_none2;
	s->cs.set_luma_adaption = gc0307_none2;
	s->cs.set_brightness = gc0307_none2;
	s->cs.set_contrast = gc0307_none2;
	s->cs.private  = NULL,
	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}
	if(gpio_request(GPIO_PD(24),"fp_led_en"))
	{
		dev_err(&client->dev,"request fp_led_en fail\n");
		return -1;
	}
	gpio_direction_output(GPIO_PD(24),1);
	gpio_set_value(GPIO_PD(24),1);
	
	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id gc0307_id[] = {
	{ "gc0307", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, gc0307_id);

static struct i2c_driver gc0307_driver = {
	.probe		= gc0307_probe,
	.id_table	= gc0307_id,
	.driver	= {
		.name = "gc0307",
		.owner = THIS_MODULE,
	},
};

static int __init gc0307_register(void)
{
	return i2c_add_driver(&gc0307_driver);
}

module_init(gc0307_register);

