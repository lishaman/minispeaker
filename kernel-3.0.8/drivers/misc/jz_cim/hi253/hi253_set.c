#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "hi253_camera.h"

//#define HI253_SET_KERNEL_PRINT

int hi253_init(struct cim_sensor *sensor_info)
{
	struct hi253_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi253 init\n");
#endif

	/////// Start Sleep ///////
	hi253_write_reg(client,0x01, 0xf9); //sleep on
	hi253_write_reg(client,0x08, 0x0f); //Hi-Z on
	hi253_write_reg(client,0x01, 0xf8); //sleep off

	hi253_write_reg(client,0x03, 0x00); // Dummy 750us START
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00); // Dummy 750us END

	hi253_write_reg(client,0x0e, 0x03); //PLL On
	hi253_write_reg(client,0x0e, 0x73); //PLLx2

	hi253_write_reg(client,0x03, 0x00); // Dummy 750us START
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00); // Dummy 750us END

	hi253_write_reg(client,0x0e, 0x00); //PLL off
	hi253_write_reg(client,0x01, 0xf1); //sleep on
	hi253_write_reg(client,0x08, 0x00); //Hi-Z off

	hi253_write_reg(client,0x01, 0xf3);
	hi253_write_reg(client,0x01, 0xf1);

	// PAGE 20
	hi253_write_reg(client,0x03, 0x20); //page 20
	hi253_write_reg(client,0x10, 0x1c); //ae off

	// PAGE 22
	hi253_write_reg(client,0x03, 0x22); //page 22
	hi253_write_reg(client,0x10, 0x69); //awb off

	//Initial Start
	/////// PAGE 0 START ///////
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x10, 0x11); // Sub1/2_Preview2 Mode_H binning
	hi253_write_reg(client,0x11, 0x90);
	hi253_write_reg(client,0x12, 0x00);

	hi253_write_reg(client,0x0b, 0xaa); // ESD Check Register
	hi253_write_reg(client,0x0c, 0xaa); // ESD Check Register
	hi253_write_reg(client,0x0d, 0xaa); // ESD Check Register

	hi253_write_reg(client,0x20, 0x00); // Windowing start point Y
	hi253_write_reg(client,0x21, 0x04);
	hi253_write_reg(client,0x22, 0x00); // Windowing start point X
	hi253_write_reg(client,0x23, 0x03);//07

	hi253_write_reg(client,0x24, 0x04);
	hi253_write_reg(client,0x25, 0xb0);
	hi253_write_reg(client,0x26, 0x06);
	hi253_write_reg(client,0x27, 0x40); // WINROW END

	hi253_write_reg(client,0x40, 0x01); //Hblank 408
	hi253_write_reg(client,0x41, 0x68); //98
	hi253_write_reg(client,0x42, 0x00); //Vblank 20
	hi253_write_reg(client,0x43, 0x14);

	hi253_write_reg(client,0x45, 0x04);
	hi253_write_reg(client,0x46, 0x18);
	hi253_write_reg(client,0x47, 0xd8);

	//BLC
	hi253_write_reg(client,0x80, 0x2e);
	hi253_write_reg(client,0x81, 0x7e);
	hi253_write_reg(client,0x82, 0x90);
	hi253_write_reg(client,0x83, 0x00);
	hi253_write_reg(client,0x84, 0x0c);
	hi253_write_reg(client,0x85, 0x00);
	hi253_write_reg(client,0x90, 0x0a); //BLC_TIME_TH_ON
	hi253_write_reg(client,0x91, 0x0a); //BLC_TIME_TH_OFF
	hi253_write_reg(client,0x92, 0xd8); //BLC_AG_TH_ON
	hi253_write_reg(client,0x93, 0xd0); //BLC_AG_TH_OFF
	hi253_write_reg(client,0x94, 0x75);
	hi253_write_reg(client,0x95, 0x70);
	hi253_write_reg(client,0x96, 0xdc);
	hi253_write_reg(client,0x97, 0xfe);
	hi253_write_reg(client,0x98, 0x38);

	//OutDoor  BLC
	hi253_write_reg(client,0x99, 0x43);
	hi253_write_reg(client,0x9a, 0x43);
	hi253_write_reg(client,0x9b, 0x43);
	hi253_write_reg(client,0x9c, 0x43);

	//Dark BLC
	hi253_write_reg(client,0xa0, 0x00);
	hi253_write_reg(client,0xa2, 0x00);
	hi253_write_reg(client,0xa4, 0x00);
	hi253_write_reg(client,0xa6, 0x00);

	//Normal BLC
	hi253_write_reg(client,0xa8, 0x43);
	hi253_write_reg(client,0xaa, 0x43);
	hi253_write_reg(client,0xac, 0x43);
	hi253_write_reg(client,0xae, 0x43);

	/////// PAGE 2 START ///////
	hi253_write_reg(client,0x03, 0x02);
	hi253_write_reg(client,0x12, 0x03);
	hi253_write_reg(client,0x13, 0x03);
	hi253_write_reg(client,0x16, 0x00);
	hi253_write_reg(client,0x17, 0x8C);
	hi253_write_reg(client,0x18, 0x4c); //Double_AG off
	hi253_write_reg(client,0x19, 0x00);
	hi253_write_reg(client,0x1a, 0x39); //ADC400->560
	hi253_write_reg(client,0x1c, 0x09);
	hi253_write_reg(client,0x1d, 0x40);
	hi253_write_reg(client,0x1e, 0x30);
	hi253_write_reg(client,0x1f, 0x10);

	hi253_write_reg(client,0x20, 0x77);
	hi253_write_reg(client,0x21, 0xde);
	hi253_write_reg(client,0x22, 0xa7);
	hi253_write_reg(client,0x23, 0x30); //CLAMP
	hi253_write_reg(client,0x27, 0x3c);
	hi253_write_reg(client,0x2b, 0x80);
	hi253_write_reg(client,0x2e, 0x11);
	hi253_write_reg(client,0x2f, 0xa1);
	hi253_write_reg(client,0x30, 0x05); //For Hi-253 never no change 0x05

	hi253_write_reg(client,0x50, 0x20);
	hi253_write_reg(client,0x52, 0x01);
	hi253_write_reg(client,0x55, 0x1c);
	hi253_write_reg(client,0x56, 0x11);
	hi253_write_reg(client,0x5d, 0xa2);
	hi253_write_reg(client,0x5e, 0x5a);

	hi253_write_reg(client,0x60, 0x87);
	hi253_write_reg(client,0x61, 0x99);
	hi253_write_reg(client,0x62, 0x88);
	hi253_write_reg(client,0x63, 0x97);
	hi253_write_reg(client,0x64, 0x88);
	hi253_write_reg(client,0x65, 0x97);

	hi253_write_reg(client,0x67, 0x0c);
	hi253_write_reg(client,0x68, 0x0c);
	hi253_write_reg(client,0x69, 0x0c);

	hi253_write_reg(client,0x72, 0x89);
	hi253_write_reg(client,0x73, 0x96);
	hi253_write_reg(client,0x74, 0x89);
	hi253_write_reg(client,0x75, 0x96);
	hi253_write_reg(client,0x76, 0x89);
	hi253_write_reg(client,0x77, 0x96);

	hi253_write_reg(client,0x7c, 0x85);
	hi253_write_reg(client,0x7d, 0xaf);
	hi253_write_reg(client,0x80, 0x01);
	hi253_write_reg(client,0x81, 0x7f);
	hi253_write_reg(client,0x82, 0x13);

	hi253_write_reg(client,0x83, 0x24);
	hi253_write_reg(client,0x84, 0x7d);
	hi253_write_reg(client,0x85, 0x81);
	hi253_write_reg(client,0x86, 0x7d);
	hi253_write_reg(client,0x87, 0x81);

	hi253_write_reg(client,0x92, 0x48);
	hi253_write_reg(client,0x93, 0x54);
	hi253_write_reg(client,0x94, 0x7d);
	hi253_write_reg(client,0x95, 0x81);
	hi253_write_reg(client,0x96, 0x7d);
	hi253_write_reg(client,0x97, 0x81);

	hi253_write_reg(client,0xa0, 0x02);
	hi253_write_reg(client,0xa1, 0x7b);
	hi253_write_reg(client,0xa2, 0x02);
	hi253_write_reg(client,0xa3, 0x7b);
	hi253_write_reg(client,0xa4, 0x7b);
	hi253_write_reg(client,0xa5, 0x02);
	hi253_write_reg(client,0xa6, 0x7b);
	hi253_write_reg(client,0xa7, 0x02);

	hi253_write_reg(client,0xa8, 0x85);
	hi253_write_reg(client,0xa9, 0x8c);
	hi253_write_reg(client,0xaa, 0x85);
	hi253_write_reg(client,0xab, 0x8c);
	hi253_write_reg(client,0xac, 0x10);
	hi253_write_reg(client,0xad, 0x16);
	hi253_write_reg(client,0xae, 0x10);
	hi253_write_reg(client,0xaf, 0x16);

	hi253_write_reg(client,0xb0, 0x99);
	hi253_write_reg(client,0xb1, 0xa3);
	hi253_write_reg(client,0xb2, 0xa4);
	hi253_write_reg(client,0xb3, 0xae);
	hi253_write_reg(client,0xb4, 0x9b);
	hi253_write_reg(client,0xb5, 0xa2);
	hi253_write_reg(client,0xb6, 0xa6);
	hi253_write_reg(client,0xb7, 0xac);
	hi253_write_reg(client,0xb8, 0x9b);
	hi253_write_reg(client,0xb9, 0x9f);
	hi253_write_reg(client,0xba, 0xa6);
	hi253_write_reg(client,0xbb, 0xaa);
	hi253_write_reg(client,0xbc, 0x9b);
	hi253_write_reg(client,0xbd, 0x9f);
	hi253_write_reg(client,0xbe, 0xa6);
	hi253_write_reg(client,0xbf, 0xaa);

	hi253_write_reg(client,0xc4, 0x2c);
	hi253_write_reg(client,0xc5, 0x43);
	hi253_write_reg(client,0xc6, 0x63);
	hi253_write_reg(client,0xc7, 0x79);

	hi253_write_reg(client,0xc8, 0x2d);
	hi253_write_reg(client,0xc9, 0x42);
	hi253_write_reg(client,0xca, 0x2d);
	hi253_write_reg(client,0xcb, 0x42);
	hi253_write_reg(client,0xcc, 0x64);
	hi253_write_reg(client,0xcd, 0x78);
	hi253_write_reg(client,0xce, 0x64);
	hi253_write_reg(client,0xcf, 0x78);

	hi253_write_reg(client,0xd0, 0x0a);
	hi253_write_reg(client,0xd1, 0x09);
	hi253_write_reg(client,0xd4, 0x0a); //DCDC_TIME_TH_ON
	hi253_write_reg(client,0xd5, 0x0a); //DCDC_TIME_TH_OFF
	hi253_write_reg(client,0xd6, 0xd8); //DCDC_AG_TH_ON
	hi253_write_reg(client,0xd7, 0xd0); //DCDC_AG_TH_OFF
	hi253_write_reg(client,0xe0, 0xc4);
	hi253_write_reg(client,0xe1, 0xc4);
	hi253_write_reg(client,0xe2, 0xc4);
	hi253_write_reg(client,0xe3, 0xc4);
	hi253_write_reg(client,0xe4, 0x00);
	hi253_write_reg(client,0xe8, 0x80);
	hi253_write_reg(client,0xe9, 0x40);
	hi253_write_reg(client,0xea, 0x7f);

	/////// PAGE 3 ///////
	hi253_write_reg(client,0x03, 0x03);
	hi253_write_reg(client,0x10, 0x10);

	/////// PAGE 10 START ///////
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x10, 0x03); // CrYCbY // For Demoset 0x03
	hi253_write_reg(client,0x12, 0x30);

	/* disable contrast */
	hi253_write_reg(client,0x13, 0x00);

	hi253_write_reg(client,0x20, 0x00);
	hi253_write_reg(client,0x30, 0x00);
	hi253_write_reg(client,0x31, 0x00);
	hi253_write_reg(client,0x32, 0x00);
	hi253_write_reg(client,0x33, 0x00);

	hi253_write_reg(client,0x34, 0x30);
	hi253_write_reg(client,0x35, 0x00);
	hi253_write_reg(client,0x36, 0x00);
	hi253_write_reg(client,0x38, 0x00);
	hi253_write_reg(client,0x3e, 0x58);
	hi253_write_reg(client,0x3f, 0x00);

	/* adjust brightness */
	hi253_write_reg(client,0x40, 0xa0); // YOFS
	hi253_write_reg(client,0x41, 0x00); // DYOFS

	hi253_write_reg(client,0x60, 0x67);
	hi253_write_reg(client,0x61, 0x7c); //7e //8e //88 //80
	hi253_write_reg(client,0x62, 0x7c); //7e //8e //88 //80
	hi253_write_reg(client,0x63, 0x50); //Double_AG 50->30
	hi253_write_reg(client,0x64, 0x41);

	hi253_write_reg(client,0x66, 0x42);
	hi253_write_reg(client,0x67, 0x20);

	hi253_write_reg(client,0x6a, 0x80); //8a
	hi253_write_reg(client,0x6b, 0x84); //74
	hi253_write_reg(client,0x6c, 0x80); //7e //7a
	hi253_write_reg(client,0x6d, 0x80); //8e

	//Don't touch//////////////////////////
	//hi253_write_reg(client,0x72, 0x84);
	//hi253_write_reg(client,0x76, 0x19);
	//hi253_write_reg(client,0x73, 0x70);
	//hi253_write_reg(client,0x74, 0x68);
	//hi253_write_reg(client,0x75, 0x60); // white protection ON
	//hi253_write_reg(client,0x77, 0x0e); //08 //0a
	//hi253_write_reg(client,0x78, 0x2a); //20
	//hi253_write_reg(client,0x79, 0x08);
	////////////////////////////////////////

	/////// PAGE 11 START ///////
	hi253_write_reg(client,0x03, 0x11);
	hi253_write_reg(client,0x10, 0x7f);
	hi253_write_reg(client,0x11, 0x40);
	hi253_write_reg(client,0x12, 0x0a); // Blue Max-Filter Delete
	hi253_write_reg(client,0x13, 0xbb);

	hi253_write_reg(client,0x26, 0x31); // Double_AG 31->20
	hi253_write_reg(client,0x27, 0x34); // Double_AG 34->22
	hi253_write_reg(client,0x28, 0x0f);
	hi253_write_reg(client,0x29, 0x10);
	hi253_write_reg(client,0x2b, 0x30);
	hi253_write_reg(client,0x2c, 0x32);

	//Out2 D-LPF th
	hi253_write_reg(client,0x30, 0x70);
	hi253_write_reg(client,0x31, 0x10);
	hi253_write_reg(client,0x32, 0x58);
	hi253_write_reg(client,0x33, 0x09);
	hi253_write_reg(client,0x34, 0x06);
	hi253_write_reg(client,0x35, 0x03);

	//Out1 D-LPF th
	hi253_write_reg(client,0x36, 0x70);
	hi253_write_reg(client,0x37, 0x18);
	hi253_write_reg(client,0x38, 0x58);
	hi253_write_reg(client,0x39, 0x09);
	hi253_write_reg(client,0x3a, 0x06);
	hi253_write_reg(client,0x3b, 0x03);

	//Indoor D-LPF th
	hi253_write_reg(client,0x3c, 0x80);
	hi253_write_reg(client,0x3d, 0x18);
	hi253_write_reg(client,0x3e, 0xa0); //80
	hi253_write_reg(client,0x3f, 0x0c);
	hi253_write_reg(client,0x40, 0x09);
	hi253_write_reg(client,0x41, 0x06);

	hi253_write_reg(client,0x42, 0x80);
	hi253_write_reg(client,0x43, 0x18);
	hi253_write_reg(client,0x44, 0xa0); //80
	hi253_write_reg(client,0x45, 0x12);
	hi253_write_reg(client,0x46, 0x10);
	hi253_write_reg(client,0x47, 0x10);

	hi253_write_reg(client,0x48, 0x90);
	hi253_write_reg(client,0x49, 0x40);
	hi253_write_reg(client,0x4a, 0x80);
	hi253_write_reg(client,0x4b, 0x13);
	hi253_write_reg(client,0x4c, 0x10);
	hi253_write_reg(client,0x4d, 0x11);

	hi253_write_reg(client,0x4e, 0x80);
	hi253_write_reg(client,0x4f, 0x30);
	hi253_write_reg(client,0x50, 0x80);
	hi253_write_reg(client,0x51, 0x13);
	hi253_write_reg(client,0x52, 0x10);
	hi253_write_reg(client,0x53, 0x13);

	hi253_write_reg(client,0x54, 0x11);
	hi253_write_reg(client,0x55, 0x17);
	hi253_write_reg(client,0x56, 0x20);
	hi253_write_reg(client,0x57, 0x01);
	hi253_write_reg(client,0x58, 0x00);
	hi253_write_reg(client,0x59, 0x00);

	hi253_write_reg(client,0x5a, 0x1f); //18
	hi253_write_reg(client,0x5b, 0x00);
	hi253_write_reg(client,0x5c, 0x00);

	hi253_write_reg(client,0x60, 0x3f);
	hi253_write_reg(client,0x62, 0x60);
	hi253_write_reg(client,0x70, 0x06);

	/////// PAGE 12 START ///////
	hi253_write_reg(client,0x03, 0x12);
	hi253_write_reg(client,0x20, 0x0f);
	hi253_write_reg(client,0x21, 0x0f);

	hi253_write_reg(client,0x25, 0x00); //0x30

	hi253_write_reg(client,0x28, 0x00);
	hi253_write_reg(client,0x29, 0x00);
	hi253_write_reg(client,0x2a, 0x00);

	hi253_write_reg(client,0x30, 0x50);
	hi253_write_reg(client,0x31, 0x18);
	hi253_write_reg(client,0x32, 0x32);
	hi253_write_reg(client,0x33, 0x40);
	hi253_write_reg(client,0x34, 0x50);
	hi253_write_reg(client,0x35, 0x70);
	hi253_write_reg(client,0x36, 0xa0);

	//Out2 th
	hi253_write_reg(client,0x40, 0xa0);
	hi253_write_reg(client,0x41, 0x40);
	hi253_write_reg(client,0x42, 0xa0);
	hi253_write_reg(client,0x43, 0x90);
	hi253_write_reg(client,0x44, 0x90);
	hi253_write_reg(client,0x45, 0x80);

	//Out1 th
	hi253_write_reg(client,0x46, 0xb0);
	hi253_write_reg(client,0x47, 0x55);
	hi253_write_reg(client,0x48, 0xa0);
	hi253_write_reg(client,0x49, 0x90);
	hi253_write_reg(client,0x4a, 0x90);
	hi253_write_reg(client,0x4b, 0x80);

	//Indoor th
	hi253_write_reg(client,0x4c, 0xb0);
	hi253_write_reg(client,0x4d, 0x40);
	hi253_write_reg(client,0x4e, 0x90);
	hi253_write_reg(client,0x4f, 0x90);
	hi253_write_reg(client,0x50, 0xa0);
	hi253_write_reg(client,0x51, 0x80);

	//Dark1 th
	hi253_write_reg(client,0x52, 0xb0);
	hi253_write_reg(client,0x53, 0x60);
	hi253_write_reg(client,0x54, 0xc0);
	hi253_write_reg(client,0x55, 0xc0);
	hi253_write_reg(client,0x56, 0xc0);
	hi253_write_reg(client,0x57, 0x80);

	//Dark2 th
	hi253_write_reg(client,0x58, 0x90);
	hi253_write_reg(client,0x59, 0x40);
	hi253_write_reg(client,0x5a, 0xd0);
	hi253_write_reg(client,0x5b, 0xd0);
	hi253_write_reg(client,0x5c, 0xe0);
	hi253_write_reg(client,0x5d, 0x80);

	//Dark3 th
	hi253_write_reg(client,0x5e, 0x88);
	hi253_write_reg(client,0x5f, 0x40);
	hi253_write_reg(client,0x60, 0xe0);
	hi253_write_reg(client,0x61, 0xe0);
	hi253_write_reg(client,0x62, 0xe0);
	hi253_write_reg(client,0x63, 0x80);

	hi253_write_reg(client,0x70, 0x15);
	hi253_write_reg(client,0x71, 0x01); //Don't Touch register

	hi253_write_reg(client,0x72, 0x18);
	hi253_write_reg(client,0x73, 0x01); //Don't Touch register

	hi253_write_reg(client,0x74, 0x25);
	hi253_write_reg(client,0x75, 0x15);

	hi253_write_reg(client,0x80, 0x20);
	hi253_write_reg(client,0x81, 0x40);
	hi253_write_reg(client,0x82, 0x65);
	hi253_write_reg(client,0x85, 0x1a);
	hi253_write_reg(client,0x88, 0x00);
	hi253_write_reg(client,0x89, 0x00);
	hi253_write_reg(client,0x90, 0x5d); //For Preview

	//Dont Touch register
	hi253_write_reg(client,0xD0, 0x0c);
	hi253_write_reg(client,0xD1, 0x80);
	hi253_write_reg(client,0xD2, 0x67);
	hi253_write_reg(client,0xD3, 0x00);
	hi253_write_reg(client,0xD4, 0x00);
	hi253_write_reg(client,0xD5, 0x02);
	hi253_write_reg(client,0xD6, 0xff);
	hi253_write_reg(client,0xD7, 0x18);
	//End
	hi253_write_reg(client,0x3b, 0x06);
	hi253_write_reg(client,0x3c, 0x06);

	hi253_write_reg(client,0xc5, 0x00);//55->48
	hi253_write_reg(client,0xc6, 0x00);//48->40

	/////// PAGE 13 START ///////
	hi253_write_reg(client,0x03, 0x13);
	//Edge
	hi253_write_reg(client,0x10, 0xcb);
	hi253_write_reg(client,0x11, 0x7b);
	hi253_write_reg(client,0x12, 0x07);
	hi253_write_reg(client,0x14, 0x00);

	hi253_write_reg(client,0x20, 0x15);
	hi253_write_reg(client,0x21, 0x13);
	hi253_write_reg(client,0x22, 0x33);
	hi253_write_reg(client,0x23, 0x05);
	hi253_write_reg(client,0x24, 0x09);

	hi253_write_reg(client,0x25, 0x0a);

	hi253_write_reg(client,0x26, 0x18);
	hi253_write_reg(client,0x27, 0x30);
	hi253_write_reg(client,0x29, 0x12);
	hi253_write_reg(client,0x2a, 0x50);

	//Low clip th
	hi253_write_reg(client,0x2b, 0x02);
	hi253_write_reg(client,0x2c, 0x02);
	hi253_write_reg(client,0x25, 0x06);
	hi253_write_reg(client,0x2d, 0x0c);
	hi253_write_reg(client,0x2e, 0x12);
	hi253_write_reg(client,0x2f, 0x12);

	//Out2 Edge
	hi253_write_reg(client,0x50, 0x10);
	hi253_write_reg(client,0x51, 0x14);
	hi253_write_reg(client,0x52, 0x12);
	hi253_write_reg(client,0x53, 0x0c);
	hi253_write_reg(client,0x54, 0x0f);
	hi253_write_reg(client,0x55, 0x0c);

	//Out1 Edge
	hi253_write_reg(client,0x56, 0x10);
	hi253_write_reg(client,0x57, 0x13);
	hi253_write_reg(client,0x58, 0x12);
	hi253_write_reg(client,0x59, 0x0c);
	hi253_write_reg(client,0x5a, 0x0f);
	hi253_write_reg(client,0x5b, 0x0c);

	//Indoor Edge
	hi253_write_reg(client,0x5c, 0x0a);
	hi253_write_reg(client,0x5d, 0x0b);
	hi253_write_reg(client,0x5e, 0x0a);
	hi253_write_reg(client,0x5f, 0x08);
	hi253_write_reg(client,0x60, 0x09);
	hi253_write_reg(client,0x61, 0x08);

	//Dark1 Edge
	hi253_write_reg(client,0x62, 0x08);
	hi253_write_reg(client,0x63, 0x08);
	hi253_write_reg(client,0x64, 0x08);
	hi253_write_reg(client,0x65, 0x06);
	hi253_write_reg(client,0x66, 0x06);
	hi253_write_reg(client,0x67, 0x06);

	//Dark2 Edge
	hi253_write_reg(client,0x68, 0x07);
	hi253_write_reg(client,0x69, 0x07);
	hi253_write_reg(client,0x6a, 0x07);
	hi253_write_reg(client,0x6b, 0x05);
	hi253_write_reg(client,0x6c, 0x05);
	hi253_write_reg(client,0x6d, 0x05);

	//Dark3 Edge
	hi253_write_reg(client,0x6e, 0x07);
	hi253_write_reg(client,0x6f, 0x07);
	hi253_write_reg(client,0x70, 0x07);
	hi253_write_reg(client,0x71, 0x05);
	hi253_write_reg(client,0x72, 0x05);
	hi253_write_reg(client,0x73, 0x05);

	//2DY
	hi253_write_reg(client,0x80, 0xfd);
	hi253_write_reg(client,0x81, 0x1f);
	hi253_write_reg(client,0x82, 0x05);
	hi253_write_reg(client,0x83, 0x31);

	hi253_write_reg(client,0x90, 0x05);
	hi253_write_reg(client,0x91, 0x05);
	hi253_write_reg(client,0x92, 0x33);
	hi253_write_reg(client,0x93, 0x30);
	hi253_write_reg(client,0x94, 0x03);
	hi253_write_reg(client,0x95, 0x14);
	hi253_write_reg(client,0x97, 0x20);
	hi253_write_reg(client,0x99, 0x20);

	hi253_write_reg(client,0xa0, 0x01);
	hi253_write_reg(client,0xa1, 0x02);
	hi253_write_reg(client,0xa2, 0x01);
	hi253_write_reg(client,0xa3, 0x02);
	hi253_write_reg(client,0xa4, 0x05);
	hi253_write_reg(client,0xa5, 0x05);
	hi253_write_reg(client,0xa6, 0x07);
	hi253_write_reg(client,0xa7, 0x08);
	hi253_write_reg(client,0xa8, 0x07);
	hi253_write_reg(client,0xa9, 0x08);
	hi253_write_reg(client,0xaa, 0x07);
	hi253_write_reg(client,0xab, 0x08);

	//Out2
	hi253_write_reg(client,0xb0, 0x22);
	hi253_write_reg(client,0xb1, 0x2a);
	hi253_write_reg(client,0xb2, 0x28);
	hi253_write_reg(client,0xb3, 0x22);
	hi253_write_reg(client,0xb4, 0x2a);
	hi253_write_reg(client,0xb5, 0x28);

	//Out1
	hi253_write_reg(client,0xb6, 0x22);
	hi253_write_reg(client,0xb7, 0x2a);
	hi253_write_reg(client,0xb8, 0x28);
	hi253_write_reg(client,0xb9, 0x22);
	hi253_write_reg(client,0xba, 0x2a);
	hi253_write_reg(client,0xbb, 0x28);

	//Indoor
	hi253_write_reg(client,0xbc, 0x25);
	hi253_write_reg(client,0xbd, 0x2a);
	hi253_write_reg(client,0xbe, 0x27);
	hi253_write_reg(client,0xbf, 0x25);
	hi253_write_reg(client,0xc0, 0x2a);
	hi253_write_reg(client,0xc1, 0x27);

	//Dark1
	hi253_write_reg(client,0xc2, 0x1e);
	hi253_write_reg(client,0xc3, 0x24);
	hi253_write_reg(client,0xc4, 0x20);
	hi253_write_reg(client,0xc5, 0x1e);
	hi253_write_reg(client,0xc6, 0x24);
	hi253_write_reg(client,0xc7, 0x20);

	//Dark2
	hi253_write_reg(client,0xc8, 0x18);
	hi253_write_reg(client,0xc9, 0x20);
	hi253_write_reg(client,0xca, 0x1e);
	hi253_write_reg(client,0xcb, 0x18);
	hi253_write_reg(client,0xcc, 0x20);
	hi253_write_reg(client,0xcd, 0x1e);

	//Dark3
	hi253_write_reg(client,0xce, 0x18);
	hi253_write_reg(client,0xcf, 0x20);
	hi253_write_reg(client,0xd0, 0x1e);
	hi253_write_reg(client,0xd1, 0x18);
	hi253_write_reg(client,0xd2, 0x20);
	hi253_write_reg(client,0xd3, 0x1e);

	/////// PAGE 14 START ///////
	hi253_write_reg(client,0x03, 0x14);
	hi253_write_reg(client,0x10, 0x11);

	hi253_write_reg(client,0x14, 0x80); // GX
	hi253_write_reg(client,0x15, 0x80); // GY
	hi253_write_reg(client,0x16, 0x80); // RX
	hi253_write_reg(client,0x17, 0x80); // RY
	hi253_write_reg(client,0x18, 0x80); // BX
	hi253_write_reg(client,0x19, 0x80); // BY

	hi253_write_reg(client,0x20, 0x60); //X 60 //a0
	hi253_write_reg(client,0x21, 0x80); //Y

	hi253_write_reg(client,0x22, 0x80);
	hi253_write_reg(client,0x23, 0x80);
	hi253_write_reg(client,0x24, 0x80);

	hi253_write_reg(client,0x30, 0xc8);
	hi253_write_reg(client,0x31, 0x2b);
	hi253_write_reg(client,0x32, 0x00);
	hi253_write_reg(client,0x33, 0x00);
	hi253_write_reg(client,0x34, 0x90);

	hi253_write_reg(client,0x40, 0x48); //31
	hi253_write_reg(client,0x50, 0x34); //23 //32
	hi253_write_reg(client,0x60, 0x29); //1a //27
	hi253_write_reg(client,0x70, 0x34); //23 //32

	/////// PAGE 15 START ///////
	hi253_write_reg(client,0x03, 0x15);
	hi253_write_reg(client,0x10, 0x0f);

	//Rstep H 16
	//Rstep L 14
	hi253_write_reg(client,0x14, 0x42); //CMCOFSGH_Day //4c
	hi253_write_reg(client,0x15, 0x32); //CMCOFSGM_CWF //3c
	hi253_write_reg(client,0x16, 0x24); //CMCOFSGL_A //2e
	hi253_write_reg(client,0x17, 0x2f); //CMC SIGN

	//CMC_Default_CWF
	hi253_write_reg(client,0x30, 0x8f);
	hi253_write_reg(client,0x31, 0x59);
	hi253_write_reg(client,0x32, 0x0a);
	hi253_write_reg(client,0x33, 0x15);
	hi253_write_reg(client,0x34, 0x5b);
	hi253_write_reg(client,0x35, 0x06);
	hi253_write_reg(client,0x36, 0x07);
	hi253_write_reg(client,0x37, 0x40);
	hi253_write_reg(client,0x38, 0x87); //86

	//CMC OFS L_A
	hi253_write_reg(client,0x40, 0x92);
	hi253_write_reg(client,0x41, 0x1b);
	hi253_write_reg(client,0x42, 0x89);
	hi253_write_reg(client,0x43, 0x81);
	hi253_write_reg(client,0x44, 0x00);
	hi253_write_reg(client,0x45, 0x01);
	hi253_write_reg(client,0x46, 0x89);
	hi253_write_reg(client,0x47, 0x9e);
	hi253_write_reg(client,0x48, 0x28);

	//hi253_write_reg(client,0x40, 0x93);
	//hi253_write_reg(client,0x41, 0x1c);
	//hi253_write_reg(client,0x42, 0x89);
	//hi253_write_reg(client,0x43, 0x82);
	//hi253_write_reg(client,0x44, 0x01);
	//hi253_write_reg(client,0x45, 0x01);
	//hi253_write_reg(client,0x46, 0x8a);
	//hi253_write_reg(client,0x47, 0x9d);
	//hi253_write_reg(client,0x48, 0x28);

	//CMC POFS H_DAY
	hi253_write_reg(client,0x50, 0x02);
	hi253_write_reg(client,0x51, 0x82);
	hi253_write_reg(client,0x52, 0x00);
	hi253_write_reg(client,0x53, 0x07);
	hi253_write_reg(client,0x54, 0x11);
	hi253_write_reg(client,0x55, 0x98);
	hi253_write_reg(client,0x56, 0x00);
	hi253_write_reg(client,0x57, 0x0b);
	hi253_write_reg(client,0x58, 0x8b);

	hi253_write_reg(client,0x80, 0x03);
	hi253_write_reg(client,0x85, 0x40);
	hi253_write_reg(client,0x87, 0x02);
	hi253_write_reg(client,0x88, 0x00);
	hi253_write_reg(client,0x89, 0x00);
	hi253_write_reg(client,0x8a, 0x00);

	/////// PAGE 16 START ///////
	hi253_write_reg(client,0x03, 0x16);
	hi253_write_reg(client,0x10, 0x31);
	hi253_write_reg(client,0x18, 0x5e);// Double_AG 5e->37
	hi253_write_reg(client,0x19, 0x5d);// Double_AG 5e->36
	hi253_write_reg(client,0x1a, 0x0e);
	hi253_write_reg(client,0x1b, 0x01);
	hi253_write_reg(client,0x1c, 0xdc);
	hi253_write_reg(client,0x1d, 0xfe);

	//GMA Default
	hi253_write_reg(client,0x30, 0x00);
	hi253_write_reg(client,0x31, 0x0a);
	hi253_write_reg(client,0x32, 0x1f);
	hi253_write_reg(client,0x33, 0x33);
	hi253_write_reg(client,0x34, 0x53);
	hi253_write_reg(client,0x35, 0x6c);
	hi253_write_reg(client,0x36, 0x81);
	hi253_write_reg(client,0x37, 0x94);
	hi253_write_reg(client,0x38, 0xa4);
	hi253_write_reg(client,0x39, 0xb3);
	hi253_write_reg(client,0x3a, 0xc0);
	hi253_write_reg(client,0x3b, 0xcb);
	hi253_write_reg(client,0x3c, 0xd5);
	hi253_write_reg(client,0x3d, 0xde);
	hi253_write_reg(client,0x3e, 0xe6);
	hi253_write_reg(client,0x3f, 0xee);
	hi253_write_reg(client,0x40, 0xf5);
	hi253_write_reg(client,0x41, 0xfc);
	hi253_write_reg(client,0x42, 0xff);

	hi253_write_reg(client,0x50, 0x00);
	hi253_write_reg(client,0x51, 0x09);
	hi253_write_reg(client,0x52, 0x1f);
	hi253_write_reg(client,0x53, 0x37);
	hi253_write_reg(client,0x54, 0x5b);
	hi253_write_reg(client,0x55, 0x76);
	hi253_write_reg(client,0x56, 0x8d);
	hi253_write_reg(client,0x57, 0xa1);
	hi253_write_reg(client,0x58, 0xb2);
	hi253_write_reg(client,0x59, 0xbe);
	hi253_write_reg(client,0x5a, 0xc9);
	hi253_write_reg(client,0x5b, 0xd2);
	hi253_write_reg(client,0x5c, 0xdb);
	hi253_write_reg(client,0x5d, 0xe3);
	hi253_write_reg(client,0x5e, 0xeb);
	hi253_write_reg(client,0x5f, 0xf0);
	hi253_write_reg(client,0x60, 0xf5);
	hi253_write_reg(client,0x61, 0xf7);
	hi253_write_reg(client,0x62, 0xf8);

	hi253_write_reg(client,0x70, 0x00);
	hi253_write_reg(client,0x71, 0x08);
	hi253_write_reg(client,0x72, 0x17);
	hi253_write_reg(client,0x73, 0x2f);
	hi253_write_reg(client,0x74, 0x53);
	hi253_write_reg(client,0x75, 0x6c);
	hi253_write_reg(client,0x76, 0x81);
	hi253_write_reg(client,0x77, 0x94);
	hi253_write_reg(client,0x78, 0xa4);
	hi253_write_reg(client,0x79, 0xb3);
	hi253_write_reg(client,0x7a, 0xc0);
	hi253_write_reg(client,0x7b, 0xcb);
	hi253_write_reg(client,0x7c, 0xd5);
	hi253_write_reg(client,0x7d, 0xde);
	hi253_write_reg(client,0x7e, 0xe6);
	hi253_write_reg(client,0x7f, 0xee);
	hi253_write_reg(client,0x80, 0xf4);
	hi253_write_reg(client,0x81, 0xfa);
	hi253_write_reg(client,0x82, 0xff);

	/////// PAGE 17 START ///////
	hi253_write_reg(client,0x03, 0x17);
	hi253_write_reg(client,0x10, 0xf7);

	/////// PAGE 20 START ///////
	hi253_write_reg(client,0x03, 0x20);
	hi253_write_reg(client,0x11, 0x1c);
	hi253_write_reg(client,0x18, 0x30);
	hi253_write_reg(client,0x1a, 0x08);
	hi253_write_reg(client,0x20, 0x01); //05_lowtemp Y Mean off
	hi253_write_reg(client,0x21, 0x30);
	hi253_write_reg(client,0x22, 0x10);
	hi253_write_reg(client,0x23, 0x00);
	hi253_write_reg(client,0x24, 0x00); //Uniform Scene Off

	hi253_write_reg(client,0x28, 0xe7);
	hi253_write_reg(client,0x29, 0x0d); //20100305 ad->0d
	hi253_write_reg(client,0x2a, 0xff);
	hi253_write_reg(client,0x2b, 0x04); //f4->Adaptive off

	hi253_write_reg(client,0x2c, 0xc2);
	hi253_write_reg(client,0x2d, 0xcf);  //fe->AE Speed option
	hi253_write_reg(client,0x2e, 0x33);
	hi253_write_reg(client,0x30, 0x78); //f8
	hi253_write_reg(client,0x32, 0x03);
	hi253_write_reg(client,0x33, 0x2e);
	hi253_write_reg(client,0x34, 0x30);
	hi253_write_reg(client,0x35, 0xd4);
	hi253_write_reg(client,0x36, 0xfe);
	hi253_write_reg(client,0x37, 0x32);
	hi253_write_reg(client,0x38, 0x04);

	hi253_write_reg(client,0x39, 0x22); //AE_escapeC10
	hi253_write_reg(client,0x3a, 0xde); //AE_escapeC11

	hi253_write_reg(client,0x3b, 0x22); //AE_escapeC1
	hi253_write_reg(client,0x3c, 0xde); //AE_escapeC2

	hi253_write_reg(client,0x50, 0x45);
	hi253_write_reg(client,0x51, 0x88);

	hi253_write_reg(client,0x56, 0x03);
	hi253_write_reg(client,0x57, 0xf7);
	hi253_write_reg(client,0x58, 0x14);
	hi253_write_reg(client,0x59, 0x88);
	hi253_write_reg(client,0x5a, 0x04);

	//New Weight For Samsung
	//hi253_write_reg(client,0x60, 0xaa);
	//hi253_write_reg(client,0x61, 0xaa);
	//hi253_write_reg(client,0x62, 0xaa);
	//hi253_write_reg(client,0x63, 0xaa);
	//hi253_write_reg(client,0x64, 0xaa);
	//hi253_write_reg(client,0x65, 0xaa);
	//hi253_write_reg(client,0x66, 0xab);
	//hi253_write_reg(client,0x67, 0xEa);
	//hi253_write_reg(client,0x68, 0xab);
	//hi253_write_reg(client,0x69, 0xEa);
	//hi253_write_reg(client,0x6a, 0xaa);
	//hi253_write_reg(client,0x6b, 0xaa);
	//hi253_write_reg(client,0x6c, 0xaa);
	//hi253_write_reg(client,0x6d, 0xaa);
	//hi253_write_reg(client,0x6e, 0xaa);
	//hi253_write_reg(client,0x6f, 0xaa);

	hi253_write_reg(client,0x60, 0x55); // AEWGT1
	hi253_write_reg(client,0x61, 0x55); // AEWGT2
	hi253_write_reg(client,0x62, 0x6a); // AEWGT3
	hi253_write_reg(client,0x63, 0xa9); // AEWGT4
	hi253_write_reg(client,0x64, 0x6a); // AEWGT5
	hi253_write_reg(client,0x65, 0xa9); // AEWGT6
	hi253_write_reg(client,0x66, 0x6a); // AEWGT7
	hi253_write_reg(client,0x67, 0xa9); // AEWGT8
	hi253_write_reg(client,0x68, 0x6b); // AEWGT9
	hi253_write_reg(client,0x69, 0xe9); // AEWGT10
	hi253_write_reg(client,0x6a, 0x6a); // AEWGT11
	hi253_write_reg(client,0x6b, 0xa9); // AEWGT12
	hi253_write_reg(client,0x6c, 0x6a); // AEWGT13
	hi253_write_reg(client,0x6d, 0xa9); // AEWGT14
	hi253_write_reg(client,0x6e, 0x55); // AEWGT15
	hi253_write_reg(client,0x6f, 0x55); // AEWGT16

	hi253_write_reg(client,0x70, 0x76); //6e
	hi253_write_reg(client,0x71, 0x00); //82(+8)->+0

	// haunting control
	hi253_write_reg(client,0x76, 0x43);
	hi253_write_reg(client,0x77, 0xe2); //04 //f2
	hi253_write_reg(client,0x78, 0x23); //Yth1
	hi253_write_reg(client,0x79, 0x42); //Yth2 //46
	hi253_write_reg(client,0x7a, 0x23); //23
	hi253_write_reg(client,0x7b, 0x22); //22
	hi253_write_reg(client,0x7d, 0x23);

	hi253_write_reg(client,0x83, 0x01); //EXP Normal 33.33 fps
	hi253_write_reg(client,0x84, 0x5f);
	hi253_write_reg(client,0x85, 0x00);

	hi253_write_reg(client,0x86, 0x02); //EXPMin 5859.38 fps
	hi253_write_reg(client,0x87, 0x00);

	//hi253_write_reg(client,0x88, 0x04); //EXP Max 10.00 fps
	//hi253_write_reg(client,0x89, 0x92);
	//hi253_write_reg(client,0x8a, 0x00);

	hi253_write_reg(client,0x88, 0x02); //EXP Max 15.00 fps
	hi253_write_reg(client,0x89, 0xbe);
	hi253_write_reg(client,0x8a, 0x00);

	hi253_write_reg(client,0x8B, 0x75); //EXP100
	hi253_write_reg(client,0x8C, 0x00);
	hi253_write_reg(client,0x8D, 0x61); //EXP120
	hi253_write_reg(client,0x8E, 0x00);

	hi253_write_reg(client,0x9c, 0x18); //EXP Limit 488.28 fps
	hi253_write_reg(client,0x9d, 0x00);
	hi253_write_reg(client,0x9e, 0x02); //EXP Unit
	hi253_write_reg(client,0x9f, 0x00);

	//AE_Middle Time option
	//hi253_write_reg(client,0xa0, 0x03);
	//hi253_write_reg(client,0xa1, 0xa9);
	//hi253_write_reg(client,0xa2, 0x80);

	hi253_write_reg(client,0xb0, 0x18);
	hi253_write_reg(client,0xb1, 0x14); //ADC 400->560
	hi253_write_reg(client,0xb2, 0xe0); //d0
	hi253_write_reg(client,0xb3, 0x18);
	hi253_write_reg(client,0xb4, 0x1a);
	hi253_write_reg(client,0xb5, 0x44);
	hi253_write_reg(client,0xb6, 0x2f);
	hi253_write_reg(client,0xb7, 0x28);
	hi253_write_reg(client,0xb8, 0x25);
	hi253_write_reg(client,0xb9, 0x22);
	hi253_write_reg(client,0xba, 0x21);
	hi253_write_reg(client,0xbb, 0x20);
	hi253_write_reg(client,0xbc, 0x1f);
	hi253_write_reg(client,0xbd, 0x1f);

	//AE_Adaptive Time option
	//hi253_write_reg(client,0xc0, 0x10);
	//hi253_write_reg(client,0xc1, 0x2b);
	//hi253_write_reg(client,0xc2, 0x2b);
	//hi253_write_reg(client,0xc3, 0x2b);
	//hi253_write_reg(client,0xc4, 0x08);

	hi253_write_reg(client,0xc8, 0x80);
	hi253_write_reg(client,0xc9, 0x40);

	/////// PAGE 22 START ///////
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x10, 0xfd);
	hi253_write_reg(client,0x11, 0x2e);
	hi253_write_reg(client,0x19, 0x01); // Low On //
	hi253_write_reg(client,0x20, 0x30);
	hi253_write_reg(client,0x21, 0x80);
	hi253_write_reg(client,0x24, 0x01);
	//hi253_write_reg(client,0x25, 0x00); //7f New Lock Cond & New light stable

	hi253_write_reg(client,0x30, 0x80);
	hi253_write_reg(client,0x31, 0x80);
	hi253_write_reg(client,0x38, 0x11);
	hi253_write_reg(client,0x39, 0x34);

	hi253_write_reg(client,0x40, 0xf4);
	hi253_write_reg(client,0x41, 0x55); //44
	hi253_write_reg(client,0x42, 0x33); //43

	hi253_write_reg(client,0x43, 0xf6);
	hi253_write_reg(client,0x44, 0x55); //44
	hi253_write_reg(client,0x45, 0x44); //33

	hi253_write_reg(client,0x46, 0x00);
	hi253_write_reg(client,0x50, 0xb2);
	hi253_write_reg(client,0x51, 0x81);
	hi253_write_reg(client,0x52, 0x98);

	hi253_write_reg(client,0x80, 0x40); //3e
	hi253_write_reg(client,0x81, 0x20);
	hi253_write_reg(client,0x82, 0x3e);

	hi253_write_reg(client,0x83, 0x5e); //5e
	hi253_write_reg(client,0x84, 0x1e); //24
	hi253_write_reg(client,0x85, 0x5e); //54 //56 //5a
	hi253_write_reg(client,0x86, 0x22); //24 //22

	hi253_write_reg(client,0x87, 0x49);
	hi253_write_reg(client,0x88, 0x39);
	hi253_write_reg(client,0x89, 0x37); //38
	hi253_write_reg(client,0x8a, 0x28); //2a

	hi253_write_reg(client,0x8b, 0x41); //47
	hi253_write_reg(client,0x8c, 0x39);
	hi253_write_reg(client,0x8d, 0x34);
	hi253_write_reg(client,0x8e, 0x28); //2c

	hi253_write_reg(client,0x8f, 0x53); //4e
	hi253_write_reg(client,0x90, 0x52); //4d
	hi253_write_reg(client,0x91, 0x51); //4c
	hi253_write_reg(client,0x92, 0x4e); //4a
	hi253_write_reg(client,0x93, 0x4a); //46
	hi253_write_reg(client,0x94, 0x45);
	hi253_write_reg(client,0x95, 0x3d);
	hi253_write_reg(client,0x96, 0x31);
	hi253_write_reg(client,0x97, 0x28);
	hi253_write_reg(client,0x98, 0x24);
	hi253_write_reg(client,0x99, 0x20);
	hi253_write_reg(client,0x9a, 0x20);

	hi253_write_reg(client,0x9b, 0x77);
	hi253_write_reg(client,0x9c, 0x77);
	hi253_write_reg(client,0x9d, 0x48);
	hi253_write_reg(client,0x9e, 0x38);
	hi253_write_reg(client,0x9f, 0x30);

	hi253_write_reg(client,0xa0, 0x60);
	hi253_write_reg(client,0xa1, 0x34);
	hi253_write_reg(client,0xa2, 0x6f);
	hi253_write_reg(client,0xa3, 0xff);

	hi253_write_reg(client,0xa4, 0x14); //1500fps
	hi253_write_reg(client,0xa5, 0x2c); // 700fps
	hi253_write_reg(client,0xa6, 0xcf);

	hi253_write_reg(client,0xad, 0x40);
	hi253_write_reg(client,0xae, 0x4a);

	hi253_write_reg(client,0xaf, 0x28);  // low temp Rgain
	hi253_write_reg(client,0xb0, 0x26);  // low temp Rgain

	hi253_write_reg(client,0xb1, 0x00); //0x20 -> 0x00 0405 modify
	hi253_write_reg(client,0xb4, 0xea);
	hi253_write_reg(client,0xb8, 0xa0); //a2: b-2, R+2  //b4 B-3, R+4 lowtemp
	hi253_write_reg(client,0xb9, 0x00);

	/////// PAGE 20 ///////
	hi253_write_reg(client,0x03, 0x20);
	hi253_write_reg(client,0x10, 0x8c);

	// PAGE 20
	hi253_write_reg(client,0x03, 0x20); //page 20
	hi253_write_reg(client,0x10, 0x9c); //ae off

	// PAGE 22
	hi253_write_reg(client,0x03, 0x22); //page 22
	hi253_write_reg(client,0x10, 0xe9); //awb off

	// PAGE 0
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x0e, 0x03); //PLL On
	hi253_write_reg(client,0x0e, 0x73); //PLLx2

	hi253_write_reg(client,0x03, 0x00); // Dummy 750us
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);
	hi253_write_reg(client,0x03, 0x00);

	hi253_write_reg(client,0x03, 0x00); // Page 0
	hi253_write_reg(client,0x01, 0x50); // Sleep Off
	return 0;
}

int hi253_preview_set(struct cim_sensor *sensor_info)
{

	struct hi253_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
	return 0;
}


int hi253_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
	struct hi253_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi253 size switch %d * %d\n",width,height);
#endif

	if(width == 1600 && height == 1200)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x00);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x10, 0x00); //close scaling
	}
	else if(width == 1280 && height == 1024)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x00);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x12, 0x20);
		hi253_write_reg(client,0x10, 0x07);
		hi253_write_reg(client,0x11, 0x00);
		hi253_write_reg(client,0x20, 0x06);
		hi253_write_reg(client,0x21, 0x40);
		hi253_write_reg(client,0x22, 0x04);
		hi253_write_reg(client,0x23, 0xb0);
		hi253_write_reg(client,0x24, 0x00);
		hi253_write_reg(client,0x25, 0xa0);
		hi253_write_reg(client,0x26, 0x00);
		hi253_write_reg(client,0x27, 0x58);
		hi253_write_reg(client,0x28, 0x05);
		hi253_write_reg(client,0x29, 0xa0);
		hi253_write_reg(client,0x2a, 0x04);
		hi253_write_reg(client,0x2b, 0x58);
		hi253_write_reg(client,0x2c, 0x08);
		hi253_write_reg(client,0x2d, 0x00);
		hi253_write_reg(client,0x2e, 0x08);
		hi253_write_reg(client,0x2f, 0x00);
		hi253_write_reg(client,0x30, 0x2c);
	}
	else if(width == 1024 && height == 768)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x00);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x12, 0x20);
		hi253_write_reg(client,0x10, 0x07);
		hi253_write_reg(client,0x11, 0x00);
		hi253_write_reg(client,0x20, 0x05);
		hi253_write_reg(client,0x21, 0x20);
		hi253_write_reg(client,0x22, 0x03);
		hi253_write_reg(client,0x23, 0xd8);
		hi253_write_reg(client,0x24, 0x00);
		hi253_write_reg(client,0x25, 0x90);
		hi253_write_reg(client,0x26, 0x00);
		hi253_write_reg(client,0x27, 0x6c);
		hi253_write_reg(client,0x28, 0x04);
		hi253_write_reg(client,0x29, 0x90);
		hi253_write_reg(client,0x2a, 0x03);
		hi253_write_reg(client,0x2b, 0x6c);
		hi253_write_reg(client,0x2c, 0x09);
		hi253_write_reg(client,0x2d, 0xc1);
		hi253_write_reg(client,0x2e, 0x09);
		hi253_write_reg(client,0x2f, 0xc1);
		hi253_write_reg(client,0x30, 0x56);
	}

	else if(width == 800 && height == 600)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x11);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x10, 0x00);

	}
	else if(width == 640 && height == 480)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x00);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x10, 0x07);
		hi253_write_reg(client,0x11, 0x00);
		hi253_write_reg(client,0x12, 0x20);
		hi253_write_reg(client,0x20, 0x02);
		hi253_write_reg(client,0x21, 0x84); //80);
		hi253_write_reg(client,0x22, 0x01);
		hi253_write_reg(client,0x23, 0xe0);
		hi253_write_reg(client,0x24, 0x00);
		hi253_write_reg(client,0x25, 0x04); //00);
		hi253_write_reg(client,0x26, 0x00);
		hi253_write_reg(client,0x27, 0x00);
		hi253_write_reg(client,0x28, 0x02);
		hi253_write_reg(client,0x29, 0x84); //80);
		hi253_write_reg(client,0x2a, 0x01);
		hi253_write_reg(client,0x2b, 0xe0);
		hi253_write_reg(client,0x2c, 0x14);
		hi253_write_reg(client,0x2d, 0x00);
		hi253_write_reg(client,0x2e, 0x14);
		hi253_write_reg(client,0x2f, 0x00);
		hi253_write_reg(client,0x30, 0x66);
	}
	else if(width == 352 && height == 288)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x00);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x12, 0x20);
		hi253_write_reg(client,0x10, 0x07);
		hi253_write_reg(client,0x11, 0x00);
		hi253_write_reg(client,0x20, 0x01);
		hi253_write_reg(client,0x21, 0x80);
		hi253_write_reg(client,0x22, 0x01);
		hi253_write_reg(client,0x23, 0x20);
		hi253_write_reg(client,0x24, 0x00);
		hi253_write_reg(client,0x25, 0x10);
		hi253_write_reg(client,0x26, 0x00);
		hi253_write_reg(client,0x27, 0x00);
		hi253_write_reg(client,0x28, 0x01);
		hi253_write_reg(client,0x29, 0x70);
		hi253_write_reg(client,0x2a, 0x01);
		hi253_write_reg(client,0x2b, 0x20);
		hi253_write_reg(client,0x2c, 0x21);
		hi253_write_reg(client,0x2d, 0x55);
		hi253_write_reg(client,0x2e, 0x21);
		hi253_write_reg(client,0x2f, 0x55);
		hi253_write_reg(client,0x30, 0x4b);
	}
	else if(width == 320 && height == 240)
	{

	}
	else if(width == 176 && height == 144)
	{
		hi253_write_reg(client,0x03, 0x00);
		hi253_write_reg(client,0x10, 0x00);
		hi253_write_reg(client,0x03, 0x18);
		hi253_write_reg(client,0x12, 0x20);
		hi253_write_reg(client,0x10, 0x07);
		hi253_write_reg(client,0x11, 0x00);
		hi253_write_reg(client,0x20, 0x00);
		hi253_write_reg(client,0x21, 0xc0);
		hi253_write_reg(client,0x22, 0x00);
		hi253_write_reg(client,0x23, 0x90);
		hi253_write_reg(client,0x24, 0x00);
		hi253_write_reg(client,0x25, 0x08);
		hi253_write_reg(client,0x26, 0x00);
		hi253_write_reg(client,0x27, 0x00);
		hi253_write_reg(client,0x28, 0x00);
		hi253_write_reg(client,0x29, 0xb8);
		hi253_write_reg(client,0x2a, 0x00);
		hi253_write_reg(client,0x2b, 0x90);
		hi253_write_reg(client,0x2c, 0x42);
		hi253_write_reg(client,0x2d, 0xaa);
		hi253_write_reg(client,0x2e, 0x42);
		hi253_write_reg(client,0x2f, 0xaa);
		hi253_write_reg(client,0x30, 0x2c);
	}
	else
		return 0;


	return 0;
}



int hi253_capture_set(struct cim_sensor *sensor_info)
{

	struct hi253_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"capture\n");
#endif
	return 0;
}

void hi253_set_ab_50hz(struct i2c_client *client)
{
	hi253_write_reg(client,0x03,0x20);
	hi253_write_reg(client,0x10,0x9c);
}

void hi253_set_ab_60hz(struct i2c_client *client)
{
	hi253_write_reg(client,0x03,0x20);
	hi253_write_reg(client,0x10,0x8c);
}

int hi253_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct hi253_sensor *s;
	struct i2c_client * client;
	unsigned char *str_antibanding;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi253_set_antibanding");
#endif
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			hi253_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
		case ANTIBANDING_50HZ :
			hi253_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_50HZ";
			break;
		case ANTIBANDING_60HZ :
			hi253_set_ab_60hz(client);
			str_antibanding = "ANTIBANDING_60HZ";
			break;
		case ANTIBANDING_OFF :
			str_antibanding = "ANTIBANDING_OFF";
			break;
		default:
			hi253_set_ab_50hz(client);
			str_antibanding = "ANTIBANDING_AUTO";
			break;
	}
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev, "set antibanding to %s\n", str_antibanding);
#endif
	return 0;
}

void hi253_set_effect_normal(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x11, 0x03);
	hi253_write_reg(client,0x12, 0x30);
	hi253_write_reg(client,0x13, 0x02);
	hi253_write_reg(client,0x44, 0x80);
	hi253_write_reg(client,0x45, 0x80);
}

void hi253_set_effect_grayscale(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x11, 0x03);
	hi253_write_reg(client,0x12, 0x03);
	hi253_write_reg(client,0x13, 0x02);
	hi253_write_reg(client,0x40, 0x00);
	hi253_write_reg(client,0x44, 0x80);
	hi253_write_reg(client,0x45, 0x80);
}

void hi253_set_effect_sepia(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x11, 0x03);
	hi253_write_reg(client,0x12, 0x33);
	hi253_write_reg(client,0x13, 0x00);
	hi253_write_reg(client,0x44, 0x70);
	hi253_write_reg(client,0x45, 0x98);
}

void hi253_set_effect_colorinv(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x11, 0x03);
	hi253_write_reg(client,0x12, 0x08);
	hi253_write_reg(client,0x13, 0x02);
	hi253_write_reg(client,0x14, 0x00);
}

void hi253_set_effect_sepiagreen(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x11, 0x03);
	hi253_write_reg(client,0x12, 0x03);
	hi253_write_reg(client,0x40, 0x00);
	hi253_write_reg(client,0x13, 0x02);
	hi253_write_reg(client,0x44, 0x30);
	hi253_write_reg(client,0x45, 0x50);

}

void hi253_set_effect_sepiablue(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x10);
	hi253_write_reg(client,0x11, 0x03);
	hi253_write_reg(client,0x12, 0x03);
	hi253_write_reg(client,0x40, 0x00);
	hi253_write_reg(client,0x13, 0x02);
	hi253_write_reg(client,0x44, 0xb0);
	hi253_write_reg(client,0x45, 0x40);
}

int hi253_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct hi253_sensor *s;
	struct i2c_client * client;
	unsigned char *str_effect;

	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi253_set_effect");
#endif
	switch(arg)
	{
		case EFFECT_NONE:
			hi253_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
		case EFFECT_MONO :
			hi253_set_effect_grayscale(client);
			str_effect = "EFFECT_MONO";
			break;
		case EFFECT_NEGATIVE :
			hi253_set_effect_colorinv(client);
			str_effect = "EFFECT_NEGATIVE";
			break;
		case EFFECT_SOLARIZE ://bao guang
			str_effect = "EFFECT_SOLARIZE";
			break;
		case EFFECT_SEPIA :
			hi253_set_effect_sepia(client);
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
			hi253_set_effect_sepiagreen(client);
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
			hi253_set_effect_normal(client);
			str_effect = "EFFECT_NONE";
			break;
	}
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"set effect to %s\n", str_effect);
#endif
	return 0;
}

void hi253_set_wb_auto(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x11, 0x2e);
	//hi253_write_reg(client,0x80, 0x38);
	//hi253_write_reg(client,0x82, 0x38);
	hi253_write_reg(client,0x83, 0x6e);
	hi253_write_reg(client,0x84, 0x14);
	hi253_write_reg(client,0x85, 0x69);
	hi253_write_reg(client,0x86, 0x14);
}

void hi253_set_wb_cloud(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x11, 0x28);
	hi253_write_reg(client,0x80, 0x71);
	hi253_write_reg(client,0x82, 0x2b);
	hi253_write_reg(client,0x83, 0x72);
	hi253_write_reg(client,0x84, 0x70);
	hi253_write_reg(client,0x85, 0x2b);
	hi253_write_reg(client,0x86, 0x28);
}

void hi253_set_wb_daylight(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x11, 0x28);
	hi253_write_reg(client,0x80, 0x59);
	hi253_write_reg(client,0x82, 0x29);
	hi253_write_reg(client,0x83, 0x60);
	hi253_write_reg(client,0x84, 0x50);
	hi253_write_reg(client,0x85, 0x2f);
	hi253_write_reg(client,0x86, 0x23);
}

void hi253_set_wb_incandescence(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x11, 0x28);
	hi253_write_reg(client,0x80, 0x29);
	hi253_write_reg(client,0x82, 0x54);
	hi253_write_reg(client,0x83, 0x2e);
	hi253_write_reg(client,0x84, 0x23);
	hi253_write_reg(client,0x85, 0x58);
	hi253_write_reg(client,0x86, 0x4f);
}

void hi253_set_wb_fluorescent(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x11, 0x28);
	hi253_write_reg(client,0x80, 0x41);
	hi253_write_reg(client,0x82, 0x42);
	hi253_write_reg(client,0x83, 0x44);
	hi253_write_reg(client,0x84, 0x34);
	hi253_write_reg(client,0x85, 0x46);
	hi253_write_reg(client,0x86, 0x3a);
}

void hi253_set_wb_tungsten(struct i2c_client *client)
{
	hi253_write_reg(client,0x03, 0x22);
	hi253_write_reg(client,0x80, 0x24);
	hi253_write_reg(client,0x82, 0x58);
	hi253_write_reg(client,0x83, 0x27);
	hi253_write_reg(client,0x84, 0x22);
	hi253_write_reg(client,0x85, 0x58);
	hi253_write_reg(client,0x86, 0x52);
}


int hi253_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct hi253_sensor *s;
	struct i2c_client * client ;
	unsigned char *str_balance;
	s = container_of(sensor_info, struct hi253_sensor, cs);
	client = s->client;
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"hi253_set_balance");
#endif
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			hi253_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
		case WHITE_BALANCE_INCANDESCENT :
			hi253_set_wb_incandescence(client);
			str_balance = "WHITE_BALANCE_INCANDESCENT";
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			hi253_set_wb_fluorescent(client);
			str_balance = "WHITE_BALANCE_FLUORESCENT";
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			str_balance = "WHITE_BALANCE_WARM_FLUORESCENT";
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			hi253_set_wb_daylight(client);
			str_balance = "WHITE_BALANCE_DAYLIGHT";
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			hi253_set_wb_cloud(client);
			str_balance = "WHITE_BALANCE_CLOUDY_DAYLIGHT";
			break;
		case WHITE_BALANCE_TWILIGHT :
			str_balance = "WHITE_BALANCE_TWILIGHT";
			break;
		case WHITE_BALANCE_SHADE :
			str_balance = "WHITE_BALANCE_SHADE";
			break;
		default:
			hi253_set_wb_auto(client);
			str_balance = "WHITE_BALANCE_AUTO";
			break;
	}
#ifdef HI253_SET_KERNEL_PRINT
	dev_info(&client->dev,"set balance to %s\n", str_balance);
#endif
	return 0;
}

