#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "ov5640_camera.h"
#include "ov5640_set.h"
#include "../cim_reg.h"

extern u16 g_chipid;	// 0x5640 or 0x5642

static struct frm_size ov5640_capture_table[]= {
        { 2592, 1944 },         //5MP
        { 1920, 1080 },         //1080P
        { 1280,  720 },         //720P
        {  640,  480 },         //VGA
        {  320,  240 },         //QVGA
};

static struct frm_size ov5640_preview_table[]= {
        { 1920, 1080 },         //1080P
        { 1280,  720 },         //720P
        {  640,  480 },         //VGA
        {  320,  240 },         //QVGA
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

int ov5640_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val)
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

unsigned char ov5640_read_reg(struct i2c_client *client,unsigned short reg)
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

int ov5640_power_up(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);

	if (gpio_is_valid(s->gpio_en)) {
		dev_info(&s->client->dev,"ov5640_power_up: GP%c%d\n", 'A'+s->gpio_en/32, s->gpio_en%32);

		gpio_set_value(s->gpio_en, 0);	//active low
		mdelay(50);
	}
	return 0;
}

int ov5640_power_down(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);

	if (gpio_is_valid(s->gpio_en)) {
		dev_info(&s->client->dev,"ov5640_power_down: GP%c%d\n", 'A'+s->gpio_en/32, s->gpio_en%32);

		gpio_set_value(s->gpio_en, 1);
		mdelay(50);
	}
	return 0;
}

int ov5640_reset(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s = container_of(sensor_info, struct ov5640_sensor, cs);

	dev_info(&s->client->dev,"ov5640_reset: GP%c%d\n", 'A'+s->gpio_rst/32, s->gpio_rst%32);

	gpio_set_value(s->gpio_rst,0);
	mdelay(150);
	gpio_set_value(s->gpio_rst,1);
	mdelay(150);
	return 0;
}

int ov5640_sensor_probe(struct cim_sensor *sensor_info)
{
	struct ov5640_sensor *s;
	u8 chipid_high;
	u8 chipid_low;
	u8 revision;
	int ret = 0;

	s = container_of(sensor_info, struct ov5640_sensor, cs);
	ov5640_power_up(sensor_info);
	ov5640_reset(sensor_info);
	mdelay(10);

	/*
	 * check and show chip ID
	 */
	chipid_high = ov5640_read_reg(s->client, 0x300a);
	chipid_low = ov5640_read_reg(s->client, 0x300b);
	revision = ov5640_read_reg(s->client, 0x302a);

	g_chipid = s->cs.chipid = chipid_high << 8 | chipid_low;

	if ( (s->cs.chipid != 0x5640) && (s->cs.chipid != 0x5642) ) {
		dev_err(&s->client->dev, "read sensor %s ID: 0x%x, Revision: 0x%x, failed!\n", s->client->name, s->cs.chipid, revision);
		ret = -1;
		goto out;
	}

	dev_info(&s->client->dev, "Sensor ID: 0x%x, Revision: 0x%x\n", s->cs.chipid, revision);
	/*
	 * read Module ID
	 */
	ov5640_read_otp(sensor_info);

out:
	ov5640_power_down(sensor_info);
	return ret;
}

int ov5640_none(struct cim_sensor  *sensor_info)
{
	return 0;
}

int ov5640_none2(struct cim_sensor * desc,unsigned short arg)
{
	return 0;
}

static int ov5640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ov5640_sensor *s = NULL;
	struct ov5640_platform_data *pdata = NULL;

	s = kzalloc(sizeof(struct ov5640_sensor), GFP_KERNEL);
	if (s == NULL) {
		pr_err("%s L%d: kzalloc failed!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	strcpy(s->cs.name , "ov5640");
	//s->cs.cim_cfg = CIM_CFG_DSM_GCM |CIM_CFG_VSP |CIM_CFG_PACK_Y1VY0U;//CIM_CFG_PCP |
	s->cs.cim_cfg = CIM_CFG_DSM_GCM  | CIM_CFG_PACK_VY1UY0; // | CIM_CFG_ORDER_UYVY;
	s->cs.cim_cfg |= CIM_CFG_PCP;
	s->cs.modes.balance = WHITE_BALANCE_AUTO | WHITE_BALANCE_DAYLIGHT | WHITE_BALANCE_CLOUDY_DAYLIGHT | WHITE_BALANCE_INCANDESCENT | WHITE_BALANCE_FLUORESCENT;
	s->cs.modes.effect = EFFECT_NONE|EFFECT_MONO|EFFECT_NEGATIVE|EFFECT_SEPIA|EFFECT_AQUA|EFFECT_PASTEL;
	s->cs.modes.antibanding = ANTIBANDING_50HZ | ANTIBANDING_60HZ | ANTIBANDING_AUTO | ANTIBANDING_OFF;
	//s->cs.modes.flash_mode = FLASH_MODE_OFF;
	s->cs.modes.scene_mode = SCENE_MODE_AUTO;
	s->cs.modes.focus_mode = FOCUS_MODE_FIXED | FOCUS_MODE_AUTO;
	s->cs.modes.fps = FPS_MODE_25;//FPS_MODE_15 | FPS_MODE_20 |
	s->cs.preview_size = ov5640_preview_table,
	s->cs.capture_size = ov5640_capture_table,
	s->cs.prev_resolution_nr = ARRAY_SIZE(ov5640_preview_table);
	s->cs.cap_resolution_nr = ARRAY_SIZE(ov5640_capture_table);

	s->cs.probe = ov5640_sensor_probe;
	s->cs.init = ov5640_init;
	s->cs.reset = ov5640_reset;
	s->cs.power_on = ov5640_power_up;
	s->cs.shutdown = ov5640_power_down;
	s->cs.af_init = ov5640_none;
	s->cs.start_af = ov5640_none;
	s->cs.stop_af = ov5640_none;

	s->cs.set_preivew_mode = ov5640_preview_set;
	s->cs.set_capture_mode = ov5640_capture_set;
	s->cs.set_video_mode = ov5640_none,
	s->cs.set_resolution = ov5640_size_switch;
	s->cs.set_balance = ov5640_set_balance;
	s->cs.set_effect = ov5640_set_effect;
	s->cs.set_antibanding = ov5640_set_antibanding;
	s->cs.set_flash_mode = ov5640_none2;
	s->cs.set_scene_mode = ov5640_none2;
	s->cs.set_focus_mode = ov5640_set_focus;
	s->cs.set_fps = ov5640_none2;
	s->cs.set_nightshot = ov5640_none2;
	s->cs.set_luma_adaption = ov5640_none2;
	s->cs.set_brightness = ov5640_none2;
	s->cs.set_contrast = ov5640_none2;

	s->cs.private  = NULL;

//	INIT_DELAYED_WORK(&s->work, ov5640_late_work);
//	ov5640_work_queue = create_singlethread_workqueue("ov5640_work_queue");
//	if (ov5640_work_queue == NULL) {
//		pr_err("%s L%d: error!\n", __func__, __LINE__);
//		return -ENOMEM;
//	}

	s->client = client;
	pdata = client->dev.platform_data;
	if( client->dev.platform_data == NULL){
		printk("err!!! no camera i2c pdata!!! \n\n");
		return -1;
	}

	s->gpio_en = pdata->gpio_en;
	s->gpio_rst = pdata->gpio_rst;
	s->gpio_pwdn = pdata->gpio_pwdn;

	if(!gpio_is_valid(pdata->gpio_en)) {
		printk("invalid gpio_en GP%c%d! \n",
			'A'+pdata->gpio_en/32, pdata->gpio_en%32);
	} else {
		if(gpio_request(pdata->gpio_en, "ov5640_en")) {
			printk("fail to request gpio_en GP%c%d!\n",
					'A'+pdata->gpio_en/32, pdata->gpio_en%32);
		} else {
			s->gpio_en = pdata->gpio_en;
			gpio_direction_output(s->gpio_en,1);
		}
	}

	if(!gpio_is_valid(pdata->gpio_pwdn)) {
		printk("invalid gpio_pwdn GP%c%d! \n",
			'A'+pdata->gpio_pwdn/32, pdata->gpio_pwdn%32);
	} else {
		if(gpio_request(pdata->gpio_pwdn, "ov5640_pwdn")) {
			printk("fail to request gpio_pwdn GP%c%d!\n",
				'A'+pdata->gpio_pwdn/32, pdata->gpio_pwdn%32);
		} else {
			s->gpio_pwdn = pdata->gpio_pwdn;
			gpio_direction_output(s->gpio_pwdn,0);
		}
	}

	if(!gpio_is_valid(pdata->gpio_rst)) {
		printk("invalid gpio_rst GP%c%d! \n",
			'A'+pdata->gpio_rst/32, pdata->gpio_rst%32);
	} else {
		if(gpio_request(pdata->gpio_rst, "ov5640_rst")) {
			printk("fail to request gpio_rst GP%c%d!\n",
				'A'+pdata->gpio_rst/32, pdata->gpio_rst%32);
		} else {
			s->gpio_rst = pdata->gpio_rst;
			gpio_direction_output(s->gpio_rst,1);
		}
	}

	s->cs.facing = pdata->facing;
	s->cs.orientation = pdata->orientation;
	s->cs.cap_wait_frame = pdata->cap_wait_frame;
	//sensor_set_i2c_speed(client,400000);//set ov5640 i2c speed : 400khz
	camera_sensor_register(&s->cs);

	return 0;
}

static const struct i2c_device_id ov5640_id[] = {
	{ "ov5640", 0 },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE(i2c, ov5640_id);

static struct i2c_driver ov5640_driver = {
	.probe		= ov5640_probe,
	.id_table	= ov5640_id,
	.driver	= {
		.name = "ov5640",
	},
};

static int __init ov5640_register(void)
{
	return i2c_add_driver(&ov5640_driver);
}

module_init(ov5640_register);

