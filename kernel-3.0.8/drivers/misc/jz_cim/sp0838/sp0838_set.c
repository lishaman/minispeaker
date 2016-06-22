#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "sp0838_camera.h"

//#define SP0838_SET_KERNEL_PRINT


int sp0838_init(struct cim_sensor *sensor_info)
{
	struct sp0838_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"sp0838 init\n");
#endif
	sp0838_write_reg(client,0xfd,0x00);
	
	sp0838_write_reg(client,0x1B,0x02);
	sp0838_write_reg(client,0x27,0xe8);
	sp0838_write_reg(client,0x28,0x0B);
	sp0838_write_reg(client,0x32,0x00);
	sp0838_write_reg(client,0x22,0xc0);
	sp0838_write_reg(client,0x26,0x10);	

#ifndef CONFIG_HORIZONTAL_MIRROR
	#ifndef CONFIG_VERTICAL_MIRROR
	sp0838_write_reg(client, 0x31, 0x10);	//set mirror bit
	#else
	sp0838_write_reg(client, 0x31, 0x10 | 0x40);
	#endif
#else
	#ifndef CONFIG_VERTICAL_MIRROR
	sp0838_write_reg(client, 0x31, 0x10 | 0x20);
	#else
	sp0838_write_reg(client, 0x31, 0x10 | 0x60);
	#endif
#endif

	sp0838_write_reg(client,0x5f,0x11);
	
	sp0838_write_reg(client,0xfd,0x01);
	
	sp0838_write_reg(client,0x25,0x1a);
	sp0838_write_reg(client,0x26,0xfb);
	sp0838_write_reg(client,0x28,0x75);
	sp0838_write_reg(client,0x29,0x4e);
	sp0838_write_reg(client,0x31,0x60); 
	sp0838_write_reg(client,0x32,0x18);
	sp0838_write_reg(client,0x4d,0xdc);
	sp0838_write_reg(client,0x4e,0x53);
	sp0838_write_reg(client,0x41,0x8c);
	sp0838_write_reg(client,0x42,0x57);
	sp0838_write_reg(client,0x55,0xff);
	sp0838_write_reg(client,0x56,0x00);
	sp0838_write_reg(client,0x59,0x82);
	sp0838_write_reg(client,0x5a,0x00);
	sp0838_write_reg(client,0x5d,0xff);
	sp0838_write_reg(client,0x5e,0x6f);
	sp0838_write_reg(client,0x57,0xff);
	sp0838_write_reg(client,0x58,0x00);
	sp0838_write_reg(client,0x5b,0xff);
	sp0838_write_reg(client,0x5c,0xa8);
	sp0838_write_reg(client,0x5f,0x75);
	sp0838_write_reg(client,0x60,0x00);
	sp0838_write_reg(client,0x2d,0x00);
	sp0838_write_reg(client,0x2e,0x00);
	sp0838_write_reg(client,0x2f,0x00);
	sp0838_write_reg(client,0x30,0x00);
	sp0838_write_reg(client,0x33,0x00);
	sp0838_write_reg(client,0x34,0x00);
	sp0838_write_reg(client,0x37,0x00);
	sp0838_write_reg(client,0x38,0x00);
	
	sp0838_write_reg(client,0xfd,0x00);
	
	sp0838_write_reg(client,0x33,0x6f); //0x6f
	sp0838_write_reg(client,0x51,0x3f);
	sp0838_write_reg(client,0x52,0x09);
	sp0838_write_reg(client,0x53,0x00);
	sp0838_write_reg(client,0x54,0x00);
	sp0838_write_reg(client,0x55,0x10);
	sp0838_write_reg(client,0x4f,0x08);
	sp0838_write_reg(client,0x50,0x08);
	sp0838_write_reg(client,0x57,0x40);
	sp0838_write_reg(client,0x58,0x40);
	sp0838_write_reg(client,0x59,0x10);
	sp0838_write_reg(client,0x56,0x71);  	//0x70
	sp0838_write_reg(client,0x5a,0x02);
	sp0838_write_reg(client,0x5b,0x10);	//0x02
	sp0838_write_reg(client,0x5c,0x28);	//0x20
	sp0838_write_reg(client,0x65,0x06);
	sp0838_write_reg(client,0x66,0x01);
	sp0838_write_reg(client,0x67,0x03);
	sp0838_write_reg(client,0x68,0xc5);	//0xc6
	sp0838_write_reg(client,0x69,0x7f);
	sp0838_write_reg(client,0x6a,0x01);
	sp0838_write_reg(client,0x6b,0x06);
	sp0838_write_reg(client,0x6c,0x01);
	sp0838_write_reg(client,0x6d,0x03);
	sp0838_write_reg(client,0x6e,0xc5);	//0xc6
	sp0838_write_reg(client,0x6f,0x7f);
	sp0838_write_reg(client,0x70,0x01);
	sp0838_write_reg(client,0x71,0x0a);
	sp0838_write_reg(client,0x72,0x10);
	sp0838_write_reg(client,0x73,0x03);
	sp0838_write_reg(client,0x74,0xc4);	//0xc4
	sp0838_write_reg(client,0x75,0x7f);
	sp0838_write_reg(client,0x76,0x01);
	sp0838_write_reg(client,0xcb,0x07);
	sp0838_write_reg(client,0xcc,0x04);
	sp0838_write_reg(client,0xce,0xff);
	sp0838_write_reg(client,0xcf,0x10);
	sp0838_write_reg(client,0xd0,0x20);
	sp0838_write_reg(client,0xd1,0x00);
	sp0838_write_reg(client,0xd2,0x1c);
	sp0838_write_reg(client,0xd3,0x16);
	sp0838_write_reg(client,0xd4,0x00);
	sp0838_write_reg(client,0xd6,0x1c);
	sp0838_write_reg(client,0xd7,0x16);
	sp0838_write_reg(client,0xdd,0x70); //0x70);
	sp0838_write_reg(client,0xde,0xa5); //0x90);
	sp0838_write_reg(client,0x7f,0xd7);
	sp0838_write_reg(client,0x80,0xbc);
	sp0838_write_reg(client,0x81,0xed);
	sp0838_write_reg(client,0x82,0xd7);
	sp0838_write_reg(client,0x83,0xd4);
	sp0838_write_reg(client,0x84,0xd6);
	sp0838_write_reg(client,0x85,0xff);
	sp0838_write_reg(client,0x86,0x89);
	sp0838_write_reg(client,0x87,0xf8);
	sp0838_write_reg(client,0x88,0x3c);
	sp0838_write_reg(client,0x89,0x33);
	sp0838_write_reg(client,0x8a,0x0f);
	sp0838_write_reg(client,0x8b,0x00);
	sp0838_write_reg(client,0x8c,0x1a);
	sp0838_write_reg(client,0x8d,0x29);
	sp0838_write_reg(client,0x8e,0x41);
	sp0838_write_reg(client,0x8f,0x62);
	sp0838_write_reg(client,0x90,0x7c);
	sp0838_write_reg(client,0x91,0x90);
	sp0838_write_reg(client,0x92,0xa2);
	sp0838_write_reg(client,0x93,0xaf);
	sp0838_write_reg(client,0x94,0xbc);
	sp0838_write_reg(client,0x95,0xc5);
	sp0838_write_reg(client,0x96,0xcd);
	sp0838_write_reg(client,0x97,0xd5);
	sp0838_write_reg(client,0x98,0xdd);
	sp0838_write_reg(client,0x99,0xe5);
	sp0838_write_reg(client,0x9a,0xed);
	sp0838_write_reg(client,0x9b,0xf5);
	
	sp0838_write_reg(client,0xfd,0x01);
	
	sp0838_write_reg(client,0x8d,0xfd);
	sp0838_write_reg(client,0x8e,0xff);
	
	sp0838_write_reg(client,0xfd,0x00);
	
	sp0838_write_reg(client,0xca,0xcf);
	sp0838_write_reg(client,0xd8,0x50);	//0x48);
	sp0838_write_reg(client,0xd9,0x50);	//0x48);
	sp0838_write_reg(client,0xda,0x50);	//0x48);
	sp0838_write_reg(client,0xdb,0x40);
	sp0838_write_reg(client,0xb9,0x00);
	sp0838_write_reg(client,0xba,0x04);
	sp0838_write_reg(client,0xbb,0x08);
	sp0838_write_reg(client,0xbc,0x10);
	sp0838_write_reg(client,0xbd,0x20);
	sp0838_write_reg(client,0xbe,0x30);
	sp0838_write_reg(client,0xbf,0x40);
	sp0838_write_reg(client,0xc0,0x50);
	sp0838_write_reg(client,0xc1,0x60);
	sp0838_write_reg(client,0xc2,0x70);
	sp0838_write_reg(client,0xc3,0x80);
	sp0838_write_reg(client,0xc4,0x90);
	sp0838_write_reg(client,0xc5,0xA0);
	sp0838_write_reg(client,0xc6,0xB0);
	sp0838_write_reg(client,0xc7,0xC0);
	sp0838_write_reg(client,0xc8,0xD0);
	sp0838_write_reg(client,0xc9,0xE0);
	
	sp0838_write_reg(client,0xfd,0x01);
	
	sp0838_write_reg(client,0x89,0xf0);
	sp0838_write_reg(client,0x8a,0xff);
	
	sp0838_write_reg(client,0xfd,0x00);
	
	sp0838_write_reg(client,0xe8,0x30);
	sp0838_write_reg(client,0xe9,0x30);
	sp0838_write_reg(client,0xea,0x40);
	sp0838_write_reg(client,0xf4,0x1b);
	sp0838_write_reg(client,0xf5,0x80);
	sp0838_write_reg(client,0xf7,0x73); //0x78);
	sp0838_write_reg(client,0xf8,0x5d); //0x63);
	sp0838_write_reg(client,0xf9,0x68);
	sp0838_write_reg(client,0xfa,0x53);
	
	sp0838_write_reg(client,0xfd,0x01);
	
	sp0838_write_reg(client,0x09,0x31);
	sp0838_write_reg(client,0x0a,0x85);
	sp0838_write_reg(client,0x0b,0x0b);
	sp0838_write_reg(client,0x14,0x00); //0x20);
	sp0838_write_reg(client,0x15,0x0f);
	
	sp0838_write_reg(client,0xfd,0x00);
//fix 22fps	
	sp0838_write_reg(client,0x05,0x00);
	sp0838_write_reg(client,0x06,0x00);
	sp0838_write_reg(client,0x09,0x01);
	sp0838_write_reg(client,0x0a,0x05);
	sp0838_write_reg(client,0xf0,0x6c);
	sp0838_write_reg(client,0xf1,0x00);
	sp0838_write_reg(client,0xf2,0x62);
	sp0838_write_reg(client,0xf5,0x7b);
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x00,0xa0);
	sp0838_write_reg(client,0x0f,0x63);
	sp0838_write_reg(client,0x16,0x63);
	sp0838_write_reg(client,0x17,0x90);
	sp0838_write_reg(client,0x18,0x98);
	sp0838_write_reg(client,0x1b,0x63);
	sp0838_write_reg(client,0x1c,0x98);
	sp0838_write_reg(client,0xb4,0x20);
	sp0838_write_reg(client,0xb5,0x3c);
	sp0838_write_reg(client,0xb6,0x68);
	sp0838_write_reg(client,0xb9,0x40);
	sp0838_write_reg(client,0xba,0x4f);
	sp0838_write_reg(client,0xbb,0x47);
	sp0838_write_reg(client,0xbc,0x45);
	sp0838_write_reg(client,0xbd,0x70);
	sp0838_write_reg(client,0xbe,0x42);
	sp0838_write_reg(client,0xbf,0x42);
	sp0838_write_reg(client,0xc0,0x42);
	sp0838_write_reg(client,0xc1,0x41);
	sp0838_write_reg(client,0xc2,0x41);
	sp0838_write_reg(client,0xc3,0x41);
	sp0838_write_reg(client,0xc4,0x41);
	sp0838_write_reg(client,0xc5,0x41);
	sp0838_write_reg(client,0xc6,0x41);
	sp0838_write_reg(client,0xca,0x70);
	sp0838_write_reg(client,0xcb,0x04);
	                  
	
	sp0838_write_reg(client,0x14,0x20);
	sp0838_write_reg(client,0x15,0x0f);
	
	sp0838_write_reg(client,0xfd,0x00);
	
	sp0838_write_reg(client,0x32,0x0); //0x15
	sp0838_write_reg(client,0x34,0x66);
	sp0838_write_reg(client,0x35,0x00);
	sp0838_write_reg(client,0x36,0x00);	//00 40 80 c0



	return 0;
}

int sp0838_preview_set(struct cim_sensor *sensor_info)                   
{                               

	struct sp0838_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
	return 0;
} 


int sp0838_size_switch(struct cim_sensor *sensor_info,int width,int height)
{	
	struct sp0838_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"sp0838 size switch %d * %d\n",width,height);
#endif

	if(width == 640 && height == 480)
	{
		sp0838_write_reg(client,0xfd,0x00);
	    sp0838_write_reg(client,0x37,0x00);
	  	sp0838_write_reg(client,0x38,0x00);
	 	sp0838_write_reg(client,0x39,0x01);
	  	sp0838_write_reg(client,0x3a,0xe4);
	  	sp0838_write_reg(client,0x3b,0x00);
	  	sp0838_write_reg(client,0x3c,0x00);
	  	sp0838_write_reg(client,0x3d,0x02);
	  	sp0838_write_reg(client,0x3e,0x84);
	  	sp0838_write_reg(client,0x47,0x00);
	  	sp0838_write_reg(client,0x48,0x00);
	  	sp0838_write_reg(client,0x49,0x01);
	  	sp0838_write_reg(client,0x4a,0xe0);
	  	sp0838_write_reg(client,0x4b,0x00);
	  	sp0838_write_reg(client,0x4c,0x00);
	  	sp0838_write_reg(client,0x4d,0x02);
	  	sp0838_write_reg(client,0x4e,0x80);

	}
	else if(width == 352 && height == 288)
	{
		sp0838_write_reg(client,0xfd,0x00);
		sp0838_write_reg(client,0x37,0x00);
	  	sp0838_write_reg(client,0x38,0x5e);
	  	sp0838_write_reg(client,0x39,0x01);
	  	sp0838_write_reg(client,0x3a,0x24);
	  	sp0838_write_reg(client,0x3b,0x00);
	  	sp0838_write_reg(client,0x3c,0x8e);
	  	sp0838_write_reg(client,0x3d,0x01);
	  	sp0838_write_reg(client,0x3e,0x64);
	  	sp0838_write_reg(client,0x47,0x00);
	  	sp0838_write_reg(client,0x48,0x00);
	  	sp0838_write_reg(client,0x49,0x01);
	  	sp0838_write_reg(client,0x4a,0x20);
	  	sp0838_write_reg(client,0x4b,0x00);
	  	sp0838_write_reg(client,0x4c,0x00);
	  	sp0838_write_reg(client,0x4d,0x01);
	  	sp0838_write_reg(client,0x4e,0x60);
	}
	else if(width == 176 && height == 144)
	{
		sp0838_write_reg(client,0xfd,0x00);
		sp0838_write_reg(client,0x37,0x00);
		sp0838_write_reg(client,0x38,0xa6);
		sp0838_write_reg(client,0x39,0x00);
		sp0838_write_reg(client,0x3a,0x94);
		sp0838_write_reg(client,0x3b,0x00);
		sp0838_write_reg(client,0x3c,0xe6);
		sp0838_write_reg(client,0x3d,0x00);
		sp0838_write_reg(client,0x3e,0xb4);
	  	sp0838_write_reg(client,0x47,0x00);
	  	sp0838_write_reg(client,0x48,0x00);
	  	sp0838_write_reg(client,0x49,0x00);
	  	sp0838_write_reg(client,0x4a,0x90);
	  	sp0838_write_reg(client,0x4b,0x00);
	  	sp0838_write_reg(client,0x4c,0x00);
	  	sp0838_write_reg(client,0x4d,0x00);
	 	sp0838_write_reg(client,0x4e,0xb0);
	}
	else if(width == 320 && height == 240)
	{
		sp0838_write_reg(client,0xfd,0x00);
		sp0838_write_reg(client,0x37,0x00);
		sp0838_write_reg(client,0x38,0x00);
		sp0838_write_reg(client,0x39,0x00);
		sp0838_write_reg(client,0x3a,0xf4);
		sp0838_write_reg(client,0x3b,0x00);
		sp0838_write_reg(client,0x3c,0x00);
		sp0838_write_reg(client,0x3d,0x01);
		sp0838_write_reg(client,0x3e,0x44);
		sp0838_write_reg(client,0x47,0x00);
		sp0838_write_reg(client,0x48,0x00);
		sp0838_write_reg(client,0x49,0x00);
		sp0838_write_reg(client,0x4a,0xf0);
		sp0838_write_reg(client,0x4b,0x00);
		sp0838_write_reg(client,0x4c,0x00);
		sp0838_write_reg(client,0x4d,0x01);
		sp0838_write_reg(client,0x4e,0x40);
	}
	else
		return 0;

	return 0;
}



int sp0838_capture_set(struct cim_sensor *sensor_info)
{

	struct sp0838_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"capture\n");
#endif
	return 0;
}

void sp0838_set_ab_50hz(struct i2c_client *client)
{

}

void sp0838_set_ab_60hz(struct i2c_client *client)
{
 
}

int sp0838_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct sp0838_sensor *s;
	struct i2c_client * client;
	unsigned char *str_antibanding;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"sp0838_set_antibanding");
#endif
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			sp0838_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
		case ANTIBANDING_50HZ :
			sp0838_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_50HZ";
			break;
		case ANTIBANDING_60HZ :
			sp0838_set_ab_60hz(client);
			str_antibanding = "ANTIBANDING_60HZ";
			break;
		case ANTIBANDING_OFF :
			str_antibanding = "ANTIBANDING_OFF";
			break;
		default:
			sp0838_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
	}
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev, "set antibanding to %s\n", str_antibanding);
#endif
	return 0;
}
void sp0838_set_effect_normal(struct i2c_client *client)
{
	sp0838_write_reg(client,0x62,0x00);
}

void sp0838_set_effect_grayscale(struct i2c_client *client)
{
	sp0838_write_reg(client,0x62,0x10);
}

void sp0838_set_effect_sepia(struct i2c_client *client)
{
	 sp0838_write_reg(client,0x62, 0x20); 
}

void sp0838_set_effect_colorinv(struct i2c_client *client)
{
	sp0838_write_reg(client,0x62,0x04);
}

void sp0838_set_effect_sepiagreen(struct i2c_client *client)
{
 
}

void sp0838_set_effect_sepiablue(struct i2c_client *client)
{

}


int sp0838_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct sp0838_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;
	
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"sp0838_set_effect");
#endif
	switch(arg)
	{
		case EFFECT_NONE:
			sp0838_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			sp0838_set_effect_grayscale(client);  
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			sp0838_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			sp0838_set_effect_sepia(client);
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
			sp0838_set_effect_sepiagreen(client);
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
			sp0838_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"set effect to %s\n", str_effect);
#endif
	return 0;
}

void sp0838_set_wb_auto(struct i2c_client *client)
{       
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x28,0x75);
	sp0838_write_reg(client,0x29,0x4e);
	     
	sp0838_write_reg(client,0xfd,0x00);
	sp0838_write_reg(client,0xe7,0x03);
	sp0838_write_reg(client,0xe7,0x00);
	sp0838_write_reg(client,0x32,0x15);	
}

void sp0838_set_wb_cloud(struct i2c_client *client)
{       
	sp0838_write_reg(client,0xfd,0x00);
	sp0838_write_reg(client,0x32,0x05);
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x28,0x71);
	sp0838_write_reg(client,0x29,0x41);
	sp0838_write_reg(client,0xfd,0x00);

}

void sp0838_set_wb_daylight(struct i2c_client *client)
{              
	sp0838_write_reg(client,0xfd,0x00);
	sp0838_write_reg(client,0x32,0x05);
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x28,0x6b);
	sp0838_write_reg(client,0x29,0x48);
	sp0838_write_reg(client,0xfd,0x00);

}


void sp0838_set_wb_incandescence(struct i2c_client *client)
{    
	sp0838_write_reg(client,0xfd,0x00);
	sp0838_write_reg(client,0x32,0x05);
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x28,0x41);
	sp0838_write_reg(client,0x29,0x71);
	sp0838_write_reg(client,0xfd,0x00);

}

void sp0838_set_wb_fluorescent(struct i2c_client *client)
{
	sp0838_write_reg(client,0xfd,0x00);
	sp0838_write_reg(client,0x32,0x05);
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x28,0x5a);
	sp0838_write_reg(client,0x29,0x62);
	sp0838_write_reg(client,0xfd,0x00);
}

void sp0838_set_wb_tungsten(struct i2c_client *client)
{   
	sp0838_write_reg(client,0xfd,0x00);
	sp0838_write_reg(client,0x32,0x05);
	sp0838_write_reg(client,0xfd,0x01);
	sp0838_write_reg(client,0x28,0x57);
	sp0838_write_reg(client,0x29,0x66);
	sp0838_write_reg(client,0xfd,0x00);

}

int sp0838_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct sp0838_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct sp0838_sensor, cs);
	client = s->client;
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"sp0838_set_balance");
#endif
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			sp0838_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			sp0838_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			sp0838_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			sp0838_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			sp0838_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			sp0838_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
#ifdef SP0838_SET_KERNEL_PRINT
	dev_info(&client->dev,"set balance to %s\n", str_balance);
#endif
	return 0;
}

