#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "ov3640_camera.h"

//#define OV3640_SET_KERNEL_PRINT


int ov3640_init(struct cim_sensor *sensor_info)
{
	struct ov3640_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"ov3640 init\n");
#endif

	ov3640_write_reg(client,0x3012,0x80);
mdelay(5);
	ov3640_write_reg(client,0x304d,0x45);
	ov3640_write_reg(client,0x30a7,0x5e);
	ov3640_write_reg(client,0x3087,0x16);
	ov3640_write_reg(client,0x309C,0x1a);
	ov3640_write_reg(client,0x30a2,0xe4);
	ov3640_write_reg(client,0x30aa,0x42);
	ov3640_write_reg(client,0x30b0,0xff);
	ov3640_write_reg(client,0x30b1,0xff);
	ov3640_write_reg(client,0x30b2,0x10);
	ov3640_write_reg(client,0x300e,0x39);
	ov3640_write_reg(client,0x300f,0x21);
	ov3640_write_reg(client,0x3010,0x20);
	ov3640_write_reg(client,0x304c,0x81);  //pclk
	ov3640_write_reg(client,0x30d7,0x10);
	ov3640_write_reg(client,0x30d9,0x0d);
	ov3640_write_reg(client,0x30db,0x08);
	ov3640_write_reg(client,0x3016,0x82);
	ov3640_write_reg(client,0x3018,0x48);
	ov3640_write_reg(client,0x3019,0x40);
	ov3640_write_reg(client,0x301a,0x82);
	ov3640_write_reg(client,0x307d,0x00);
	ov3640_write_reg(client,0x3087,0x02);
	ov3640_write_reg(client,0x3082,0x20);
	//ov3640_write_reg(client,0x3015,0x12);//zi dong zeng yi
	ov3640_write_reg(client,0x3015,0x11);
	ov3640_write_reg(client,0x3014,0x84);
	ov3640_write_reg(client,0x3013,0xf7);
	ov3640_write_reg(client,0x303c,0x08);
	ov3640_write_reg(client,0x303d,0x18);
	ov3640_write_reg(client,0x303e,0x06);
	ov3640_write_reg(client,0x303F,0x0c);
	ov3640_write_reg(client,0x3030,0x62);
	ov3640_write_reg(client,0x3031,0x26);
	ov3640_write_reg(client,0x3032,0xe6);
	ov3640_write_reg(client,0x3033,0x6e);
	ov3640_write_reg(client,0x3034,0xea);
	ov3640_write_reg(client,0x3035,0xae);
	ov3640_write_reg(client,0x3036,0xa6);
	ov3640_write_reg(client,0x3037,0x6a);
	ov3640_write_reg(client,0x3104,0x02);
	ov3640_write_reg(client,0x3105,0xfd);
	ov3640_write_reg(client,0x3106,0x00);
	ov3640_write_reg(client,0x3107,0xff);
	ov3640_write_reg(client,0x3300,0x13);
	ov3640_write_reg(client,0x3301,0xde);
	ov3640_write_reg(client,0x3302,0xcf);
	ov3640_write_reg(client,0x3312,0x26);
	ov3640_write_reg(client,0x3314,0x42);
	ov3640_write_reg(client,0x3313,0x2b);
	ov3640_write_reg(client,0x3315,0x42);
	ov3640_write_reg(client,0x3310,0xd0);
	ov3640_write_reg(client,0x3311,0xbd);
	ov3640_write_reg(client,0x330c,0x18);
	ov3640_write_reg(client,0x330d,0x18);
	ov3640_write_reg(client,0x330e,0x56);
	ov3640_write_reg(client,0x330f,0x5c);
	ov3640_write_reg(client,0x330b,0x1c);
	ov3640_write_reg(client,0x3306,0x5c);
	ov3640_write_reg(client,0x3307,0x11);
	ov3640_write_reg(client,0x336a,0x52);
	ov3640_write_reg(client,0x3370,0x46);
	ov3640_write_reg(client,0x3376,0x38);
	ov3640_write_reg(client,0x30b8,0x20);
	ov3640_write_reg(client,0x30b9,0x17);
	ov3640_write_reg(client,0x30ba,0x04);
	ov3640_write_reg(client,0x30bb,0x08);
	ov3640_write_reg(client,0x3507,0x06);
	ov3640_write_reg(client,0x350a,0x4f);
	ov3640_write_reg(client,0x3100,0x02);
	ov3640_write_reg(client,0x3301,0xde);
	ov3640_write_reg(client,0x3304,0xfc);
	ov3640_write_reg(client,0x3400,0x00);
	ov3640_write_reg(client,0x3404,0x02);
	ov3640_write_reg(client,0x3600,0xc0);
	ov3640_write_reg(client,0x3088,0x08);
	ov3640_write_reg(client,0x3089,0x00);
	ov3640_write_reg(client,0x308a,0x06);
	ov3640_write_reg(client,0x308b,0x00);
	ov3640_write_reg(client,0x308d,0x04);
	ov3640_write_reg(client,0x3086,0x03);
	ov3640_write_reg(client,0x3086,0x00);
	ov3640_write_reg(client,0x30a9,0xbd);
	ov3640_write_reg(client,0x3317,0x04);
	ov3640_write_reg(client,0x3316,0xf8);
	ov3640_write_reg(client,0x3312,0x17);
	ov3640_write_reg(client,0x3314,0x30);
	ov3640_write_reg(client,0x3313,0x23);
	ov3640_write_reg(client,0x3315,0x3e);
	ov3640_write_reg(client,0x3311,0x9e);
	ov3640_write_reg(client,0x3310,0xc0);
	ov3640_write_reg(client,0x330c,0x18);
	ov3640_write_reg(client,0x330d,0x18);
	ov3640_write_reg(client,0x330e,0x5e);
	ov3640_write_reg(client,0x330f,0x6c);
	ov3640_write_reg(client,0x330b,0x1c);
	ov3640_write_reg(client,0x3306,0x5c);
	ov3640_write_reg(client,0x3307,0x11);
	ov3640_write_reg(client,0x3308,0x25);
	ov3640_write_reg(client,0x3340,0x20);
	ov3640_write_reg(client,0x3341,0x50);
	ov3640_write_reg(client,0x3342,0x18);
	ov3640_write_reg(client,0x3343,0x23);
	ov3640_write_reg(client,0x3344,0xad);
	ov3640_write_reg(client,0x3345,0xd0);
	ov3640_write_reg(client,0x3346,0xb8);
	ov3640_write_reg(client,0x3347,0xb4);
	ov3640_write_reg(client,0x3348,0x04);
	ov3640_write_reg(client,0x3349,0x98);
	ov3640_write_reg(client,0x3355,0x02);
	ov3640_write_reg(client,0x3358,0x44);
	ov3640_write_reg(client,0x3359,0x44);
	ov3640_write_reg(client,0x3300,0x13);
	ov3640_write_reg(client,0x3367,0x23);
	ov3640_write_reg(client,0x3368,0xBB);
	ov3640_write_reg(client,0x3369,0xD6);
	ov3640_write_reg(client,0x336A,0x2A);
	ov3640_write_reg(client,0x336B,0x07);
	ov3640_write_reg(client,0x336C,0x00);
	ov3640_write_reg(client,0x336D,0x23);
	ov3640_write_reg(client,0x336E,0xC3);
	ov3640_write_reg(client,0x336F,0xDE);
	ov3640_write_reg(client,0x3370,0x2b);
	ov3640_write_reg(client,0x3371,0x07);
	ov3640_write_reg(client,0x3372,0x00);
	ov3640_write_reg(client,0x3373,0x23);
	ov3640_write_reg(client,0x3374,0x9e);
	ov3640_write_reg(client,0x3375,0xD6);
	ov3640_write_reg(client,0x3376,0x29);
	ov3640_write_reg(client,0x3377,0x07);
	ov3640_write_reg(client,0x3378,0x00);
	ov3640_write_reg(client,0x332a,0x1d);
	ov3640_write_reg(client,0x331b,0x08);
	ov3640_write_reg(client,0x331c,0x16);
	ov3640_write_reg(client,0x331d,0x2d);
	ov3640_write_reg(client,0x331e,0x54);
	ov3640_write_reg(client,0x331f,0x66);
	ov3640_write_reg(client,0x3320,0x73);
	ov3640_write_reg(client,0x3321,0x80);
	ov3640_write_reg(client,0x3322,0x8c);
	ov3640_write_reg(client,0x3323,0x95);
	ov3640_write_reg(client,0x3324,0x9d);
	ov3640_write_reg(client,0x3325,0xac);
	ov3640_write_reg(client,0x3326,0xb8);
	ov3640_write_reg(client,0x3327,0xcc);
	ov3640_write_reg(client,0x3328,0xdd);
	ov3640_write_reg(client,0x3329,0xee);
	ov3640_write_reg(client,0x332e,0x04);
	ov3640_write_reg(client,0x332f,0x04);
	ov3640_write_reg(client,0x3331,0x02);
	ov3640_write_reg(client,0x3012,0x10);
	ov3640_write_reg(client,0x3023,0x06);
	ov3640_write_reg(client,0x3026,0x03);
	ov3640_write_reg(client,0x3027,0x04);
	ov3640_write_reg(client,0x302a,0x03);
	ov3640_write_reg(client,0x302b,0x10);
	ov3640_write_reg(client,0x3075,0x24);
	ov3640_write_reg(client,0x300d,0x01);
	ov3640_write_reg(client,0x30d7,0x90);
	ov3640_write_reg(client,0x3069,0x04);
	ov3640_write_reg(client,0x303e,0x00);
	ov3640_write_reg(client,0x303f,0xc0);
	ov3640_write_reg(client,0x3302,0xef);
	ov3640_write_reg(client,0x335f,0x34);
	ov3640_write_reg(client,0x3360,0x0c);
	ov3640_write_reg(client,0x3361,0x04);
	ov3640_write_reg(client,0x3362,0x34);
	ov3640_write_reg(client,0x3363,0x08);
	ov3640_write_reg(client,0x3364,0x04);
	ov3640_write_reg(client,0x3403,0x42);
	ov3640_write_reg(client,0x3088,0x04);
	ov3640_write_reg(client,0x3089,0x00);
	ov3640_write_reg(client,0x308a,0x03);
	ov3640_write_reg(client,0x308b,0x00);
	ov3640_write_reg(client,0x300e,0x32);
	ov3640_write_reg(client,0x300f,0x21);
	ov3640_write_reg(client,0x3010,0x20);
	ov3640_write_reg(client,0x304c,0x82);
	ov3640_write_reg(client,0x3302,0xef);
	ov3640_write_reg(client,0x335f,0x34);
	ov3640_write_reg(client,0x3360,0x0c);
	ov3640_write_reg(client,0x3361,0x04);
	ov3640_write_reg(client,0x3362,0x12);
	ov3640_write_reg(client,0x3363,0x88);
	ov3640_write_reg(client,0x3364,0xe4);
	ov3640_write_reg(client,0x3403,0x42);
	ov3640_write_reg(client,0x3088,0x12);
	ov3640_write_reg(client,0x3089,0x80);
	ov3640_write_reg(client,0x308a,0x01);
	ov3640_write_reg(client,0x308b,0xe0);
	ov3640_write_reg(client,0x304c,0x85);
	ov3640_write_reg(client,0x300e,0x39);
	ov3640_write_reg(client,0x300f,0xa1);//12.5fps -> 25fps
#if defined(CONFIG_JZ4810_F4810) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	ov3640_write_reg(client,0x3011,0x5); /* default 0x00, change ov3640 PCLK DIV here --- by Lutts*/
#else
	ov3640_write_reg(client,0x3011,0x0); /* default 0x00, change ov3640 PCLK DIV here --- by Lutts*/
#endif
	ov3640_write_reg(client,0x3010,0x81);
	ov3640_write_reg(client,0x302e,0xA0);
	ov3640_write_reg(client,0x302d,0x00);
	ov3640_write_reg(client,0x3071,0x82);
	ov3640_write_reg(client,0x301C,0x05);
	return 0;
}


static void enable_test_pattern(struct i2c_client *client)
{
	unsigned char value;

	/* test pattern(color bar) */
	value = ov3640_read_reg(client, 0x307b);
	value &= ~0x3;
	ov3640_write_reg(client, 0x307b, value | 0x2);

	value = ov3640_read_reg(client, 0x306c);
	ov3640_write_reg(client, 0x306c, value & ~0x10);

	value = ov3640_read_reg(client, 0x307d);
	ov3640_write_reg(client,0x307d,value | 0x80);

//	value = ov3640_read_reg(client, 0x3080);
//	ov3640_write_reg(client,0x3080,value | 0x80);
}


int ov3640_preview_set(struct cim_sensor *sensor_info)
{

	struct ov3640_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
//	unsigned char value;

	printk("%s %s()\n", __FILE__, __func__);

	ov3640_write_reg(client,0x3012,0x10);
	ov3640_write_reg(client,0x3023,0x06);
	ov3640_write_reg(client,0x3026,0x03);
	ov3640_write_reg(client,0x3027,0x04);
	ov3640_write_reg(client,0x302a,0x03);
	ov3640_write_reg(client,0x302b,0x10);
	ov3640_write_reg(client,0x3075,0x24);
	ov3640_write_reg(client,0x300d,0x01);
	ov3640_write_reg(client,0x30d7,0x90);
	ov3640_write_reg(client,0x3069,0x04);
	ov3640_write_reg(client,0x303e,0x00);
	ov3640_write_reg(client,0x303f,0xc0);
	ov3640_write_reg(client,0x3302,0xef);
	ov3640_write_reg(client,0x335f,0x34);
	ov3640_write_reg(client,0x3360,0x0c);
	ov3640_write_reg(client,0x3361,0x04);
	ov3640_write_reg(client,0x3362,0x12);
	ov3640_write_reg(client,0x3363,0x88);
	ov3640_write_reg(client,0x3364,0xe4);
	ov3640_write_reg(client,0x3403,0x42);
	ov3640_write_reg(client,0x3088,0x12);
	ov3640_write_reg(client,0x3089,0x80);
	ov3640_write_reg(client,0x308a,0x01);
	ov3640_write_reg(client,0x308b,0xe0);

	ov3640_write_reg(client,0x304c,0x85);
	//ov3640_write_reg(client,0x3366,0x10);

	ov3640_write_reg(client,0x300e,0x39);
	ov3640_write_reg(client,0x300f,0xa1);//12.5fps -> 25fps
	ov3640_write_reg(client,0x3010,0x81);
#if defined(CONFIG_JZ4810_F4810) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	ov3640_write_reg(client,0x3011,0x5); /* default: 0x00, change ov3640 PCLK here --- by Lutts */
#else
	ov3640_write_reg(client,0x3011,0x0); /* default: 0x00, change ov3640 PCLK here --- by Lutts */
#endif
	ov3640_write_reg(client,0x302e,0xa0);
	ov3640_write_reg(client,0x302d,0x00);
	ov3640_write_reg(client,0x3071,0x82);
	ov3640_write_reg(client,0x301C,0x05);

	ov3640_write_reg(client,0x3100,0x02);
	ov3640_write_reg(client,0x3301,0xde);
	ov3640_write_reg(client,0x3304,0xfc);

	/* reg 0x3404 */
#define YUV422_YUYV 0x00
#define YUV422_YVYU 0X01
#define YUV422_UYVY 0x02
#define YUV422_VYUY 0x03

#define Y8	   0x0D

#define YUV444_YUV 0x0E
#define YUV444_YVU 0x0F
#define YUV444_UYV 0x1C
#define YUV444_VYU 0x1D
#define YUV444_UVY 0x1E
#define YUV444_VUY 0x1F

	ov3640_write_reg(client,0x3400,0x00); /* source */
	ov3640_write_reg(client,0x3404,YUV422_UYVY); /* fmt control */

#ifdef CONFIG_VIDEO_CIM_IN_FMT_YUV444
//	ov3640_write_reg(client,0x3404, YUV444_YUV);
#endif
	//ov3640_write_reg(client,0x3404, YUV444_YVU);
	//ov3640_write_reg(client,0x3404, YUV444_UVY);
	//ov3640_write_reg(client,0x3404, YUV444_VUY);

	//ov3640_write_reg(client,0x3404, Y8);

	ov3640_write_reg(client,0x3600,0xc0);

	ov3640_write_reg(client,0x3013,0xf7);


//	enable_test_pattern(client);	//ylyuan

	/***************** preview reg set **************************/
	return 0;
}


int ov3640_size_switch(struct cim_sensor *sensor_info,int width,int height)  //false
{
	struct ov3640_sensor *s;
	struct i2c_client * client ;
	char value;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"ov3640 size switch %d * %d\n",width,height);
#endif

	if(width == 2048 && height == 1536)
	{
		ov3640_write_reg(client,0x304c, 0x81);      //false
	}
	else if(width == 1600 && height == 1200)
	{
		ov3640_write_reg(client,0x304c, 0x81);
	}
	else if(width == 1280 && height == 1024)
	{
		ov3640_write_reg(client,0x304c, 0x82);
	}
	else if(width == 1024 && height == 768)
	{
		ov3640_write_reg(client,0x304c, 0x82);
	}
	else if(width == 800 && height == 600)
	{
		ov3640_write_reg(client,0x304c, 0x82);
	}
	else if(width == 800 && height == 480)
	{
		ov3640_write_reg(client,0x304c,0x82);
	}
	else if(width == 640 && height == 480)
	{
		ov3640_write_reg(client,0x304c, 0x82);
	}
	else if(width == 480 && height == 320)
	{
		ov3640_write_reg(client,0x304c,0x84);
	}
	else if(width == 352 && height == 288)
	{
		ov3640_write_reg(client,0x304c, 0x84);
	}
	else if(width == 320 && height == 240)
	{
		ov3640_write_reg(client,0x304c, 0x84);
	}
	else if(width == 176 && height == 144)
	{
		ov3640_write_reg(client,0x304c, 0x84);
	}
	else
		return 0;

	value = ov3640_read_reg(client,0x3012);

	if ( (value & 0x70) == 0 ) //FULL MODE     //false
	{
		ov3640_write_reg(client,0x3302, 0xef);
		ov3640_write_reg(client,0x335f, 0x68);
		ov3640_write_reg(client,0x3360, 0x18);
		ov3640_write_reg(client,0x3361, 0x0c);
	}
	else
	{
		ov3640_write_reg(client,0x3302, 0xef);
		ov3640_write_reg(client,0x335f, 0x34);
		ov3640_write_reg(client,0x3360, 0x0c);
		ov3640_write_reg(client,0x3361, 0x04);
	}

	ov3640_write_reg(client,0x3362, (unsigned char)((((width+8)>>8) & 0x0F) + (((height+4)>>4)&0x70)) );
	ov3640_write_reg(client,0x3363, (unsigned char)((width+8) & 0xFF) );
	ov3640_write_reg(client,0x3364, (unsigned char)((height+4) & 0xFF) );

	ov3640_write_reg(client,0x3403, 0x42);

	ov3640_write_reg(client,0x3088, (unsigned char)((width>>8)&0xFF) );
	ov3640_write_reg(client,0x3089, (unsigned char)(width & 0xFF) );
	ov3640_write_reg(client,0x308a, (unsigned char)((height>>8)&0xFF) );
	ov3640_write_reg(client,0x308b, (unsigned char)(height & 0xFF) );

	ov3640_write_reg(client,0x3f00,0x12);//update zone

	return 0;
}



int ov3640_capture_set(struct cim_sensor *sensor_info)
{

	struct ov3640_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/
	ov3640_write_reg(client,0x3012,0x00);
	ov3640_write_reg(client,0x3020,0x01);
	ov3640_write_reg(client,0x3021,0x1d);
	ov3640_write_reg(client,0x3022,0x00);
	ov3640_write_reg(client,0x3023,0x0a);
	ov3640_write_reg(client,0x3024,0x08);
	ov3640_write_reg(client,0x3025,0x18);
	ov3640_write_reg(client,0x3026,0x06);
	ov3640_write_reg(client,0x3027,0x0c);
	ov3640_write_reg(client,0x302a,0x06);
	ov3640_write_reg(client,0x302b,0x20);
	ov3640_write_reg(client,0x3075,0x44);
	ov3640_write_reg(client,0x300d,0x00);
	ov3640_write_reg(client,0x30d7,0x10);
	ov3640_write_reg(client,0x3069,0x40);
	ov3640_write_reg(client,0x303e,0x01);
	ov3640_write_reg(client,0x303f,0x80);
	ov3640_write_reg(client,0x3302,0xef);
	ov3640_write_reg(client,0x335f,0x68);
	ov3640_write_reg(client,0x3360,0x18);
	ov3640_write_reg(client,0x3361,0x0c);
	ov3640_write_reg(client,0x3362,0x68);
	ov3640_write_reg(client,0x3363,0x08);
	ov3640_write_reg(client,0x3364,0x04);
	ov3640_write_reg(client,0x3403,0x42);
	ov3640_write_reg(client,0x3088,0x08);
	ov3640_write_reg(client,0x3089,0x00);
	ov3640_write_reg(client,0x308a,0x06);
	ov3640_write_reg(client,0x308b,0x00);
	ov3640_write_reg(client,0x300e,0x39);
	ov3640_write_reg(client,0x300f,0x21);
	ov3640_write_reg(client,0x3010,0x20);
	ov3640_write_reg(client,0x304c,0x81);
	ov3640_write_reg(client,0x3366,0x10);
#if defined(CONFIG_JZ4810_F4770) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	ov3640_write_reg(client,0x3011,0x5); /* default 0x00, change ov3640 PCLK DIV here --- by lutts */
#else
	ov3640_write_reg(client,0x3011,0x0); /* default 0x00, change ov3640 PCLK DIV here --- by lutts */
#endif
	ov3640_write_reg(client,0x3f00,0x02);//disable overlay

#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"capture\n");
#endif


//	enable_test_pattern(client);	//ylyuan

	return 0;
}

void ov3640_set_ab_50hz(struct i2c_client *client)
{
	unsigned char reg3013,reg3014;
	reg3013 = ov3640_read_reg(client,0x3013);
	reg3014 = ov3640_read_reg(client,0x3014);

	reg3013 = (reg3013 & ~(0x20)) | 0x20;
	ov3640_write_reg(client,0x3013, reg3013);
	reg3014 = (reg3014 & ~(0xc0)) | 0x80;
	ov3640_write_reg(client,0x3014, reg3014);
}

void ov3640_set_ab_60hz(struct i2c_client *client)
{
 	unsigned char reg3013,reg3014;
	reg3013 = ov3640_read_reg(client,0x3013);
	reg3014 = ov3640_read_reg(client,0x3014);

	reg3013 = (reg3013 & ~(0x20)) | 0x20;
	ov3640_write_reg(client,0x3013, reg3013);
	reg3014 = (reg3014 & ~(0xc0)) | 0x00;
	ov3640_write_reg(client,0x3014, reg3014);
}

int ov3640_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov3640_sensor *s;
	struct i2c_client * client;
	unsigned char *str_antibanding;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"ov3640_set_antibanding");
#endif
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			ov3640_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
		case ANTIBANDING_50HZ :
			ov3640_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_50HZ";
			break;
		case ANTIBANDING_60HZ :
			ov3640_set_ab_60hz(client);
			str_antibanding = "ANTIBANDING_60HZ";
			break;
		case ANTIBANDING_OFF :
			str_antibanding = "ANTIBANDING_OFF";
			break;
		default:
			ov3640_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
	}
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev, "set antibanding to %s\n", str_antibanding);
#endif
	return 0;
}
void ov3640_set_effect_normal(struct i2c_client *client)
{
	/*
//	ov3640_write_reg(client,0x62,0x00);
	ov3640_write_reg(client,0x3302, 0xef);
	ov3640_write_reg(client,0x3355, 0x00);
	*/
}

void ov3640_set_effect_grayscale(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x3302, 0xef);
	ov3640_write_reg(client,0x3355, 0x40);//bit[6] negative
	*/
}

void ov3640_set_effect_sepia(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x3302, 0xef);
	ov3640_write_reg(client,0x3355, 0x18);
	ov3640_write_reg(client,0x335a, 0x40);
	ov3640_write_reg(client,0x335b, 0xa6);
	*/
}

void ov3640_set_effect_colorinv(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x3302, 0xef);
	ov3640_write_reg(client,0x3355, 0x00);
	*/
}

void ov3640_set_effect_sepiagreen(struct i2c_client *client)
{
	/*
 	ov3640_write_reg(client,0x3302, 0xef);
	ov3640_write_reg(client,0x3355, 0x18);
	ov3640_write_reg(client,0x335a, 0x60);
	ov3640_write_reg(client,0x335b, 0x60);
	*/
}

void ov3640_set_effect_sepiablue(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x3302, 0xef);
	ov3640_write_reg(client,0x3355, 0x18);
	ov3640_write_reg(client,0x335a, 0xa0);
	ov3640_write_reg(client,0x335b, 0x40);
	*/
}


int ov3640_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov3640_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;

	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"ov3640_set_effect");
#endif
	switch(arg)
	{
		case EFFECT_NONE:
			ov3640_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			ov3640_set_effect_grayscale(client);
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			ov3640_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			ov3640_set_effect_sepia(client);
			str_effect = "EFFECT_SEPIA";
			break;
		case EFFECT_POSTERIZE ://se diao fen li
			str_effect = "EFFECT_POSTERIZE";
			break;
		case EFFECT_WHITEBOARD :
			str_effect = "EFFECT_WHITEBOARD";
			break;
		case EFFECT_BLACKBOARD :
			str_effect = "EFFECT_BLACKBOARD";
			break;
		case EFFECT_AQUA  ://qian lv se
			ov3640_set_effect_sepiagreen(client);
			str_effect = "EFFECT_AQUA";
			break;
		case EFFECT_PASTEL:
			str_effect = "EFFECT_PASTEL";
			break;
		case EFFECT_MOSAIC:
			str_effect = "EFFECT_MOSAIC";
			break;
		case EFFECT_RESIZE:
			str_effect = "EFFECT_RESIZE";
			break;
		default:
			ov3640_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"set effect to %s\n", str_effect);
#endif
	return 0;
}

void ov3640_set_wb_auto(struct i2c_client *client)
{
	ov3640_write_reg(client,0x332b, 0x00);//AWB auto, bit[3]:0,auto
}

void ov3640_set_wb_cloud(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x332b, 0x08);
	ov3640_write_reg(client,0x33a7, 0x68);
	ov3640_write_reg(client,0x33a8, 0x40);
	ov3640_write_reg(client,0x33a9, 0x4e);
	*/
}

void ov3640_set_wb_daylight(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x332b, 0x08); //AWB off
	ov3640_write_reg(client,0x33a7, 0x5e);
	ov3640_write_reg(client,0x33a8, 0x40);
	ov3640_write_reg(client,0x33a9, 0x46);
	*/
}


void ov3640_set_wb_incandescence(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x332b, 0x08); //AWB off
	ov3640_write_reg(client,0x33a7, 0x5e);
	ov3640_write_reg(client,0x33a8, 0x40);
	ov3640_write_reg(client,0x33a9, 0x46);
	*/
}

void ov3640_set_wb_fluorescent(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x332b, 0x08); //AWB off
	ov3640_write_reg(client,0x33a7, 0x5e);
	ov3640_write_reg(client,0x33a8, 0x40);
	ov3640_write_reg(client,0x33a9, 0x46);
	*/
}

void ov3640_set_wb_tungsten(struct i2c_client *client)
{
	/*
	ov3640_write_reg(client,0x332b, 0x08);
	ov3640_write_reg(client,0x33a7, 0x44);
	ov3640_write_reg(client,0x33a8, 0x40);
	ov3640_write_reg(client,0x33a9, 0x70);
	*/
}

int ov3640_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov3640_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct ov3640_sensor, cs);
	client = s->client;
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"ov3640_set_balance");
#endif
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			ov3640_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			ov3640_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			ov3640_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			ov3640_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			ov3640_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			ov3640_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
#ifdef OV3640_SET_KERNEL_PRINT
	dev_info(&client->dev,"set balance to %s\n", str_balance);
#endif
	return 0;
}
