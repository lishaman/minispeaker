#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <mach/jz_cim.h>
#include "adv7180_camera.h"
#include "adv7180_set.h"
#include "adv7180_reg.h"
#include "../cim_reg.h"

#define adv7180_DEBUG
#ifdef adv7180_DEBUG
#define dprintk(x...)   do{printk("adv7180---\t");printk(x);printk("\n");}while(0)
#else
#define dprintk(x...)
#endif

static struct frm_size adv7180_capture_table[] = {
	{720,576},
	{720,536},
	{640,480},
};
static struct frm_size adv7180_preview_table[] = {
	{720,576},
	{720,536},
	{640,480},
};

void adv7180_power_down(struct cim_sensor * sensor_info)
{
#if 0
	struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);
        printk("adv7180 power down\n");
        gpio_set_value(s->gpio_en,0);
        msleep(50);
#endif
}

void adv7180_power_up(struct cim_sensor * sensor_info)
{
#if 0
	struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);

        printk("adv7180 power up,s->gpio_en = %d\n",s->gpio_en);

        gpio_set_value(s->gpio_en,1);
        msleep(50);
#endif

}

void adv7180_reset(struct cim_sensor * sensor_info)
{
#if 0
	struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);

        printk("adv7180 reset %d\n",s->gpio_rst);

        gpio_set_value(s->gpio_rst,0);
        msleep(250);
        gpio_set_value(s->gpio_rst,1);
        msleep(250);
#endif
}


int adv7180_set_balance(struct cim_sensor * sensor_info,int arg)
{
	printk("====%s\n",__func__);
	return 0;
}

int adv7180_set_effect(struct cim_sensor * sensor_info,int arg)
{
	printk("====%s\n",__func__);
	return 0;
}

int adv7180_set_antibanding(struct cim_sensor * sensor_info,int arg)
{
	printk("====%s\n",__func__);
	return 0;
}


int adv7180_set_flash_mode(struct cim_sensor * sensor_info,int arg)
{
	printk("====%s\n",__func__);
	return 0;
}

int adv7180_set_scene_mode(struct cim_sensor *sensor_info,int arg)
{

	printk("====%s\n",__func__);
	return 0;
}

int adv7180_set_focus_mode(struct cim_sensor * sensor_info,int arg)
{

	printk("====%s\n",__func__);
	return 0;
}


int adv7180_set_fps(int fps)
{

	printk("====%s\n",__func__);
	return 0;
}

int adv7180_set_luma_adaptation(int arg)
{
	printk("====%s\n",__func__);
	return 0;
}

int adv7180_set_parameter(int cmd, int mode, int arg)
{
#if 0
	switch(cmd)
	{
		case CPCMD_SET_BALANCE :
			adv7180_set_balance(mode,arg);
			break;
		case CPCMD_SET_EFFECT :
			adv7180_set_effect(mode,arg);
			break;
		case CPCMD_SET_ANTIBANDING :
			adv7180_set_antibanding(mode,arg);
			break;
		case CPCMD_SET_FLASH_MODE :
			adv7180_set_flash_mode(mode,arg);
			break;
		case CPCMD_SET_SCENE_MODE :
			adv7180_set_scene_mode(mode,arg);
			break;
		case CPCMD_SET_PIXEL_FORMAT :
			break;
		case CPCMD_SET_FOCUS_MODE :
			adv7180_set_focus_mode(mode,arg);
			break;
		case CPCMD_SET_PREVIEW_FPS:
			adv7180_set_fps(arg);
			break;
		case CPCMD_SET_NIGHTSHOT_MODE:
			break;
		case CPCMD_SET_LUMA_ADAPTATION:
			adv7180_set_luma_adaptation(arg);
			break;
	}
#endif
	return 0;
}

int adv7180_set_power(int state)
{
	return 0;
}
int adv7180_capture_set(struct cim_sensor * sensor_info)
{
        printk("===>%s\n",__func__);
        struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);
        capture_set(s->client);
	return 0;
}
int adv7180_preview_set(struct cim_sensor * sensor_info)
{

	printk("===>%s\n",__func__);
	struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);
	preview_set(s->client);
	return 0;
}

int adv7180_sensor_init(struct cim_sensor * sensor_info)
{
	printk("===>%s() L%d\n", __func__, __LINE__);
	printk("===>init ADV7180\n");
	struct adv7180_sensor *s;
	s = container_of(sensor_info, struct adv7180_sensor, cs);
	adv7180_power_up(sensor_info);
	adv7180_reset(sensor_info);
	u8 standard = 0;
	char buf[4] = { 0 };
	adv7180_set_reg(s->client,ADV7180_INPUT_CONTROL_REG,0x00);
	adv7180_set_reg(s->client,ADV7180_EXTENDED_OUTPUT_CONTROL_REG,0x57);
	adv7180_set_reg(s->client,ADV7180_FILTER_CTL,0x41);
	adv7180_set_reg(s->client,ADV7180_FILED_CTL,0x02);
	adv7180_set_reg(s->client,ADV7180_MANUAL_WINDOW_CTL,0xA2);
	adv7180_set_reg(s->client,ADV7180_BLM_OPT,0x6A);
	adv7180_set_reg(s->client,ADV7180_BGB_OPT,0xA0);
	adv7180_set_reg(s->client,ADV7180_USR_SPA,0x80);
	adv7180_set_reg(s->client,ADV7180_ADC_CFG,0x81);
	adv7180_set_reg(s->client,ADV7180_USR_SPA,0x00);

	buf[0] = 0x10;
	adv7180_read(s->client,buf,1,buf,1);
	standard = buf[0];

	sensor_info->para.standard = PAL;

	if(standard == 13||standard == 15)         //for NTSC M/J
	{
		sensor_info->para.standard = NTSC;
		printk("sensor_info->para.standard = %d\n",sensor_info->para.standard);
		printk("NTSC mode\n");

		adv7180_set_reg(s->client,0x31,0x1A);
		adv7180_set_reg(s->client,0x32,0x81);
		adv7180_set_reg(s->client,0x33,0x84);
		adv7180_set_reg(s->client,0x34,0x00);
		adv7180_set_reg(s->client,0x35,0x00);
		adv7180_set_reg(s->client,0x36,0x7D);
		adv7180_set_reg(s->client,0x37,0x09);
		adv7180_set_reg(s->client,0xE5,0x85);
		adv7180_set_reg(s->client,0xE6,0x44);
		adv7180_set_reg(s->client,0xE7,0x03);

	}
		mdelay(5);
		return 0;
}


int adv7180_sensor_probe(struct cim_sensor *sensor_info)
{
	printk("====>%s\n",__func__);
	struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);
        adv7180_power_up(sensor_info);
        adv7180_reset(sensor_info);
	u8 value = 0;
	char buf[4] = { 0 };
	buf[0] = ADV7180_IDENT_REG;
	adv7180_read(s->client,buf,1,buf,1);
	value = buf[0];
	printk("buf0:%d %d %d %d\n",buf[0],buf[1],buf[2],buf[3]);
	buf[0] = 0x10;
	adv7180_read(s->client,buf,1,buf,1);
	printk("buf1:%d %d %d %d\n",buf[0],buf[1],buf[2],buf[3]);


	adv7180_power_down(sensor_info);
	if (value == 0x1E) { /* Is ADV */
		return 0;
	}
	return -1;
}

/* sensor_set_function use for init preview or capture.there may be some difference between preview and capture.
 * so we divided it into two sequences.param: function indicated which function
 * 0: preview
 * 1: capture
 * 2: recording
 */
int adv7180_set_function(int function, void *cookie)
{
#if 0
	switch (function)
	{
		case 0:
			preview_set(adv7180_sensor_desc.client);
			break;
		case 1:
			capture_set(adv7180_sensor_desc.client);
			break;
		case 2:
			break;
	}
#endif
	return 0;
}

int adv7180_set_resolution(struct cim_sensor * sensor_info,int width,int height)//camera_mode_t mode)
{

	struct adv7180_sensor *s;
        s = container_of(sensor_info, struct adv7180_sensor, cs);

	size_switch(s->client, width, height);
	return 0;
}
int adv7180_none (struct cim_sensor * sensor_info)
{
	return 0;
}
int adv7180_none2(struct cim_sensor * sensor_info,unsigned short arg)
{
	return 0;
}
static int adv7180_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	printk("\nadv7180_i2c_probe............\n");
	struct adv7180_sensor * s;
        struct cam_sensor_plat_data *pdata;
        s = kzalloc(sizeof(struct adv7180_sensor), GFP_KERNEL);

	strcpy(s->cs.name , "adv7180");
	s->cs.cim_cfg = 0x0| (2 << 18)//CIM_CFG_ORDER_UYVY
            //    |CIM_CFG_PACK_Y0UY1V                 /* pack mode :0x 4 3 2 1*/
                |(2 << 4)                 /* pack mode :0x 4 3 2 1*/
#if !defined(CONFIG_SOC_JZ4780)
          //      |CIM_CFG_BYPASS                 /* Bypass Mode */
#endif
                //|CIM_CFG_PCP                  /* PCLK working edge:1-falling */
#if     0 //ndef CONFIG_JZ_CIM_IN_FMT_ITU656
                |CIM_CFG_DSM_GCM,               // Gated Clock Mode
#else
                 | CIM_CFG_DF_ITU656
                 | CIM_CFG_DSM_CIM;            // CCIR656 Interlace Mode
#endif
//        	|1 << 8 |  1 << 3             // CCIR656 Progressive Mode
//        	  | 0;
//	adv7180_sensor_desc.client = client;
//	sensor_set_i2c_speed(client,400000);// set ov3640 i2c speed : 400khz
//	ret = camera_sensor_register(&adv7180_sensor_desc);
//	dump_stack();

	s->cs.preview_size = adv7180_preview_table,
	s->cs.capture_size = adv7180_capture_table,
        s->cs.prev_resolution_nr = ARRAY_SIZE(adv7180_preview_table);
        s->cs.cap_resolution_nr = ARRAY_SIZE(adv7180_capture_table);
	s->cs.probe = adv7180_sensor_probe;
        s->cs.init = adv7180_sensor_init;
	s->cs.reset = adv7180_reset;
        s->cs.power_on = adv7180_power_up;
	s->cs.shutdown = adv7180_power_down;
        s->cs.af_init = adv7180_none;
        s->cs.start_af = adv7180_none;
        s->cs.stop_af = adv7180_none;
        //s->cs.read_all_regs = adv7180_read_all_regs;

        s->cs.set_preivew_mode = adv7180_preview_set;
        s->cs.set_capture_mode = adv7180_capture_set;
	s->cs.set_video_mode = adv7180_none,
	s->cs.set_resolution = adv7180_set_resolution;
	s->cs.set_balance = adv7180_set_balance;
	s->cs.set_effect = adv7180_set_effect;
	s->cs.set_antibanding = adv7180_set_antibanding;
	s->cs.set_flash_mode = adv7180_set_flash_mode;
	s->cs.set_scene_mode = adv7180_set_scene_mode;
	s->cs.set_focus_mode = adv7180_set_focus_mode;
	s->cs.set_fps = adv7180_set_fps;
	s->cs.set_nightshot = adv7180_none2;
	s->cs.set_luma_adaption = adv7180_set_luma_adaptation;
	s->cs.set_brightness = adv7180_none2;
	s->cs.set_contrast = adv7180_none2;

	s->client = client;
        pdata = client->dev.platform_data;
        if( client->dev.platform_data == NULL){
                dev_err(&client->dev,"err!!! no camera i2c pdata!!! \n\n");
                return -1;
        }
#if 0
	if(gpio_request(pdata->gpio_en, "adv7180_en") || gpio_request(pdata->gpio_rst, "ov3640_rst")){
                dev_err(&client->dev,"request adv7180 gpio rst and en fail\n");
                return -1;
        }
        s->gpio_en= pdata->gpio_en;
        s->gpio_rst = pdata->gpio_rst;

        if(!(gpio_is_valid(s->gpio_rst) && gpio_is_valid(s->gpio_en))){
                dev_err(&client->dev," adv7180  gpio is invalid\n");
                return -1;
        }

	printk("s->gpio_rst = %d,s->gpio_en = %d\n",s->gpio_rst,s->gpio_en);
        gpio_direction_output(s->gpio_rst,1);
        gpio_direction_output(s->gpio_en,1);
#endif
        s->cs.facing = pdata->facing;
        s->cs.orientation = pdata->orientation;
        s->cs.cap_wait_frame = 2;//2;//pdata->cap_wait_frame;
        //sensor_set_i2c_speed(client,400000);//set ov3640 i2c speed : 400khz
        camera_sensor_register(&s->cs);

	return ret;
}
static const struct i2c_device_id adv7180_id[] = {
	{ "adv7180", 0 },
	{ }    /* Terminating entry */
};
MODULE_DEVICE_TABLE(i2c, adv7180_id);

static struct i2c_driver adv7180_driver = {
	.probe		= adv7180_i2c_probe,
	.id_table	= adv7180_id,
	.driver	= {
		.name = "adv7180",
		.owner = THIS_MODULE,
	},
};

static int __init adv7180_register(void)
{
	int ret;

	printk("-----------adv7180_register----------\n");

	ret = i2c_add_driver(&adv7180_driver);

	if (ret) {
		printk(KERN_WARNING "Adding adv7180 driver failed ");
	}else {
		 pr_info("Successfully added driver %s\n",
				 adv7180_driver.driver.name);
	}
	return ret;
}
module_init(adv7180_register);

