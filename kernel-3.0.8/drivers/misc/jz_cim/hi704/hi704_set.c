#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "hi704_camera.h"

//#define HI704_SET_KERNEL_PRINT

int hi704_init(struct cim_sensor *sensor_info)
{
	struct hi704_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi704 init\n");
#endif

	hi704_write_reg(client,0x01, 0xf1);
	hi704_write_reg(client,0x01, 0xf3);
	hi704_write_reg(client,0x01, 0xf1);
	hi704_write_reg(client,0x03, 0x00);//Page 0
	hi704_write_reg(client,0x10, 0x00);
	hi704_write_reg(client,0x12, 0x00);
	hi704_write_reg(client,0x20, 0x00);
	hi704_write_reg(client,0x21, 0x04);
	hi704_write_reg(client,0x22, 0x00);
	hi704_write_reg(client,0x23, 0x04);
	hi704_write_reg(client,0x40, 0x01);
	hi704_write_reg(client,0x41, 0x58);
	hi704_write_reg(client,0x42, 0x00);
	hi704_write_reg(client,0x43, 0x04);
//BLC
	hi704_write_reg(client,0x80, 0x2e);
	hi704_write_reg(client,0x81, 0x7e);	
	hi704_write_reg(client,0x82, 0x90);
	hi704_write_reg(client,0x83, 0x30);
	hi704_write_reg(client,0x84, 0x2c);
	hi704_write_reg(client,0x85, 0x4b);
	hi704_write_reg(client,0x89, 0x48);
	hi704_write_reg(client,0x90, 0x0b);
	hi704_write_reg(client,0x91, 0x0b);
	hi704_write_reg(client,0x92, 0x48);
	hi704_write_reg(client,0x93, 0x48);
	hi704_write_reg(client,0x98, 0x38);
	hi704_write_reg(client,0x99, 0x40);
	hi704_write_reg(client,0xa0, 0x00);
	hi704_write_reg(client,0xa8, 0x40);
	hi704_write_reg(client,0x03, 0x02 );//Page 2
	hi704_write_reg(client,0x13, 0x40 );
	hi704_write_reg(client,0x14, 0x04 );
	hi704_write_reg(client,0x1a, 0x00 );
	hi704_write_reg(client,0x1b, 0x08 );
	hi704_write_reg(client,0x20, 0x33 );
	hi704_write_reg(client,0x21, 0xaa );
	hi704_write_reg(client,0x22, 0xa7 );
	hi704_write_reg(client,0x23, 0x32 );
	hi704_write_reg(client,0x3b, 0x48 );
	hi704_write_reg(client,0x50, 0x21 );
	hi704_write_reg(client,0x52, 0xa2 );
	hi704_write_reg(client,0x53, 0x0a );
	hi704_write_reg(client,0x54, 0x30 );
	hi704_write_reg(client,0x55, 0x10 );
	hi704_write_reg(client,0x56, 0x0c );
	hi704_write_reg(client,0x59, 0x0f );
	hi704_write_reg(client,0x60, 0xca );
	hi704_write_reg(client,0x61, 0xdb );
	hi704_write_reg(client,0x62, 0xca );
	hi704_write_reg(client,0x63, 0xda );
	hi704_write_reg(client,0x64, 0xca );
	hi704_write_reg(client,0x65, 0xda );
	hi704_write_reg(client,0x72, 0xcb );
	hi704_write_reg(client,0x73, 0xd8 );
	hi704_write_reg(client,0x74, 0xcb );
	hi704_write_reg(client,0x75, 0xd8 );
	hi704_write_reg(client,0x80, 0x02 );
	hi704_write_reg(client,0x81, 0xbd );
	hi704_write_reg(client,0x82, 0x24 );
	hi704_write_reg(client,0x83, 0x3e );
	hi704_write_reg(client,0x84, 0x24 );
	hi704_write_reg(client,0x85, 0x3e );
	hi704_write_reg(client,0x92, 0x72 );
	hi704_write_reg(client,0x93, 0x8c );
	hi704_write_reg(client,0x94, 0x72 );
	hi704_write_reg(client,0x95, 0x8c );
	hi704_write_reg(client,0xa0, 0x03 );
	hi704_write_reg(client,0xa1, 0xbb );
	hi704_write_reg(client,0xa4, 0xbb );
	hi704_write_reg(client,0xa5, 0x03 );
	hi704_write_reg(client,0xa8, 0x44 );
	hi704_write_reg(client,0xa9, 0x6a );
	hi704_write_reg(client,0xaa, 0x92 );
	hi704_write_reg(client,0xab, 0xb7 );
	hi704_write_reg(client,0xb8, 0xc9 );
	hi704_write_reg(client,0xb9, 0xd0 );
	hi704_write_reg(client,0xbc, 0x20 );
	hi704_write_reg(client,0xbd, 0x28 );
	hi704_write_reg(client,0xc0, 0xde );
	hi704_write_reg(client,0xc1, 0xec );
	hi704_write_reg(client,0xc2, 0xde );
	hi704_write_reg(client,0xc3, 0xec );
	hi704_write_reg(client,0xc4, 0xe0 );
	hi704_write_reg(client,0xc5, 0xea );
	hi704_write_reg(client,0xc6, 0xe0 );
	hi704_write_reg(client,0xc7, 0xea );
	hi704_write_reg(client,0xc8, 0xe1 );
	hi704_write_reg(client,0xc9, 0xe8 );
	hi704_write_reg(client,0xca, 0xe1 );
	hi704_write_reg(client,0xcb, 0xe8 );
	hi704_write_reg(client,0xcc, 0xe2 );
	hi704_write_reg(client,0xcd, 0xe7 );
	hi704_write_reg(client,0xce, 0xe2 );
	hi704_write_reg(client,0xcf, 0xe7 );
	hi704_write_reg(client,0xd0, 0xc8 );
	hi704_write_reg(client,0xd1, 0xef );

	hi704_write_reg(client,0x03, 0x10 );//Page 10
	hi704_write_reg(client,0x10, 0x03);//ISPCTL1(Control the format of image data) : YUV order for Lot51 by NHJ
	hi704_write_reg(client,0x11, 0x43);
	hi704_write_reg(client,0x12, 0x30);
	hi704_write_reg(client,0x40, 0x80);//brightness
	hi704_write_reg(client,0x41, 0x10);
	hi704_write_reg(client,0x48, 0x84);
	hi704_write_reg(client,0x50, 0x48);
	hi704_write_reg(client,0x60, 0x7f);
	hi704_write_reg(client,0x61, 0x00);
	hi704_write_reg(client,0x62, 0xa0);
	hi704_write_reg(client,0x63, 0x90);// 50
	hi704_write_reg(client,0x64, 0x48);
	hi704_write_reg(client,0x66, 0x90);
	hi704_write_reg(client,0x67, 0x36);
	hi704_write_reg(client,0x03, 0x11);//Page 11  LPF
	hi704_write_reg(client,0x10, 0x25);
	hi704_write_reg(client,0x11, 0x1f); //open LPF in dark, 0x0a close
	hi704_write_reg(client,0x20, 0x00);
	hi704_write_reg(client,0x21, 0x38);
	hi704_write_reg(client,0x23, 0x0a);
	hi704_write_reg(client,0x60, 0x10);
	hi704_write_reg(client,0x61, 0x82);
	hi704_write_reg(client,0x62, 0x00);
	hi704_write_reg(client,0x63, 0x83);
	hi704_write_reg(client,0x64, 0x83);
	hi704_write_reg(client,0x67, 0xf0);
	hi704_write_reg(client,0x68, 0x30);
	hi704_write_reg(client,0x69, 0x10);
	hi704_write_reg(client,0x03, 0x12);//Page 12  2D
	hi704_write_reg(client,0x40, 0xe9);
	hi704_write_reg(client,0x41, 0x09);
	hi704_write_reg(client,0x50, 0x18);
	hi704_write_reg(client,0x51, 0x24);
	hi704_write_reg(client,0x70, 0x1f);
	hi704_write_reg(client,0x71, 0x00);
	hi704_write_reg(client,0x72, 0x00);
	hi704_write_reg(client,0x73, 0x00);
	hi704_write_reg(client,0x74, 0x10);
	hi704_write_reg(client,0x75, 0x10);
	hi704_write_reg(client,0x76, 0x20);
	hi704_write_reg(client,0x77, 0x80);
	hi704_write_reg(client,0x78, 0x88);
	hi704_write_reg(client,0x79, 0x18);
	hi704_write_reg(client,0xb0, 0x7d);
	hi704_write_reg(client,0x03, 0x13);//Page 13
	hi704_write_reg(client,0x10, 0x01);
	hi704_write_reg(client,0x11, 0x89);
	hi704_write_reg(client,0x12, 0x14);
	hi704_write_reg(client,0x13, 0x19);
	hi704_write_reg(client,0x14, 0x08);
	hi704_write_reg(client,0x20, 0x06);
	hi704_write_reg(client,0x21, 0x03);
	hi704_write_reg(client,0x23, 0x30);
	hi704_write_reg(client,0x24, 0x33);
	hi704_write_reg(client,0x25, 0x08);
	hi704_write_reg(client,0x26, 0x18);
	hi704_write_reg(client,0x27, 0x00);
	hi704_write_reg(client,0x28, 0x08);
	hi704_write_reg(client,0x29, 0x50);
	hi704_write_reg(client,0x2a, 0xe0);
	hi704_write_reg(client,0x2b, 0x10);
	hi704_write_reg(client,0x2c, 0x28);
	hi704_write_reg(client,0x2d, 0x40);
	hi704_write_reg(client,0x2e, 0x00);
	hi704_write_reg(client,0x2f, 0x00);
	hi704_write_reg(client,0x30, 0x11);
	hi704_write_reg(client,0x80, 0x03);
	hi704_write_reg(client,0x81, 0x07);
	hi704_write_reg(client,0x90, 0x04);
	hi704_write_reg(client,0x91, 0x02);
	hi704_write_reg(client,0x92, 0x00);
	hi704_write_reg(client,0x93, 0x20);
	hi704_write_reg(client,0x94, 0x42);
	hi704_write_reg(client,0x95, 0x60);
	hi704_write_reg(client,0x03, 0x14);//Page 14
	hi704_write_reg(client,0x10, 0x01);
	hi704_write_reg(client,0x20, 0x60);
	hi704_write_reg(client,0x21, 0x80);
	hi704_write_reg(client,0x22, 0x66);
	hi704_write_reg(client,0x23, 0x50);
	hi704_write_reg(client,0x24, 0x44);
	hi704_write_reg(client,0x03, 0x15);//Page 15
	hi704_write_reg(client,0x10, 0x03);
	hi704_write_reg(client,0x14, 0x3c);
	hi704_write_reg(client,0x16, 0x2c);
	hi704_write_reg(client,0x17, 0x2f);
	hi704_write_reg(client,0x30, 0xcb);
	hi704_write_reg(client,0x31, 0x61);
	hi704_write_reg(client,0x32, 0x16);
	hi704_write_reg(client,0x33, 0x23);
	hi704_write_reg(client,0x34, 0xce);
	hi704_write_reg(client,0x35, 0x2b);
	hi704_write_reg(client,0x36, 0x01);
	hi704_write_reg(client,0x37, 0x34);
	hi704_write_reg(client,0x38, 0x75);
	hi704_write_reg(client,0x40, 0x87);
	hi704_write_reg(client,0x41, 0x18);
	hi704_write_reg(client,0x42, 0x91);
	hi704_write_reg(client,0x43, 0x94);
	hi704_write_reg(client,0x44, 0x9f);
	hi704_write_reg(client,0x45, 0x33);
	hi704_write_reg(client,0x46, 0x00);
	hi704_write_reg(client,0x47, 0x94);
	hi704_write_reg(client,0x48, 0x14);
	hi704_write_reg(client,0x03, 0x16);//Page 16
	hi704_write_reg(client,0x30, 0x00);// original
	hi704_write_reg(client,0x31, 0x0a);
	hi704_write_reg(client,0x32, 0x1b);
	hi704_write_reg(client,0x33, 0x2e);
	hi704_write_reg(client,0x34, 0x5c);
	hi704_write_reg(client,0x35, 0x79);
	hi704_write_reg(client,0x36, 0x95);
	hi704_write_reg(client,0x37, 0xa4);
	hi704_write_reg(client,0x38, 0xb1);
	hi704_write_reg(client,0x39, 0xbd);

	hi704_write_reg(client,0x3a, 0xc8);
	hi704_write_reg(client,0x3b, 0xd9);
	hi704_write_reg(client,0x3c, 0xe8);
	hi704_write_reg(client,0x3d, 0xf5);
	hi704_write_reg(client,0x3e, 0xff);
	hi704_write_reg(client,0x03, 0x17);//page 17
	hi704_write_reg(client,0xc4, 0x3c);
	hi704_write_reg(client,0xc5, 0x32);
	hi704_write_reg(client,0x03, 0x20);//page 20
	hi704_write_reg(client,0x10, 0x0c);
	hi704_write_reg(client,0x11, 0x04);
	hi704_write_reg(client,0x20, 0x01);
	hi704_write_reg(client,0x28, 0x27);//
	hi704_write_reg(client,0x29, 0xa1);
	hi704_write_reg(client,0x2a, 0xf0);
	hi704_write_reg(client,0x2b, 0x34);
	hi704_write_reg(client,0x2c, 0x2b);
	hi704_write_reg(client,0x30, 0xf8); //new add by peter
	hi704_write_reg(client,0x39, 0x22);
	hi704_write_reg(client,0x3a, 0xde);
	hi704_write_reg(client,0x3b, 0x22);
	hi704_write_reg(client,0x3c, 0xde);
	hi704_write_reg(client,0x60, 0x95);
	hi704_write_reg(client,0x68, 0x3c);
	hi704_write_reg(client,0x69, 0x64);
	hi704_write_reg(client,0x6a, 0x28);
	hi704_write_reg(client,0x6b, 0xc8);
	hi704_write_reg(client,0x70, 0x42);
	hi704_write_reg(client,0x76, 0x22);
	hi704_write_reg(client,0x77, 0x02);
	hi704_write_reg(client,0x78, 0x12);
	hi704_write_reg(client,0x79, 0x27);
	hi704_write_reg(client,0x7a, 0x23);
	hi704_write_reg(client,0x7c, 0x1d);
	hi704_write_reg(client,0x7d, 0x22);
//50Hz
	hi704_write_reg(client,0x83, 0x00); //;;ExpNormal 30Fps
	hi704_write_reg(client,0x84, 0xc3);
	hi704_write_reg(client,0x85, 0x4b);
	hi704_write_reg(client,0x86, 0x00);//ExpMin
	hi704_write_reg(client,0x87, 0xfa);
//50Hz 8fps
	//hi704_write_reg(client,0x88, 0x02);
	//hi704_write_reg(client,0x89, 0x49);
	//hi704_write_reg(client,0x8a, 0xf0);
//    hi704_write_reg(client,0x88, 0x01);//ExpMin 10Fps
//	hi704_write_reg(client,0x89, 0x24);
//	hi704_write_reg(client,0x8a, 0xf8);
	hi704_write_reg(client,0x8b, 0x3a);//EXP100
	hi704_write_reg(client,0x8c, 0x98);
	hi704_write_reg(client,0x8d, 0x30);//EXP120
	hi704_write_reg(client,0x8e, 0xd4);
	hi704_write_reg(client,0x91, 0x02);
	hi704_write_reg(client,0x92, 0xdc);
	hi704_write_reg(client,0x93, 0x6c);
	hi704_write_reg(client,0x94, 0x01);
	hi704_write_reg(client,0x95, 0xb7);
	hi704_write_reg(client,0x96, 0x74);
	hi704_write_reg(client,0x98, 0x8c);
	hi704_write_reg(client,0x99, 0x23);
	hi704_write_reg(client,0x9c, 0x0b);//new add by peter
	hi704_write_reg(client,0x9d, 0x3b);
	hi704_write_reg(client,0x9e, 0x00);
	hi704_write_reg(client,0x9f, 0xfa);
//	hi704_write_reg(client,0xb0, 0x1a);//AG
	hi704_write_reg(client,0xb1, 0x14);//AGMIN
	hi704_write_reg(client,0xb2, 0x50);//AGMAX 80
//	hi704_write_reg(client,0xb3, 0x1a);//AGLVL
	hi704_write_reg(client,0xb4, 0x14);//AGTH1
	hi704_write_reg(client,0xb5, 0x38);//AGTH2
	hi704_write_reg(client,0xb6, 0x26);//AGBTH1
	hi704_write_reg(client,0xb7, 0x20);//AGBTH2
	hi704_write_reg(client,0xb8, 0x1d);//AGBTH3
	hi704_write_reg(client,0xb9, 0x1b);//AGBTH4
	hi704_write_reg(client,0xba, 0x1a);//AGBTH5
	hi704_write_reg(client,0xbb, 0x19);//AGBTH6
	hi704_write_reg(client,0xbc, 0x19);//AGBTH7
	hi704_write_reg(client,0xbd, 0x18);//AGBTH8
	hi704_write_reg(client,0xc0, 0x1a);//PGA_Sky_gain
	hi704_write_reg(client,0xc3, 0x48);// 60
	hi704_write_reg(client,0xc4, 0x48);// 58
	hi704_write_reg(client,0x03, 0x22);//page 22
	hi704_write_reg(client,0x10, 0xe2);
	hi704_write_reg(client,0x11, 0x26);
	hi704_write_reg(client,0x21, 0x40);
	hi704_write_reg(client,0x30, 0x80);
	hi704_write_reg(client,0x31, 0x80);
	hi704_write_reg(client,0x38, 0x12);
	hi704_write_reg(client,0x39, 0x33);
	hi704_write_reg(client,0x40, 0xf0);
	hi704_write_reg(client,0x41, 0x33);
	hi704_write_reg(client,0x42, 0x33);
	hi704_write_reg(client,0x43, 0xf3);
	hi704_write_reg(client,0x44, 0x55);
	hi704_write_reg(client,0x45, 0x44);
	hi704_write_reg(client,0x46, 0x02);
	hi704_write_reg(client,0x80, 0x45);//R Gain
	hi704_write_reg(client,0x81, 0x20);//G Gain
	hi704_write_reg(client,0x82, 0x48);//B Gain

	hi704_write_reg(client,0x83, 0x52);//RMAX
	hi704_write_reg(client,0x84, 0x1b);//RMIN
	hi704_write_reg(client,0x85, 0x50);//BMAX
	hi704_write_reg(client,0x86, 0x25);//BMIN
	hi704_write_reg(client,0x87, 0x4d);//RMAXB
	hi704_write_reg(client,0x88, 0x38);//RMINB
	hi704_write_reg(client,0x89, 0x3e);//BMAXB
	hi704_write_reg(client,0x8a, 0x02);//BMINB
	hi704_write_reg(client,0x8b, 0x02);
	hi704_write_reg(client,0x8d, 0x22);
	hi704_write_reg(client,0x8e, 0x71);
	hi704_write_reg(client,0x8f, 0x63);
	hi704_write_reg(client,0x90, 0x60);
	hi704_write_reg(client,0x91, 0x5c);
	hi704_write_reg(client,0x92, 0x56);
	hi704_write_reg(client,0x93, 0x52);
	hi704_write_reg(client,0x94, 0x4c);
	hi704_write_reg(client,0x95, 0x36);
	hi704_write_reg(client,0x96, 0x31);
	hi704_write_reg(client,0x97, 0x2e);
	hi704_write_reg(client,0x98, 0x2a);
	hi704_write_reg(client,0x99, 0x29);
	hi704_write_reg(client,0x9a, 0x26);
	hi704_write_reg(client,0x9b, 0x09);
	hi704_write_reg(client,0x03, 0x22);
	hi704_write_reg(client,0x10, 0xfb);
//	hi704_write_reg(client,0x10, 0xea);//AWB ON
//	hi704_write_reg(client,0x03, 0x20);//page 20
//	hi704_write_reg(client,0x10, 0x9c);//AE ON
//	hi704_write_reg(client,0x01, 0xf0);
//      hi704_write_reg(client,0x03,0x13);
//      hi704_write_reg(client,0x20,0x02);
//      hi704_write_reg(client,0x21,0x02);
//      hi704_write_reg(client,0x90,0x02);
//      hi704_write_reg(client,0x91,0x02);
	hi704_write_reg(client,0x03, 0x00);//Page 0
	hi704_write_reg(client,0x11, 0x90);//rotate
	//hi704_write_reg(client,0x03, 0x02);//Page 2
	//hi704_write_reg(client,0x1a, 0x21);
	hi704_write_reg(client,0x03,0x20);
	hi704_write_reg(client,0x10,0x9c);
	hi704_write_reg(client,0x01,0xf0);

	return 0;
}

int hi704_preview_set(struct cim_sensor *sensor_info)
{

	struct hi704_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
	return 0;
}


int hi704_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
	struct hi704_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
	if(width == 352 && height == 288)
	{  
		hi704_write_reg(client,0x03, 0x00);
		hi704_write_reg(client,0x01, 0xf1);
		hi704_write_reg(client,0x12, 0x04);
		hi704_write_reg(client,0x20, 0x00);
		hi704_write_reg(client,0x21, 0x00);
		hi704_write_reg(client,0x22, 0x00);
		hi704_write_reg(client,0x23, 0x1a);
		hi704_write_reg(client,0x24, 0x01);
		hi704_write_reg(client,0x25, 0xe0);
		hi704_write_reg(client,0x26, 0x02);
		hi704_write_reg(client,0x27, 0x4c);
		hi704_write_reg(client,0x03, 0x18);
      		hi704_write_reg(client,0x10, 0x03);
		hi704_write_reg(client,0x11, 0x00);
		hi704_write_reg(client,0x12, 0x00);
		hi704_write_reg(client,0x13, 0x00);
		hi704_write_reg(client,0x14, 0x02);
		hi704_write_reg(client,0x15, 0x58);
		hi704_write_reg(client,0x16, 0x48);
		hi704_write_reg(client,0x17, 0x0d);
		hi704_write_reg(client,0x18, 0x11);
		hi704_write_reg(client,0x19, 0x0d);
		hi704_write_reg(client,0x1a, 0x55);
		hi704_write_reg(client,0x03, 0x00);
		hi704_write_reg(client,0x01, 0xf0);
	}  //fish mark that trim size is 352x280 

	else if(width == 176 && height == 144)
	{
		hi704_write_reg(client,0x03, 0x00);
		hi704_write_reg(client,0x01, 0xf1);
		hi704_write_reg(client,0x12, 0x04);
		hi704_write_reg(client,0x20, 0x00);
		hi704_write_reg(client,0x21, 0x00);
		hi704_write_reg(client,0x22, 0x00);
		hi704_write_reg(client,0x23, 0x1a);
		hi704_write_reg(client,0x24, 0x01);
		hi704_write_reg(client,0x25, 0xe0);
		hi704_write_reg(client,0x26, 0x02);
		hi704_write_reg(client,0x27, 0x4c);
		hi704_write_reg(client,0x03, 0x18);
      		hi704_write_reg(client,0x10, 0x03);
		hi704_write_reg(client,0x11, 0x00);
		hi704_write_reg(client,0x12, 0x00);
		hi704_write_reg(client,0x13, 0x00);
		hi704_write_reg(client,0x14, 0x02);
		hi704_write_reg(client,0x15, 0x2c);
		hi704_write_reg(client,0x16, 0x24);
		hi704_write_reg(client,0x17, 0x19);
		hi704_write_reg(client,0x18, 0x90);
		hi704_write_reg(client,0x19, 0x1a);
		hi704_write_reg(client,0x1a, 0xaa);
		hi704_write_reg(client,0x03, 0x00);
		hi704_write_reg(client,0x01, 0xf0);
	}
	else
	{
//		hi704_init_setting(client);
//		hi704_set_preview_mode(client);

		hi704_write_reg(client,0x03, 0x00);//Page 0
		hi704_write_reg(client,0x01, 0xf1);
		hi704_write_reg(client,0x12, 0x00);
		hi704_write_reg(client,0x20, 0x00);
		hi704_write_reg(client,0x21, 0x04);
		hi704_write_reg(client,0x22, 0x00);
		hi704_write_reg(client,0x23, 0x04);
		hi704_write_reg(client,0x24, 0x01);
		hi704_write_reg(client,0x25, 0xe0);
		hi704_write_reg(client,0x26, 0x02);
		hi704_write_reg(client,0x27, 0x80);
		hi704_write_reg(client,0x03, 0x18);
		hi704_write_reg(client,0x10, 0x00);
		hi704_write_reg(client,0x03, 0x00);
		hi704_write_reg(client,0x01, 0xf0);

//		hi704_write_reg(client,0x03, 0x00);
		hi704_write_reg(client,0x11, 0x90);
		
	}
	return 0;
}

int hi704_capture_set(struct cim_sensor *sensor_info)
{

	struct hi704_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"capture\n");
#endif
	return 0;
}

void hi704_set_ab_50hz(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x20);
	hi704_write_reg(client,0x10, 0x9c);
}

void hi704_set_ab_60hz(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x20);
	hi704_write_reg(client,0x10, 0x8c);
}

int hi704_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct hi704_sensor *s;
	struct i2c_client * client;
	unsigned char *str_antibanding;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi704_set_antibanding");
#endif
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			hi704_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
		case ANTIBANDING_50HZ :
			hi704_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_50HZ";
			break;
		case ANTIBANDING_60HZ :
			hi704_set_ab_60hz(client);
			str_antibanding = "ANTIBANDING_60HZ";
			break;
		case ANTIBANDING_OFF :
			str_antibanding = "ANTIBANDING_OFF";
			break;
		default:
			hi704_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
	}
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev, "set antibanding to %s\n", str_antibanding);
#endif
	return 0;
}

void hi704_set_effect_normal(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x10);
	hi704_write_reg(client,0x11, 0x03);
	hi704_write_reg(client,0x12, 0x30);
	hi704_write_reg(client,0x13, 0x00);
	hi704_write_reg(client,0x44, 0x80);
	hi704_write_reg(client,0x45, 0x80);
	hi704_write_reg(client,0x47, 0x7f);
	hi704_write_reg(client,0x03, 0x13);
	hi704_write_reg(client,0x20, 0x06);
	hi704_write_reg(client,0x21, 0x04);
	hi704_write_reg(client,0xff, 0xff);
}

void hi704_set_effect_grayscale(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x10);
	hi704_write_reg(client,0x11, 0x03);
	hi704_write_reg(client,0x12, 0x23);
	hi704_write_reg(client,0x13, 0x00);
	hi704_write_reg(client,0x44, 0x80);
	hi704_write_reg(client,0x45, 0x80);
	hi704_write_reg(client,0x47, 0x7f);
	hi704_write_reg(client,0x03, 0x13);
	hi704_write_reg(client,0x20, 0x07);
	hi704_write_reg(client,0x21, 0x03);
	hi704_write_reg(client,0xff, 0xff);
}

void hi704_set_effect_sepia(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x10);
	hi704_write_reg(client,0x11, 0x03);
	hi704_write_reg(client,0x12, 0x23);
	hi704_write_reg(client,0x13, 0x00);
	hi704_write_reg(client,0x44, 0x70);
	hi704_write_reg(client,0x45, 0x98);
	hi704_write_reg(client,0x47, 0x7f);
	hi704_write_reg(client,0x03, 0x13);
	hi704_write_reg(client,0x20, 0x07);
	hi704_write_reg(client,0x21, 0x03);
	hi704_write_reg(client,0xff, 0xff);
}

void hi704_set_effect_colorinv(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x10);
	hi704_write_reg(client,0x11, 0x03);
	hi704_write_reg(client,0x12, 0x28);
	hi704_write_reg(client,0x13, 0x00);
	hi704_write_reg(client,0x44, 0x80);
	hi704_write_reg(client,0x45, 0x80);
	hi704_write_reg(client,0x03, 0x13);
	hi704_write_reg(client,0x47, 0x7f);
	hi704_write_reg(client,0x20, 0x07);
	hi704_write_reg(client,0x21, 0x03);
	hi704_write_reg(client,0xff, 0xff);
}

void hi704_set_effect_sepiagreen(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x10);
	hi704_write_reg(client,0x11, 0x03);
	hi704_write_reg(client,0x12, 0x23);
	hi704_write_reg(client,0x13, 0x00);
	hi704_write_reg(client,0x44, 0x60);
	hi704_write_reg(client,0x45, 0x60);
	hi704_write_reg(client,0x47, 0x7f);
	hi704_write_reg(client,0x03, 0x13);
	hi704_write_reg(client,0x20, 0x07);
	hi704_write_reg(client,0x21, 0x03);
	hi704_write_reg(client,0xff, 0xff);

}

void hi704_set_effect_sepiablue(struct i2c_client *client)
{
	hi704_write_reg(client,0x03, 0x10);
	hi704_write_reg(client,0x11, 0x03);
	hi704_write_reg(client,0x12, 0x23);
	hi704_write_reg(client,0x13, 0x00);
	hi704_write_reg(client,0x44, 0xb0);
	hi704_write_reg(client,0x45, 0x40);
	hi704_write_reg(client,0x47, 0x7f);
	hi704_write_reg(client,0x03, 0x13);
	hi704_write_reg(client,0x20, 0x07);
	hi704_write_reg(client,0x21, 0x03);
	hi704_write_reg(client,0xff, 0xff);
}

int hi704_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct hi704_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;

	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi704_set_effect");
#endif
	switch(arg)
	{
		case EFFECT_NONE:
			hi704_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			hi704_set_effect_grayscale(client);
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			hi704_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			hi704_set_effect_sepia(client);
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
			hi704_set_effect_sepiagreen(client);
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
			hi704_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"set effect to %s\n", str_effect);
#endif
	return 0;
}

void hi704_set_wb_auto(struct i2c_client *client)
{
	hi704_write_reg(client,0x03,0x22);  // AUTO 3000K~7000K                        
	hi704_write_reg(client,0x10,0x7b);                 
	hi704_write_reg(client,0x83,0x62);	        
	hi704_write_reg(client,0x84,0x20);	       
	hi704_write_reg(client,0x85,0x60);	       
	hi704_write_reg(client,0x86,0x30);              
	hi704_write_reg(client,0x03,0x22);          
	hi704_write_reg(client,0x10,0xfb);        
}

void hi704_set_wb_cloud(struct i2c_client *client)
{
	// NOON130PC20_reg_WB_cloudy
	hi704_write_reg(client,0x03,0x22);   //7000K                                     
	hi704_write_reg(client,0x10,0x7b);                                            
	hi704_write_reg(client,0x80,0x50);                                               
	hi704_write_reg(client,0x81,0x20);	                                             
	hi704_write_reg(client,0x82,0x24);                                                       
	hi704_write_reg(client,0x83,0x6d);		                                     
	hi704_write_reg(client,0x84,0x65);		                                         
	hi704_write_reg(client,0x85,0x24);                                                       
	hi704_write_reg(client,0x86,0x1c);                                               
	//hi704_write_reg(client,0x03,0x22);                                                
	//hi704_write_reg(client,0x10,0xea);                                                 
}

void hi704_set_wb_daylight(struct i2c_client *client)
{
	// NOON130PC20_reg_WB_daylight  °×Ìì 
	hi704_write_reg(client,0x03,0x22);  //6500K                                     
	hi704_write_reg(client,0x10,0x7b);                                                  
	hi704_write_reg(client,0x80,0x40);                                                  
	hi704_write_reg(client,0x81,0x20);		                                     
	hi704_write_reg(client,0x82,0x28);                                                    
	hi704_write_reg(client,0x83,0x45);    	                                             
	hi704_write_reg(client,0x84,0x35);		                                      
	hi704_write_reg(client,0x85,0x2d);	                                                  
	hi704_write_reg(client,0x86,0x1c);                                                 
}

void hi704_set_wb_incandescence(struct i2c_client *client)
{
	hi704_write_reg(client,0x03,0x22);  //2800K~3000K                                     
	hi704_write_reg(client,0x10,0x7b);                                                  
	hi704_write_reg(client,0x80,0x25);                                                 
	hi704_write_reg(client,0x81,0x20);                                                       
	hi704_write_reg(client,0x82,0x44);		                                       
	hi704_write_reg(client,0x83,0x24);		                                     
	hi704_write_reg(client,0x84,0x1e);                                                       
	hi704_write_reg(client,0x85,0x50);		                                     
	hi704_write_reg(client,0x86,0x45);       
}

void hi704_set_wb_fluorescent(struct i2c_client *client)
{
	hi704_write_reg(client,0x03,0x22);  //4200K~5000K                                     
	hi704_write_reg(client,0x10,0x7b);                                                   
	hi704_write_reg(client,0x80,0x35);                                                  
	hi704_write_reg(client,0x81,0x20);		                                          
	hi704_write_reg(client,0x82,0x32);                                                    
	hi704_write_reg(client,0x83,0x3c);		                                     
	hi704_write_reg(client,0x84,0x2c);		                                          
	hi704_write_reg(client,0x85,0x45);                                                       
	hi704_write_reg(client,0x86,0x35);                     
}

void hi704_set_wb_tungsten(struct i2c_client *client)
{
	hi704_write_reg(client,0x03,0x22);  //4000K                                   
	hi704_write_reg(client,0x10,0x6a);                                                   
	hi704_write_reg(client,0x80,0x33);                                                  
	hi704_write_reg(client,0x81,0x20);		                                        
	hi704_write_reg(client,0x82,0x3d);                                                       
	hi704_write_reg(client,0x83,0x2e);		                                          
	hi704_write_reg(client,0x84,0x24);		                                      
	hi704_write_reg(client,0x85,0x43);	                                                  
	hi704_write_reg(client,0x86,0x3d);                                                    
}

int hi704_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct hi704_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct hi704_sensor, cs);
	client = s->client;
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi704_set_balance");
#endif
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			hi704_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			hi704_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			hi704_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			hi704_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			hi704_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			hi704_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
#ifdef HI704_SET_KERNEL_PRINT
	dev_info(&client->dev,"set balance to %s\n", str_balance);
#endif
	return 0;
}
