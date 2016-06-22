#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "gc0308_camera.h"

//#define GC0308_SET_KERNEL_PRINT


int gc0308_init(struct cim_sensor *sensor_info)
{
	struct gc0308_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"gc0308 init\n");
#endif

	gc0308_write_reg(client,0xfe, 0x80);
	gc0308_write_reg(client,0xfe, 0x00);	// set page0
	gc0308_write_reg(client,0xd2, 0x10);	// close AEC
	gc0308_write_reg(client,0x22, 0x55);	// close AWB

	gc0308_write_reg(client,0x03, 0x01);
	gc0308_write_reg(client,0x04, 0x2c);
	gc0308_write_reg(client,0x5a, 0x56);
	gc0308_write_reg(client,0x5b, 0x40);
	gc0308_write_reg(client,0x5c, 0x4a);

	gc0308_write_reg(client,0x22, 0x57);	// Open AWB
	gc0308_write_reg(client,0x01, 0xfa);
	gc0308_write_reg(client,0x02, 0x70);
	gc0308_write_reg(client,0x0f, 0x01);


	gc0308_write_reg(client,0xe2, 0x00);	//anti-flicker step [11:8]
	gc0308_write_reg(client,0xe3, 0x64);	//anti-flicker step [7:0]

	gc0308_write_reg(client,0xe4, 0x02);	//exp level 1  16.67fps
	gc0308_write_reg(client,0xe5, 0x58);
	gc0308_write_reg(client,0xe6, 0x03);	//exp level 2  12.5fps
	gc0308_write_reg(client,0xe7, 0x20);
	gc0308_write_reg(client,0xe8, 0x04);	//exp level 3  8.33fps
	gc0308_write_reg(client,0xe9, 0xb0);
	gc0308_write_reg(client,0xea, 0x09);	//exp level 4  4.00fps
	gc0308_write_reg(client,0xeb, 0xc4);

	gc0308_write_reg(client,0x05, 0x00);
	gc0308_write_reg(client,0x06, 0x00);
	gc0308_write_reg(client,0x07, 0x00);
	gc0308_write_reg(client,0x08, 0x00);
	gc0308_write_reg(client,0x09, 0x01);
	gc0308_write_reg(client,0x0a, 0xe8);
	gc0308_write_reg(client,0x0b, 0x02);
	gc0308_write_reg(client,0x0c, 0x88);
	gc0308_write_reg(client,0x0d, 0x02);
	gc0308_write_reg(client,0x0e, 0x02);
	gc0308_write_reg(client,0x10, 0x22);
	gc0308_write_reg(client,0x11, 0xfd);
	gc0308_write_reg(client,0x12, 0x2a);
	gc0308_write_reg(client,0x13, 0x00);

	gc0308_write_reg(client,0x15, 0x0a);
	gc0308_write_reg(client,0x16, 0x05);
	gc0308_write_reg(client,0x17, 0x01);
	gc0308_write_reg(client,0x18, 0x44);
	gc0308_write_reg(client,0x19, 0x44);
	gc0308_write_reg(client,0x1a, 0x1e);
	gc0308_write_reg(client,0x1b, 0x00);
	gc0308_write_reg(client,0x1c, 0xc1);
	gc0308_write_reg(client,0x1d, 0x08);
	gc0308_write_reg(client,0x1e, 0x60);
	gc0308_write_reg(client,0x1f, 0x03);	//16


	gc0308_write_reg(client,0x20, 0xff);
	gc0308_write_reg(client,0x21, 0xf8);
	gc0308_write_reg(client,0x22, 0x57);
	gc0308_write_reg(client,0x24, 0xa2);
	gc0308_write_reg(client,0x25, 0x0f);
	gc0308_write_reg(client,0x28, 0x11);	//add

	gc0308_write_reg(client,0x26, 0x03);//	03
	gc0308_write_reg(client,0x2f, 0x01);
	gc0308_write_reg(client,0x30, 0xf7);
	gc0308_write_reg(client,0x31, 0x50);
	gc0308_write_reg(client,0x32, 0x00);
	gc0308_write_reg(client,0x39, 0x04);
	gc0308_write_reg(client,0x3a, 0x18);
	gc0308_write_reg(client,0x3b, 0x20);
	gc0308_write_reg(client,0x3c, 0x00);
	gc0308_write_reg(client,0x3d, 0x00);
	gc0308_write_reg(client,0x3e, 0x00);
	gc0308_write_reg(client,0x3f, 0x00);
	gc0308_write_reg(client,0x50, 0x10);
	gc0308_write_reg(client,0x53, 0x82);
	gc0308_write_reg(client,0x54, 0x80);
	gc0308_write_reg(client,0x55, 0x80);
	gc0308_write_reg(client,0x56, 0x82);
	gc0308_write_reg(client,0x8b, 0x40);
	gc0308_write_reg(client,0x8c, 0x40);
	gc0308_write_reg(client,0x8d, 0x40);
	gc0308_write_reg(client,0x8e, 0x2e);
	gc0308_write_reg(client,0x8f, 0x2e);
	gc0308_write_reg(client,0x90, 0x2e);
	gc0308_write_reg(client,0x91, 0x3c);
	gc0308_write_reg(client,0x92, 0x50);
	gc0308_write_reg(client,0x5d, 0x12);
	gc0308_write_reg(client,0x5e, 0x1a);
	gc0308_write_reg(client,0x5f, 0x24);
	gc0308_write_reg(client,0x60, 0x07);
	gc0308_write_reg(client,0x61, 0x15);
	gc0308_write_reg(client,0x62, 0x08);
	gc0308_write_reg(client,0x64, 0x03);
	gc0308_write_reg(client,0x66, 0xe8);
	gc0308_write_reg(client,0x67, 0x86);
	gc0308_write_reg(client,0x68, 0xa2);
	gc0308_write_reg(client,0x69, 0x18);
	gc0308_write_reg(client,0x6a, 0x0f);
	gc0308_write_reg(client,0x6b, 0x00);
	gc0308_write_reg(client,0x6c, 0x5f);
	gc0308_write_reg(client,0x6d, 0x8f);
	gc0308_write_reg(client,0x6e, 0x55);
	gc0308_write_reg(client,0x6f, 0x38);
	gc0308_write_reg(client,0x70, 0x15);
	gc0308_write_reg(client,0x71, 0x33);
	gc0308_write_reg(client,0x72, 0xdc);
	gc0308_write_reg(client,0x73, 0x80);
	gc0308_write_reg(client,0x74, 0x02);
	gc0308_write_reg(client,0x75, 0x3f);
	gc0308_write_reg(client,0x76, 0x02);
	gc0308_write_reg(client,0x77, 0x36);
	gc0308_write_reg(client,0x78, 0x88);
	gc0308_write_reg(client,0x79, 0x81);
	gc0308_write_reg(client,0x7a, 0x81);
	gc0308_write_reg(client,0x7b, 0x22);
	gc0308_write_reg(client,0x7c, 0xff);
	gc0308_write_reg(client,0x93, 0x48);
	gc0308_write_reg(client,0x94, 0x00);
	gc0308_write_reg(client,0x95, 0x05);
	gc0308_write_reg(client,0x96, 0xe8);
	gc0308_write_reg(client,0x97, 0x40);
	gc0308_write_reg(client,0x98, 0xf0);
	gc0308_write_reg(client,0xb1, 0x38);
	gc0308_write_reg(client,0xb2, 0x38);
	gc0308_write_reg(client,0xbd, 0x38);
	gc0308_write_reg(client,0xbe, 0x36);
	gc0308_write_reg(client,0xd0, 0xc9);
	gc0308_write_reg(client,0xd1, 0x10);

	gc0308_write_reg(client,0xd3, 0x80);
	gc0308_write_reg(client,0xd5, 0xf2);
	gc0308_write_reg(client,0xd6, 0x16);
	gc0308_write_reg(client,0xdb, 0x92);
	gc0308_write_reg(client,0xdc, 0xa5);
	gc0308_write_reg(client,0xdf, 0x23);
	gc0308_write_reg(client,0xd9, 0x00);
	gc0308_write_reg(client,0xda, 0x00);
	gc0308_write_reg(client,0xe0, 0x09);

	gc0308_write_reg(client,0xed, 0x04);
	gc0308_write_reg(client,0xee, 0xa0);
	gc0308_write_reg(client,0xef, 0x40);
	gc0308_write_reg(client,0x80, 0x03);
	gc0308_write_reg(client,0x80, 0x03);
	gc0308_write_reg(client,0x9F, 0x10);
	gc0308_write_reg(client,0xA0, 0x20);
	gc0308_write_reg(client,0xA1, 0x38);
	gc0308_write_reg(client,0xA2, 0x4E);
	gc0308_write_reg(client,0xA3, 0x63);
	gc0308_write_reg(client,0xA4, 0x76);
	gc0308_write_reg(client,0xA5, 0x87);
	gc0308_write_reg(client,0xA6, 0xA2);
	gc0308_write_reg(client,0xA7, 0xB8);
	gc0308_write_reg(client,0xA8, 0xCA);
	gc0308_write_reg(client,0xA9, 0xD8);
	gc0308_write_reg(client,0xAA, 0xE3);
	gc0308_write_reg(client,0xAB, 0xEB);
	gc0308_write_reg(client,0xAC, 0xF0);
	gc0308_write_reg(client,0xAD, 0xF8);
	gc0308_write_reg(client,0xAE, 0xFD);
	gc0308_write_reg(client,0xAF, 0xFF);
	gc0308_write_reg(client,0xc0, 0x00);
	gc0308_write_reg(client,0xc1, 0x10);
	gc0308_write_reg(client,0xc2, 0x1C);
	gc0308_write_reg(client,0xc3, 0x30);
	gc0308_write_reg(client,0xc4, 0x43);
	gc0308_write_reg(client,0xc5, 0x54);
	gc0308_write_reg(client,0xc6, 0x65);
	gc0308_write_reg(client,0xc7, 0x75);
	gc0308_write_reg(client,0xc8, 0x93);
	gc0308_write_reg(client,0xc9, 0xB0);
	gc0308_write_reg(client,0xca, 0xCB);
	gc0308_write_reg(client,0xcb, 0xE6);
	gc0308_write_reg(client,0xcc, 0xFF);
	gc0308_write_reg(client,0xf0, 0x02);
	gc0308_write_reg(client,0xf1, 0x01);
	gc0308_write_reg(client,0xf2, 0x01);
	gc0308_write_reg(client,0xf3, 0x30);
	gc0308_write_reg(client,0xf9, 0x9f);
	gc0308_write_reg(client,0xfa, 0x78);

	//---------------------------------------------------------------
	gc0308_write_reg(client,0xfe, 0x01);// set page1

	gc0308_write_reg(client,0x00, 0xf5);
	gc0308_write_reg(client,0x02, 0x1a);
	gc0308_write_reg(client,0x0a, 0xa0);
	gc0308_write_reg(client,0x0b, 0x60);
	gc0308_write_reg(client,0x0c, 0x08);
	gc0308_write_reg(client,0x0e, 0x4c);
	gc0308_write_reg(client,0x0f, 0x39);
	gc0308_write_reg(client,0x11, 0x3f);
	gc0308_write_reg(client,0x12, 0x72);
	gc0308_write_reg(client,0x13, 0x13);
	gc0308_write_reg(client,0x14, 0x42);
	gc0308_write_reg(client,0x15, 0x43);
	gc0308_write_reg(client,0x16, 0xc2);
	gc0308_write_reg(client,0x17, 0xa8);
	gc0308_write_reg(client,0x18, 0x18);
	gc0308_write_reg(client,0x19, 0x40);
	gc0308_write_reg(client,0x1a, 0xd0);
	gc0308_write_reg(client,0x1b, 0xf5);
	gc0308_write_reg(client,0x70, 0x40);
	gc0308_write_reg(client,0x71, 0x58);
	gc0308_write_reg(client,0x72, 0x30);
	gc0308_write_reg(client,0x73, 0x48);
	gc0308_write_reg(client,0x74, 0x20);
	gc0308_write_reg(client,0x75, 0x60);
	gc0308_write_reg(client,0x77, 0x20);
	gc0308_write_reg(client,0x78, 0x32);
	gc0308_write_reg(client,0x30, 0x03);
	gc0308_write_reg(client,0x31, 0x40);
	gc0308_write_reg(client,0x32, 0xe0);
	gc0308_write_reg(client,0x33, 0xe0);
	gc0308_write_reg(client,0x34, 0xe0);
	gc0308_write_reg(client,0x35, 0xb0);
	gc0308_write_reg(client,0x36, 0xc0);
	gc0308_write_reg(client,0x37, 0xc0);
	gc0308_write_reg(client,0x38, 0x04);
	gc0308_write_reg(client,0x39, 0x09);
	gc0308_write_reg(client,0x3a, 0x12);
	gc0308_write_reg(client,0x3b, 0x1C);
	gc0308_write_reg(client,0x3c, 0x28);
	gc0308_write_reg(client,0x3d, 0x31);
	gc0308_write_reg(client,0x3e, 0x44);
	gc0308_write_reg(client,0x3f, 0x57);
	gc0308_write_reg(client,0x40, 0x6C);
	gc0308_write_reg(client,0x41, 0x81);
	gc0308_write_reg(client,0x42, 0x94);
	gc0308_write_reg(client,0x43, 0xA7);
	gc0308_write_reg(client,0x44, 0xB8);
	gc0308_write_reg(client,0x45, 0xD6);
	gc0308_write_reg(client,0x46, 0xEE);
	gc0308_write_reg(client,0x47, 0x0d);
	gc0308_write_reg(client,0xfe, 0x00); // set page0

	//-----------Update the registers 2010/07/06-------------//
	//Registers of Page0
	gc0308_write_reg(client,0xfe, 0x00); // set page0
	gc0308_write_reg(client,0x10, 0x26);
	gc0308_write_reg(client,0x11, 0x0d);  // fd,modified by mormo 2010/07/06
	gc0308_write_reg(client,0x1a, 0x2a);  // 1e,modified by mormo 2010/07/06

	gc0308_write_reg(client,0x1c, 0x49); // c1,modified by mormo 2010/07/06
	gc0308_write_reg(client,0x1d, 0x9a); // 08,modified by mormo 2010/07/06
	gc0308_write_reg(client,0x1e, 0x61); // 60,modified by mormo 2010/07/06

	gc0308_write_reg(client,0x3a, 0x20);

	gc0308_write_reg(client,0x50, 0x14);  // 10,modified by mormo 2010/07/06
	gc0308_write_reg(client,0x53, 0x80);
	gc0308_write_reg(client,0x56, 0x80);

	gc0308_write_reg(client,0x8b, 0x20); //LSC
	gc0308_write_reg(client,0x8c, 0x20);
	gc0308_write_reg(client,0x8d, 0x20);
	gc0308_write_reg(client,0x8e, 0x14);
	gc0308_write_reg(client,0x8f, 0x10);
	gc0308_write_reg(client,0x90, 0x14);

	gc0308_write_reg(client,0x94, 0x02);
	gc0308_write_reg(client,0x95, 0x07);
	gc0308_write_reg(client,0x96, 0xe0);

	gc0308_write_reg(client,0xb1, 0x40); // YCPT
	gc0308_write_reg(client,0xb2, 0x40);
	gc0308_write_reg(client,0xb3, 0x40);
	gc0308_write_reg(client,0xb6, 0xe0);

	gc0308_write_reg(client,0xd0, 0xcb); // AECT  c9,modifed by mormo 2010/07/06
#if defined CONFIG_JZ4770_O1
	gc0308_write_reg(client,0xd3, 0x58);
#else
	gc0308_write_reg(client,0xd3, 0x48); // 80,modified by mormor 2010/07/06
#endif
	gc0308_write_reg(client,0xf2, 0x02);
	gc0308_write_reg(client,0xf7, 0x12);
	gc0308_write_reg(client,0xf8, 0x0a);

	//Registers of Page1
	gc0308_write_reg(client,0xfe, 0x01);// set page1
	gc0308_write_reg(client,0x02, 0x20);
	gc0308_write_reg(client,0x04, 0x10);
	gc0308_write_reg(client,0x05, 0x08);
	gc0308_write_reg(client,0x06, 0x20);
	gc0308_write_reg(client,0x08, 0x0a);

	gc0308_write_reg(client,0x0e, 0x44);
	gc0308_write_reg(client,0x0f, 0x32);
	gc0308_write_reg(client,0x10, 0x41);
	gc0308_write_reg(client,0x11, 0x37);
	gc0308_write_reg(client,0x12, 0x22);
	gc0308_write_reg(client,0x13, 0x19);
	gc0308_write_reg(client,0x14, 0x44);
	gc0308_write_reg(client,0x15, 0x44);

	gc0308_write_reg(client,0x19, 0x50);
	gc0308_write_reg(client,0x1a, 0xd8);

	gc0308_write_reg(client,0x32, 0x10);

	gc0308_write_reg(client,0x35, 0x00);
	gc0308_write_reg(client,0x36, 0x80);
	gc0308_write_reg(client,0x37, 0x00);
	//-----------Update the registers end---------//

	gc0308_write_reg(client,0xfe, 0x00); // set page0
	gc0308_write_reg(client,0xd2, 0x90);

	//-----------GAMMA Select(3)---------------//
	gc0308_write_reg(client,0x9F, 0x10);
	gc0308_write_reg(client,0xA0, 0x20);
	gc0308_write_reg(client,0xA1, 0x38);
	gc0308_write_reg(client,0xA2, 0x4E);
	gc0308_write_reg(client,0xA3, 0x63);
	gc0308_write_reg(client,0xA4, 0x76);
	gc0308_write_reg(client,0xA5, 0x87);
	gc0308_write_reg(client,0xA6, 0xA2);
	gc0308_write_reg(client,0xA7, 0xB8);
	gc0308_write_reg(client,0xA8, 0xCA);
	gc0308_write_reg(client,0xA9, 0xD8);
	gc0308_write_reg(client,0xAA, 0xE3);
	gc0308_write_reg(client,0xAB, 0xEB);
	gc0308_write_reg(client,0xAC, 0xF0);
	gc0308_write_reg(client,0xAD, 0xF8);
	gc0308_write_reg(client,0xAE, 0xFD);
	gc0308_write_reg(client,0xAF, 0xFF);

#ifndef CONFIG_HORIZONTAL_MIRROR
	#ifndef CONFIG_VERTICAL_MIRROR
	gc0308_write_reg(client,0x14 , 0x10);
	#else
	gc0308_write_reg(client,0x14 , 0x12);
	#endif
#else
	#ifndef CONFIG_VERTICAL_MIRROR
	gc0308_write_reg(client,0x14 , 0x11);
	#else
	gc0308_write_reg(client,0x14 , 0x13);
	#endif
#endif
	return 0;
}

int gc0308_preview_set(struct cim_sensor *sensor_info)
{

	struct gc0308_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
	return 0;
}


int gc0308_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
	struct gc0308_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"gc0308 size switch %d * %d\n",width,height);
#endif

	if(width == 640 && height == 480)
	{
		gc0308_write_reg(client,0xfe,0x01);
		gc0308_write_reg(client,0x54,0x11);
		gc0308_write_reg(client,0x55,0x03);
		gc0308_write_reg(client,0x56,0x00);
		gc0308_write_reg(client,0x57,0x00);
		gc0308_write_reg(client,0x58,0x00);
		gc0308_write_reg(client,0x59,0x00);
		gc0308_write_reg(client,0xfe,0x00);
		gc0308_write_reg(client,0x46,0x00);

	}
	else if(width == 352 && height == 288)
	{
		gc0308_write_reg(client,0xfe,0x01);
		gc0308_write_reg(client,0x54,0x55);
		gc0308_write_reg(client,0x55,0x03);
		gc0308_write_reg(client,0x56,0x02);
		gc0308_write_reg(client,0x57,0x04);
		gc0308_write_reg(client,0x58,0x02);
		gc0308_write_reg(client,0x59,0x04);
		gc0308_write_reg(client,0xfe,0x00);
		gc0308_write_reg(client,0x46,0x80);
		gc0308_write_reg(client,0x47,0x00);
		gc0308_write_reg(client,0x48,0x00);
		gc0308_write_reg(client,0x49,0x01);
		gc0308_write_reg(client,0x4a,0x20);
		gc0308_write_reg(client,0x4b,0x01);
		gc0308_write_reg(client,0x4c,0x60);
	}
	else if(width == 176 && height == 144)
	{
		gc0308_write_reg(client,0xfe,0x01);
		gc0308_write_reg(client,0x54,0x33);
		gc0308_write_reg(client,0x55,0x03);
		gc0308_write_reg(client,0x56,0x00);
		gc0308_write_reg(client,0x57,0x00);
		gc0308_write_reg(client,0x58,0x00);
		gc0308_write_reg(client,0x59,0x00);
		gc0308_write_reg(client,0xfe,0x00);
		gc0308_write_reg(client,0x46,0x80);
		gc0308_write_reg(client,0x47,0x00);
		gc0308_write_reg(client,0x48,0x00);
		gc0308_write_reg(client,0x49,0x00);
		gc0308_write_reg(client,0x4a,0x90);
		gc0308_write_reg(client,0x4b,0x00);
		gc0308_write_reg(client,0x4c,0xb0);
	}
	else if(width == 320 && height == 240)
	{
		gc0308_write_reg(client,0xfe,0x01);
		gc0308_write_reg(client,0x54,0x55);
		gc0308_write_reg(client,0x55,0x03);
		gc0308_write_reg(client,0x56,0x02);
		gc0308_write_reg(client,0x57,0x04);
		gc0308_write_reg(client,0x58,0x02);
		gc0308_write_reg(client,0x59,0x04);
		gc0308_write_reg(client,0xfe,0x00);
		gc0308_write_reg(client,0x46,0x80);
		gc0308_write_reg(client,0x47,0x00);
		gc0308_write_reg(client,0x48,0x00);
		gc0308_write_reg(client,0x49,0x00);
		gc0308_write_reg(client,0x4a,0xf0);
		gc0308_write_reg(client,0x4b,0x01);
		gc0308_write_reg(client,0x4c,0x40);

	}
	else
		return 0;


	return 0;
}

int gc0308_capture_set(struct cim_sensor *sensor_info)
{

	struct gc0308_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"capture\n");
#endif
	return 0;
}


void gc0308_set_ab_50hz(struct i2c_client *client)
{
	gc0308_write_reg(client,0x01,0x6a);
	gc0308_write_reg(client,0x02,0x70);
	gc0308_write_reg(client,0x0f,0x00);
	gc0308_write_reg(client,0xe2,0x00);
	gc0308_write_reg(client,0xe3,0x96);
	gc0308_write_reg(client,0xe4,0x02);
	gc0308_write_reg(client,0xe5,0x58);
	gc0308_write_reg(client,0xe6,0x02);
	gc0308_write_reg(client,0xe7,0x58);
#if 0
	gc0308_write_reg(client,0xe8,0x04);
	gc0308_write_reg(client,0xe9,0xb0);
#else
	gc0308_write_reg(client,0xe8,0x02);
	gc0308_write_reg(client,0xe9,0x58);
#endif
	gc0308_write_reg(client,0xea,0x0b);
	gc0308_write_reg(client,0xeb,0xb8);
}

void gc0308_set_ab_60hz(struct i2c_client *client)
{
	gc0308_write_reg(client,0x01,0x2c);
	gc0308_write_reg(client,0x02,0x98);
	gc0308_write_reg(client,0x0f,0x02);
	gc0308_write_reg(client,0xe2,0x00);
	gc0308_write_reg(client,0xe3,0x50);
	gc0308_write_reg(client,0xe4,0x02);
	gc0308_write_reg(client,0xe5,0x80);
	gc0308_write_reg(client,0xe6,0x03);
	gc0308_write_reg(client,0xe7,0xc0);
	gc0308_write_reg(client,0xe8,0x05);
	gc0308_write_reg(client,0xe9,0x00);
	gc0308_write_reg(client,0xea,0x09);
	gc0308_write_reg(client,0xeb,0x60);
}

int gc0308_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gc0308_sensor *s;
	struct i2c_client * client;
	unsigned char *str_antibanding;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"gc0308_set_antibanding");
#endif
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			gc0308_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
		case ANTIBANDING_50HZ :
			gc0308_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_50HZ";
			break;
		case ANTIBANDING_60HZ :
			gc0308_set_ab_60hz(client);
			str_antibanding = "ANTIBANDING_60HZ";
			break;
		case ANTIBANDING_OFF :
			str_antibanding = "ANTIBANDING_OFF";
			break;
		default:
			gc0308_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
	}
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev, "set antibanding to %s\n", str_antibanding);
#endif
	return 0;
}

void gc0308_set_effect_normal(struct i2c_client *client)
{
	gc0308_write_reg(client,0x23,0x00);
	gc0308_write_reg(client,0x2d,0x0a);
	gc0308_write_reg(client,0x20,0x7f);
	gc0308_write_reg(client,0xd2,0x90);
	gc0308_write_reg(client,0x73,0x00);
	gc0308_write_reg(client,0x77,0x38);
	gc0308_write_reg(client,0xb3,0x40);
	gc0308_write_reg(client,0xb4,0x80);
	gc0308_write_reg(client,0xba,0x00);
	gc0308_write_reg(client,0xbb,0x00);
}

void gc0308_set_effect_grayscale(struct i2c_client *client)
{
	gc0308_write_reg(client,0x23,0x02);
	gc0308_write_reg(client,0x2d,0x0a);
	gc0308_write_reg(client,0x20,0xff);
	gc0308_write_reg(client,0xd2,0x90);
	gc0308_write_reg(client,0x73,0x00);
	gc0308_write_reg(client,0xb3,0x40);
	gc0308_write_reg(client,0xb4,0x80);
	gc0308_write_reg(client,0xba,0x00);
	gc0308_write_reg(client,0xbb,0x00);
}

void gc0308_set_effect_sepia(struct i2c_client *client)
{
	gc0308_write_reg(client,0x23,0x02);
	gc0308_write_reg(client,0x2d,0x0a);
	gc0308_write_reg(client,0x20,0xff);
	gc0308_write_reg(client,0xd2,0x90);
	gc0308_write_reg(client,0x73,0x00);
	gc0308_write_reg(client,0xb3,0x40);
	gc0308_write_reg(client,0xb4,0x80);
	gc0308_write_reg(client,0xba,0xd0);
	gc0308_write_reg(client,0xbb,0x28);
}

void gc0308_set_effect_colorinv(struct i2c_client *client)
{
	gc0308_write_reg(client,0x23,0x01);
	gc0308_write_reg(client,0x2d,0x0a);
	gc0308_write_reg(client,0x20,0xff);
	gc0308_write_reg(client,0xd2,0x90);
	gc0308_write_reg(client,0x73,0x00);
	gc0308_write_reg(client,0xb3,0x40);
	gc0308_write_reg(client,0xb4,0x80);
	gc0308_write_reg(client,0xba,0x00);
	gc0308_write_reg(client,0xbb,0x00);
}

void gc0308_set_effect_sepiagreen(struct i2c_client *client)
{
	gc0308_write_reg(client,0x23,0x02);
	gc0308_write_reg(client,0x2d,0x0a);
	gc0308_write_reg(client,0x20,0x7f);
	gc0308_write_reg(client,0xd2,0x90);
	gc0308_write_reg(client,0x77,0x88);
	gc0308_write_reg(client,0xb3,0x40);
	gc0308_write_reg(client,0xb4,0x80);
	gc0308_write_reg(client,0xba,0xc0);
	gc0308_write_reg(client,0xbb,0xc0);
}

void gc0308_set_effect_sepiablue(struct i2c_client *client)
{
	gc0308_write_reg(client,0x23,0x02);
	gc0308_write_reg(client,0x2d,0x0a);
	gc0308_write_reg(client,0x20,0x7f);
	gc0308_write_reg(client,0xd2,0x90);
	gc0308_write_reg(client,0x73,0x00);
	gc0308_write_reg(client,0xb3,0x40);
	gc0308_write_reg(client,0xb4,0x80);
	gc0308_write_reg(client,0xba,0x50);
	gc0308_write_reg(client,0xbb,0xe0);
}

int gc0308_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gc0308_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;

	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"gc0308_set_effect");
#endif
	switch(arg)
	{
		case EFFECT_NONE:
			gc0308_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			gc0308_set_effect_grayscale(client);
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			gc0308_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			gc0308_set_effect_sepia(client);
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
			gc0308_set_effect_sepiagreen(client);
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
			gc0308_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"set effect to %s\n", str_effect);
#endif
	return 0;
}


void gc0308_set_wb_auto(struct i2c_client *client)
{
	gc0308_write_reg(client,0x5a,0x56);
	gc0308_write_reg(client,0x5b,0x40);
	gc0308_write_reg(client,0x5c,0x4a);
	gc0308_write_reg(client,0x22,0x57);
}

void gc0308_set_wb_cloud(struct i2c_client *client)
{
	gc0308_write_reg(client,0x22,0x55);
	gc0308_write_reg(client,0x5a,0x8c);
	gc0308_write_reg(client,0x5b,0x50);
	gc0308_write_reg(client,0x5c,0x40);
}

void gc0308_set_wb_daylight(struct i2c_client *client)
{
	gc0308_write_reg(client,0x22,0x55);
	gc0308_write_reg(client,0x5a,0x74);
	gc0308_write_reg(client,0x5b,0x52);
	gc0308_write_reg(client,0x5c,0x40);
}

void gc0308_set_wb_incandescence(struct i2c_client *client)
{
	gc0308_write_reg(client,0x22,0x55);
	gc0308_write_reg(client,0x5a,0x48);
	gc0308_write_reg(client,0x5b,0x40);
	gc0308_write_reg(client,0x5c,0x5c);
}

void gc0308_set_wb_fluorescent(struct i2c_client *client)
{
	gc0308_write_reg(client,0x22,0x55);
	gc0308_write_reg(client,0x5a,0x40);
	gc0308_write_reg(client,0x5b,0x42);
	gc0308_write_reg(client,0x5c,0x50);
}

void gc0308_set_wb_tungsten(struct i2c_client *client)
{
	gc0308_write_reg(client,0x22,0x55);
	gc0308_write_reg(client,0x5a,0x40);
	gc0308_write_reg(client,0x5b,0x54);
	gc0308_write_reg(client,0x5c,0x70);
}

int gc0308_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gc0308_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct gc0308_sensor, cs);
	client = s->client;
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"gc0308_set_balance");
#endif
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			gc0308_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			gc0308_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			gc0308_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			gc0308_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			gc0308_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			gc0308_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
#ifdef GC0308_SET_KERNEL_PRINT
	dev_info(&client->dev,"set balance to %s\n", str_balance);
#endif
	return 0;
}

