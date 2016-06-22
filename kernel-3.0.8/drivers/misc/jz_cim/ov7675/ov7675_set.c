#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>
#include "ov7675_camera.h"



int ov7675_init(struct cim_sensor *sensor_info)
{
	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

	dev_info(&client->dev,"ov7675 -----------------------------------init\n");
		ov7675_write_reg(client,0x12,0x80);
		mdelay(10);
	//	ov7675_write_reg(client,0x09,0x10);
		ov7675_write_reg(client,0xc1,0x7f);
		ov7675_write_reg(client,0x11,0x00);
		ov7675_write_reg(client,0x3a,0x0c);
		ov7675_write_reg(client,0x3d,0xc0);
		ov7675_write_reg(client,0x12,0x00);
		ov7675_write_reg(client,0x15,0x02);//42
		ov7675_write_reg(client,0x17,0x13);//13
		ov7675_write_reg(client,0x18,0x01);//01
		ov7675_write_reg(client,0x32,0xbf);
		ov7675_write_reg(client,0x19,0x02);
		ov7675_write_reg(client,0x1a,0x7a);
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0x0c,0x00);
		ov7675_write_reg(client,0x3e,0x00);
		ov7675_write_reg(client,0x70,0x3a);
		ov7675_write_reg(client,0x71,0x35);
		ov7675_write_reg(client,0x72,0x11);
		ov7675_write_reg(client,0x73,0xf0);
		ov7675_write_reg(client,0xa2,0x02);

	//	ov7675_write_reg(client,0x1b,0x40);
		//gamma
		ov7675_write_reg(client,0x7a,0x20);
		ov7675_write_reg(client,0x7b,0x03);
		ov7675_write_reg(client,0x7c,0x0a);
		ov7675_write_reg(client,0x7d,0x1a);
		ov7675_write_reg(client,0x7e,0x3f);
		ov7675_write_reg(client,0x7f,0x4e);
		ov7675_write_reg(client,0x80,0x5b);
		ov7675_write_reg(client,0x81,0x68);
		ov7675_write_reg(client,0x82,0x75);
		ov7675_write_reg(client,0x83,0x7f);
		ov7675_write_reg(client,0x84,0x89);
		ov7675_write_reg(client,0x85,0x9a);
		ov7675_write_reg(client,0x86,0xa6);
		ov7675_write_reg(client,0x87,0xbd);
		ov7675_write_reg(client,0x88,0xd3);
		ov7675_write_reg(client,0x89,0xe8);
		ov7675_write_reg(client,0x13,0xe0);
		ov7675_write_reg(client,0x00,0x00);
		ov7675_write_reg(client,0x10,0x00);
		ov7675_write_reg(client,0x0d,0x40);
		ov7675_write_reg(client,0x14,0x28);
		ov7675_write_reg(client,0xa5,0x02);
		ov7675_write_reg(client,0xab,0x02);
		//bright
		ov7675_write_reg(client,0x24,0x68);
		ov7675_write_reg(client,0x25,0x58);
		ov7675_write_reg(client,0x26,0xc2);
		ov7675_write_reg(client,0x9f,0x78);
		ov7675_write_reg(client,0xa0,0x68);
		ov7675_write_reg(client,0xa1,0x03);
		ov7675_write_reg(client,0xa6,0xd8);
		ov7675_write_reg(client,0xa7,0xd8);
		ov7675_write_reg(client,0xa8,0xf0);
		ov7675_write_reg(client,0xa9,0x90);
		ov7675_write_reg(client,0xaa,0x14);
		ov7675_write_reg(client,0x13,0xe5);
		ov7675_write_reg(client,0x0e,0x61);
		ov7675_write_reg(client,0x0f,0x4b);
		ov7675_write_reg(client,0x16,0x02);
		ov7675_write_reg(client,0x1e,0x07);
		ov7675_write_reg(client,0x21,0x02);
		ov7675_write_reg(client,0x22,0x91);
		ov7675_write_reg(client,0x29,0x07);
		ov7675_write_reg(client,0x33,0x0b);
		ov7675_write_reg(client,0x35,0x0b);
		ov7675_write_reg(client,0x37,0x1d);
		ov7675_write_reg(client,0x38,0x71);
		ov7675_write_reg(client,0x39,0x2a);
		ov7675_write_reg(client,0x3c,0x78);
		ov7675_write_reg(client,0x4d,0x40);
		ov7675_write_reg(client,0x4e,0x20);
		ov7675_write_reg(client,0x69,0x00);
		ov7675_write_reg(client,0x6b,0x0a);
		ov7675_write_reg(client,0x74,0x10);
		ov7675_write_reg(client,0x8d,0x4f);
		ov7675_write_reg(client,0x8e,0x00);
		ov7675_write_reg(client,0x8f,0x00);
		ov7675_write_reg(client,0x90,0x00);
		ov7675_write_reg(client,0x91,0x00);
		ov7675_write_reg(client,0x96,0x00);
		ov7675_write_reg(client,0x9a,0x80);
		ov7675_write_reg(client,0xb0,0x84);
		ov7675_write_reg(client,0xb1,0x0c);
		ov7675_write_reg(client,0xb2,0x0e);
		ov7675_write_reg(client,0xb3,0x82);
		ov7675_write_reg(client,0xb8,0x0a);
		ov7675_write_reg(client,0x43,0x0a);
		ov7675_write_reg(client,0x44,0xf2);
		ov7675_write_reg(client,0x45,0x39);
		ov7675_write_reg(client,0x46,0x62);
		ov7675_write_reg(client,0x47,0x3d);
		ov7675_write_reg(client,0x48,0x55);
		ov7675_write_reg(client,0x59,0x83);
		ov7675_write_reg(client,0x5a,0x0d);
		ov7675_write_reg(client,0x5b,0xcd);
		ov7675_write_reg(client,0x5c,0x8c);
		ov7675_write_reg(client,0x5d,0x77);
		ov7675_write_reg(client,0x5e,0x16);
		ov7675_write_reg(client,0x6c,0x0a);
		ov7675_write_reg(client,0x6d,0x65);
		ov7675_write_reg(client,0x6e,0x11);
		ov7675_write_reg(client,0x6a,0x40);
		ov7675_write_reg(client,0x01,0x56);
		ov7675_write_reg(client,0x02,0x44);
		ov7675_write_reg(client,0x13,0xe7); //8f
		//	dprintk("-------------------------------ov7675 init 2");
		//cmx
		ov7675_write_reg(client,0x4f,0x88);
		ov7675_write_reg(client,0x50,0x8b);
		ov7675_write_reg(client,0x51,0x04);
		ov7675_write_reg(client,0x52,0x11);
		ov7675_write_reg(client,0x53,0x8c);
		ov7675_write_reg(client,0x54,0x9d);
		ov7675_write_reg(client,0x55,0x98);
		ov7675_write_reg(client,0x56,0x40);
		ov7675_write_reg(client,0x57,0x80);
		ov7675_write_reg(client,0x58,0x9a);
		//sharpness
		ov7675_write_reg(client,0x41,0x08);
		ov7675_write_reg(client,0x3f,0x00);
		ov7675_write_reg(client,0x75,0x23);
		ov7675_write_reg(client,0x76,0xe1);
		ov7675_write_reg(client,0x4c,0x00);
		ov7675_write_reg(client,0x77,0x01);
		ov7675_write_reg(client,0x3d,0xc2);
		ov7675_write_reg(client,0x4b,0x09);
		ov7675_write_reg(client,0xc9,0x30);
		ov7675_write_reg(client,0x41,0x38);

		ov7675_write_reg(client,0x56,0x40);
		ov7675_write_reg(client,0x34,0x11);
		ov7675_write_reg(client,0x3b,0x00);
		ov7675_write_reg(client,0xa4,0x88);
		ov7675_write_reg(client,0x96,0x00);
		ov7675_write_reg(client,0x97,0x30);
		ov7675_write_reg(client,0x98,0x20);
		ov7675_write_reg(client,0x99,0x30);
		ov7675_write_reg(client,0x9a,0x84);
		ov7675_write_reg(client,0x9b,0x29);
		ov7675_write_reg(client,0x9c,0x03);
		ov7675_write_reg(client,0x9d,0x99);
		ov7675_write_reg(client,0x9e,0x7f);
		ov7675_write_reg(client,0x78,0x04);
		ov7675_write_reg(client,0x79,0x01);
		ov7675_write_reg(client,0xc8,0xf0);
		ov7675_write_reg(client,0x79,0x0f);
		ov7675_write_reg(client,0xc8,0x00);
		ov7675_write_reg(client,0x79,0x10);
		ov7675_write_reg(client,0xc8,0x7e);
		ov7675_write_reg(client,0x79,0x0a);
		ov7675_write_reg(client,0xc8,0x80);
		ov7675_write_reg(client,0x79,0x0b);
		ov7675_write_reg(client,0xc8,0x01);
		ov7675_write_reg(client,0x79,0x0c);
		ov7675_write_reg(client,0xc8,0x0f);
		ov7675_write_reg(client,0x79,0x0d);
		ov7675_write_reg(client,0xc8,0x20);
		ov7675_write_reg(client,0x79,0x09);
		ov7675_write_reg(client,0xc8,0x80);
		ov7675_write_reg(client,0x79,0x02);
		ov7675_write_reg(client,0xc8,0xc0);
		ov7675_write_reg(client,0x79,0x03);
		ov7675_write_reg(client,0xc8,0x40);
		ov7675_write_reg(client,0x79,0x05);
		ov7675_write_reg(client,0xc8,0x30);
		ov7675_write_reg(client,0x79,0x26);
		ov7675_write_reg(client,0x94,0x05);
		ov7675_write_reg(client,0x95,0x09);
	//	ov7675_write_reg(client,0x3a,0x0d);
	//	ov7675_write_reg(client,0x3d,0xc3);
	//	ov7675_write_reg(client,0x19,0x03);
	//	ov7675_write_reg(client,0x1a,0x7b);
		ov7675_write_reg(client,0x2a,0x00);
		ov7675_write_reg(client,0x2b,0x00);
	//	ov7675_write_reg(client,0x18,0x01);
		//lens
		ov7675_write_reg(client,0x66,0x05);
		ov7675_write_reg(client,0x62,0x10);
		ov7675_write_reg(client,0x63,0x0b);
		ov7675_write_reg(client,0x65,0x07);
		ov7675_write_reg(client,0x64,0x0f);
		ov7675_write_reg(client,0x94,0x0e);
		ov7675_write_reg(client,0x95,0x10);
		ov7675_write_reg(client,0x09,0x00);

		//		printk("-------------------------------ov7675 init 3");

	return 0;
}

int ov7675_preview_set(struct cim_sensor *sensor_info)                   
{                               

	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
	return 0;
} 


int ov7675_size_switch(struct cim_sensor *sensor_info,int width,int height)
{	
	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov7675-----------------size switch %d * %d\n",width,height);
	
	if(width == 640 && height == 480)
	{
		ov7675_write_reg(client,0x92,0x00);              
		ov7675_write_reg(client,0x93,0x00);
		ov7675_write_reg(client,0xb9,0x00);
		ov7675_write_reg(client,0x19,0x03);
		ov7675_write_reg(client,0x1a,0x7b);
		ov7675_write_reg(client,0x17,0x13);
		ov7675_write_reg(client,0x18,0x01);              
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0xe6,0x00);
		ov7675_write_reg(client,0xd2,0x1c);

	}
	else if(width == 352 && height == 288)
	{
		ov7675_write_reg(client,0x92,0x00);              
		ov7675_write_reg(client,0x93,0x00);
		ov7675_write_reg(client,0xb9,0x00);
		ov7675_write_reg(client,0x19,0x02);
		ov7675_write_reg(client,0x1a,0x7a);
		ov7675_write_reg(client,0x17,0x13);
		ov7675_write_reg(client,0x18,0x01);              
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0xe6,0x00);
		ov7675_write_reg(client,0xd2,0x1c);

		ov7675_write_reg(client,0x19,0x17);         
		ov7675_write_reg(client,0x1a,0x5f);
		ov7675_write_reg(client,0x17,0x1d);
		ov7675_write_reg(client,0x18,0x49);
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0x32,0xbf);
	}
	else if(width == 176 && height == 144)
	{
		ov7675_write_reg(client,0x92,0x88);              
		ov7675_write_reg(client,0x93,0x00);
		ov7675_write_reg(client,0xb9,0x30);
		ov7675_write_reg(client,0x19,0x02);
		ov7675_write_reg(client,0x1a,0x3e);
		ov7675_write_reg(client,0x17,0x13);
		ov7675_write_reg(client,0x18,0x3b);              
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0xe6,0x05);
		ov7675_write_reg(client,0xd2,0x1c);

		ov7675_write_reg(client,0x19,0x17);              
		ov7675_write_reg(client,0x1a,0x3b);
		ov7675_write_reg(client,0x17,0x1d);
		ov7675_write_reg(client,0x18,0x33);
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0x32,0xbf);
	}
	else if(width == 320 && height == 240)
	{
		ov7675_write_reg(client,0x92,0x88);              
		ov7675_write_reg(client,0x93,0x00);
		ov7675_write_reg(client,0xb9,0x30);
		ov7675_write_reg(client,0x19,0x02);
		ov7675_write_reg(client,0x1a,0x3e);
		ov7675_write_reg(client,0x17,0x13);
		ov7675_write_reg(client,0x18,0x3b);              
		ov7675_write_reg(client,0x03,0x0a);
		ov7675_write_reg(client,0xe6,0x05);
		ov7675_write_reg(client,0xd2,0x1c);
	}
	else
		return 0;

	
	return 0;
}



int ov7675_capture_set(struct cim_sensor *sensor_info)
{
	
	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/

	dev_info(&client->dev,"------------------------------------capture\n");
	return 0;
}

void ov7675_set_ab_50hz(struct i2c_client *client)
{
	ov7675_write_reg(client,0x13, 0xe7);  
	ov7675_write_reg(client,0x9d, 0x4c);  
	ov7675_write_reg(client,0xa5, 0x05);  
	ov7675_write_reg(client,0x3b, 0x02);	    /* 50 Hz */	
}

void ov7675_set_ab_60hz(struct i2c_client *client)
{
	ov7675_write_reg(client,0x13, 0xe7);  
	ov7675_write_reg(client,0x9e, 0x3f);  
	ov7675_write_reg(client,0xab, 0x07);  
	ov7675_write_reg(client,0x3b, 0x02);	    /* 50 Hz */	
}

int ov7675_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov7675_set_antibanding");
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			ov7675_set_ab_50hz(client);
			dev_info(&client->dev,"ANTIBANDING_AUTO ");
			break;
		case ANTIBANDING_50HZ :
			ov7675_set_ab_50hz(client);
			dev_info(&client->dev,"ANTIBANDING_50HZ ");
			break;
		case ANTIBANDING_60HZ :
			ov7675_set_ab_60hz(client);
			dev_info(&client->dev,"ANTIBANDING_60HZ ");
			break;
		case ANTIBANDING_OFF :
			dev_info(&client->dev,"ANTIBANDING_OFF ");
			break;
	}
	return 0;
}
void ov7675_set_effect_normal(struct i2c_client *client)
{
   ov7675_write_reg(client,0x3A,0x0C);  
   ov7675_write_reg(client,0x67,0x80);  
   ov7675_write_reg(client,0x68,0x80);
   ov7675_write_reg(client,0x56,0x40);  

}

void ov7675_set_effect_grayscale(struct i2c_client *client)
{
   ov7675_write_reg(client,0x3A,0x1C);  
   ov7675_write_reg(client,0x67,0x80);  
   ov7675_write_reg(client,0x68,0x80);
   ov7675_write_reg(client,0x56,0x40); 
}

void ov7675_set_effect_sepia(struct i2c_client *client)
{
   ov7675_write_reg(client,0x3A,0x1C);  
   ov7675_write_reg(client,0x67,0x40);  
   ov7675_write_reg(client,0x68,0xa0);
   ov7675_write_reg(client,0x56,0x40);  
}

void ov7675_set_effect_colorinv(struct i2c_client *client)
{
   ov7675_write_reg(client,0x3A,0x2C);  
   ov7675_write_reg(client,0x67,0x80);  
   ov7675_write_reg(client,0x68,0x80);
   ov7675_write_reg(client,0x56,0x40); 
}

void ov7675_set_effect_sepiagreen(struct i2c_client *client)
{
   ov7675_write_reg(client,0x3A,0x1C);  
   ov7675_write_reg(client,0x67,0x40);  
   ov7675_write_reg(client,0x68,0x40);
   ov7675_write_reg(client,0x56,0x40);  
}

void ov7675_set_effect_sepiablue(struct i2c_client *client)
{
   ov7675_write_reg(client,0x3A,0x1C);  
   ov7675_write_reg(client,0x67,0xc0);  
   ov7675_write_reg(client,0x68,0x80);
   ov7675_write_reg(client,0x56,0x40);  
}


int ov7675_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov7675_set_effect");
		switch(arg)
		{
			case EFFECT_NONE:
				ov7675_set_effect_normal(client);
				dev_info(&client->dev,"EFFECT_NONE");
				break;
			case EFFECT_MONO :
				ov7675_set_effect_grayscale(client);  
				dev_info(&client->dev,"EFFECT_MONO ");
				break;
			case EFFECT_NEGATIVE :
				ov7675_set_effect_colorinv(client);
				dev_info(&client->dev,"EFFECT_NEGATIVE ");
				break;
			case EFFECT_SOLARIZE ://bao guang
				dev_info(&client->dev,"EFFECT_SOLARIZE ");
				break;
			case EFFECT_SEPIA :
				ov7675_set_effect_sepia(client);
				dev_info(&client->dev,"EFFECT_SEPIA ");
				break;
			case EFFECT_POSTERIZE ://se diao fen li
				dev_info(&client->dev,"EFFECT_POSTERIZE ");
				break;
			case EFFECT_WHITEBOARD :
				dev_info(&client->dev,"EFFECT_WHITEBOARD ");
				break;
			case EFFECT_BLACKBOARD :
				dev_info(&client->dev,"EFFECT_BLACKBOARD ");
				break;
			case EFFECT_AQUA  ://qian lv se
				ov7675_set_effect_sepiagreen(client);
				dev_info(&client->dev,"EFFECT_AQUA  ");
				break;
			case EFFECT_PASTEL:
				dev_info(&client->dev,"EFFECT_PASTEL");
				break;
			case EFFECT_MOSAIC:
				dev_info(&client->dev,"EFFECT_MOSAIC");
				break;
			case EFFECT_RESIZE:
				dev_info(&client->dev,"EFFECT_RESIZE");
				break;
		}
	
		return 0;
}

void ov7675_set_wb_auto(struct i2c_client *client)
{       
  int temp = ov7675_read_reg(client,0x13);
  ov7675_write_reg(client,0x13,temp|0x02);   // Enable AWB	
  ov7675_write_reg(client,0x01,0x56);
  ov7675_write_reg(client,0x02,0x44);		
}

void ov7675_set_wb_cloud(struct i2c_client *client)
{       
  int temp = ov7675_read_reg(client,0x13);
  ov7675_write_reg(client,0x13,temp&~0x02);   // Disable AWB	
  ov7675_write_reg(client,0x01,0x58);
  ov7675_write_reg(client,0x02,0x60);
  ov7675_write_reg(client,0x6a,0x40);
}

void ov7675_set_wb_daylight(struct i2c_client *client)
{              
  int temp = ov7675_read_reg(client,0x13);
  ov7675_write_reg(client,0x13,temp&~0x02);   // Disable AWB	
  ov7675_write_reg(client,0x01,0x56);
  ov7675_write_reg(client,0x02,0x5c);
  ov7675_write_reg(client,0x6a,0x42);

}


void ov7675_set_wb_incandescence(struct i2c_client *client)
{    
  int temp = ov7675_read_reg(client,0x13);
  ov7675_write_reg(client,0x13,temp&~0x02);   // Disable AWB	
  ov7675_write_reg(client,0x01,0x9a);
  ov7675_write_reg(client,0x02,0x40);
  ov7675_write_reg(client,0x6a,0x48);                             
}

void ov7675_set_wb_fluorescent(struct i2c_client *client)
{
  int temp = ov7675_read_reg(client,0x13);
  ov7675_write_reg(client,0x13,temp&~0x02);   // Disable AWB	
  ov7675_write_reg(client,0x01,0x8b);
  ov7675_write_reg(client,0x02,0x42);
  ov7675_write_reg(client,0x6a,0x40);
                       
}

void ov7675_set_wb_tungsten(struct i2c_client *client)
{   
  int temp = ov7675_read_reg(client,0x13);
  ov7675_write_reg(client,0x13,temp&~0x02);   // Disable AWB	
  ov7675_write_reg(client,0x01,0xb8);
  ov7675_write_reg(client,0x02,0x40);
  ov7675_write_reg(client,0x6a,0x4f);
}

int ov7675_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct ov7675_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov7675_sensor, cs);
	client = s->client;
	
	dev_info(&client->dev,"ov7675_set_balance");
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			ov7675_set_wb_auto(client);
			dev_info(&client->dev,"WHITE_BALANCE_AUTO");
			break;
		case WHITE_BALANCE_INCANDESCENT :
			ov7675_set_wb_incandescence(client);
			dev_info(&client->dev,"WHITE_BALANCE_INCANDESCENT ");
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			ov7675_set_wb_fluorescent(client);
			dev_info(&client->dev,"WHITE_BALANCE_FLUORESCENT ");
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			dev_info(&client->dev,"WHITE_BALANCE_WARM_FLUORESCENT ");
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			ov7675_set_wb_daylight(client);
			dev_info(&client->dev,"WHITE_BALANCE_DAYLIGHT ");
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			ov7675_set_wb_cloud(client);
			dev_info(&client->dev,"WHITE_BALANCE_CLOUDY_DAYLIGHT ");
			break;
		case WHITE_BALANCE_TWILIGHT :
			dev_info(&client->dev,"WHITE_BALANCE_TWILIGHT ");
			break;
		case WHITE_BALANCE_SHADE :
			dev_info(&client->dev,"WHITE_BALANCE_SHADE ");
			break;
	}

	return 0;
}

