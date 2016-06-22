#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "gc2015_camera.h"

#define SP0838_SET_KERNEL_PRINT

#ifdef gc2015_DEBUG
#define dprintk(x...)   do{printk("gc2015---\t");printk(x);printk("\n");}while(0)
#else
#define dprintk(x...)
#endif
#define mirror  1

int gc2015_init(struct cim_sensor *sensor_info)
{
	struct gc2015_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;

	gc2015_write_reg(client,0xfe , 0x80); //soft reset
	gc2015_write_reg(client,0xfe , 0x80); //soft reset
	gc2015_write_reg(client,0xfe , 0x80); //soft reset
	
	gc2015_write_reg(client,0xfe , 0x00);
	gc2015_write_reg(client,0x45 , 0x00); //output_disable
	//////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////preview capture switch /////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////
	//preview
	gc2015_write_reg(client,0x02 , 0x01); //preview mode
	gc2015_write_reg(client,0x2a , 0xca); //[7]col_binning , 0x[6]even skip
	gc2015_write_reg(client,0x48 , 0x40); //manual_gain

	gc2015_write_reg(client,0xfe , 0x01);
	////////////////////////////////////////////////////////////////////////
	////////////////////////// preview LSC /////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	
	gc2015_write_reg(client,0xb0 , 0x13); //[4]Y_LSC_en [3]lsc_compensate [2]signed_b4 [1:0]pixel array select
	gc2015_write_reg(client,0xb1 , 0x20); //P_LSC_red_b2
	gc2015_write_reg(client,0xb2 , 0x20); //P_LSC_green_b2
	gc2015_write_reg(client,0xb3 , 0x20); //P_LSC_blue_b2
	gc2015_write_reg(client,0xb4 , 0x20); //P_LSC_red_b4
	gc2015_write_reg(client,0xb5 , 0x20); //P_LSC_green_b4
	gc2015_write_reg(client,0xb6 , 0x20); //P_LSC_blue_b4
	gc2015_write_reg(client,0xb7 , 0x00); //P_LSC_compensate_b2
	gc2015_write_reg(client,0xb8 , 0x80); //P_LSC_row_center , 0x344 , 0x (0x600/2-100)/2=100
	gc2015_write_reg(client,0xb9 , 0x80); //P_LSC_col_center , 0x544 , 0x (0x800/2-200)/2=100

	////////////////////////////////////////////////////////////////////////
	////////////////////////// capture LSC ///////////////////////////
	////////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0xba , 0x13); //[4]Y_LSC_en [3]lsc_compensate [2]signed_b4 [1:0]pixel array select
	gc2015_write_reg(client,0xbb , 0x20); //C_LSC_red_b2
	gc2015_write_reg(client,0xbc , 0x20); //C_LSC_green_b2
	gc2015_write_reg(client,0xbd , 0x20); //C_LSC_blue_b2
	gc2015_write_reg(client,0xbe , 0x20); //C_LSC_red_b4
	gc2015_write_reg(client,0xbf , 0x20); //C_LSC_green_b4
	gc2015_write_reg(client,0xc0 , 0x20); //C_LSC_blue_b4
	gc2015_write_reg(client,0xc1 , 0x00); //C_Lsc_compensate_b2
	gc2015_write_reg(client,0xc2 , 0x80); //C_LSC_row_center , 0x344 , 0x (0x1200/2-344)/2=128
	gc2015_write_reg(client,0xc3 , 0x80); //C_LSC_col_center , 0x544 , 0x (0x1600/2-544)/2=128

	gc2015_write_reg(client,0xfe , 0x00);

	////////////////////////////////////////////////////////////////////////
	////////////////////////// analog configure ///////////////////////////
	////////////////////////////////////////////////////////////////////////
	if(mirror) 
		gc2015_write_reg(client,0x29 , 0x00); //cisctl mode 1
	else
		gc2015_write_reg(client,0x29 , 0x01); //cisctl mode 1
	
	gc2015_write_reg(client,0x2b , 0x06); //cisctl mode 3	
	gc2015_write_reg(client,0x32 , 0x0c); //analog mode 1
	gc2015_write_reg(client,0x33 , 0x0f); //analog mode 2
	gc2015_write_reg(client,0x34 , 0x00); //[6:4]da_rsg
	
	gc2015_write_reg(client,0x35 , 0x88); //Vref_A25
	gc2015_write_reg(client,0x37 , 0x03); //Drive Current 16  13  03

	/////////////////////////////////////////////////////////////////////
	///////////////////////////ISP Related//////////////////////////////
	/////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x40 , 0xff); 
	gc2015_write_reg(client,0x41 , 0x24); //[5]skin_detectionenable[2]auto_gray , 0x[1]y_gamma
	gc2015_write_reg(client,0x42 , 0x76); //[7]auto_sa[6]auto_ee[5]auto_dndd[4]auto_lsc[3]na[2]abs , 0x[1]awb
	gc2015_write_reg(client,0x4b , 0xea); //[1]AWB_gain_mode , 0x1:atpregain0:atpostgain
	gc2015_write_reg(client,0x4d , 0x03); //[1]inbf_en
	gc2015_write_reg(client,0x4f , 0x01); //AEC enable

	////////////////////////////////////////////////////////////////////
	/////////////////////////// BLK  ///////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x63 , 0x77); //BLK mode 1
	gc2015_write_reg(client,0x66 , 0x00); //BLK global offset
	gc2015_write_reg(client,0x6d , 0x04); 
	gc2015_write_reg(client,0x6e , 0x18); //BLK offset submode,offset R
	gc2015_write_reg(client,0x6f , 0x10);
	gc2015_write_reg(client,0x70 , 0x18);
	gc2015_write_reg(client,0x71 , 0x10);
	gc2015_write_reg(client,0x73 , 0x03); 


	////////////////////////////////////////////////////////////////////
	/////////////////////////// DNDD ////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x80 , 0x07); //[7]dn_inc_or_dec [4]zero_weight_mode[3]share [2]c_weight_adap [1]dn_lsc_mode [0]dn_b
	gc2015_write_reg(client,0x82 , 0x08); //DN lilat b base

	////////////////////////////////////////////////////////////////////
	/////////////////////////// EEINTP ////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x8a , 0x7c);
	gc2015_write_reg(client,0x8c , 0x02);
	gc2015_write_reg(client,0x8e , 0x02);
	gc2015_write_reg(client,0x8f , 0x48);

	/////////////////////////////////////////////////////////////////////
	/////////////////////////// CC_t ///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0xb0 , 0x44);
	gc2015_write_reg(client,0xb1 , 0xfe);
	gc2015_write_reg(client,0xb2 , 0x00);
	gc2015_write_reg(client,0xb3 , 0xf8);
	gc2015_write_reg(client,0xb4 , 0x48);
	gc2015_write_reg(client,0xb5 , 0xf8);
	gc2015_write_reg(client,0xb6 , 0x00);
	gc2015_write_reg(client,0xb7 , 0x04);
	gc2015_write_reg(client,0xb8 , 0x00);

	/////////////////////////////////////////////////////////////////////
	/////////////////////////// GAMMA ///////////////////////////////////
	/////////////////////////////////////////////////////////////////////
	//RGB_GAMMA
	gc2015_write_reg(client,0xbf , 0x0e);
	gc2015_write_reg(client,0xc0 , 0x1c);
	gc2015_write_reg(client,0xc1 , 0x34);
	gc2015_write_reg(client,0xc2 , 0x48);
	gc2015_write_reg(client,0xc3 , 0x5a);
	gc2015_write_reg(client,0xc4 , 0x6b);
	gc2015_write_reg(client,0xc5 , 0x7b);
	gc2015_write_reg(client,0xc6 , 0x95);
	gc2015_write_reg(client,0xc7 , 0xab);
	gc2015_write_reg(client,0xc8 , 0xbf);
	gc2015_write_reg(client,0xc9 , 0xce);
	gc2015_write_reg(client,0xca , 0xd9);
	gc2015_write_reg(client,0xcb , 0xe4);
	gc2015_write_reg(client,0xcc , 0xec);
	gc2015_write_reg(client,0xcd , 0xf7);
	gc2015_write_reg(client,0xce , 0xfd);
	gc2015_write_reg(client,0xcf , 0xff);

	/////////////////////////////////////////////////////////////////////
	/////////////////////////// YCP_t  ///////////////////////////////
	/////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0xd1 , 0x38); //saturation
	gc2015_write_reg(client,0xd2 , 0x38); //saturation
	gc2015_write_reg(client,0xde , 0x21); //auto_gray

	////////////////////////////////////////////////////////////////////
	/////////////////////////// ASDE ////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x98 , 0x30);
	gc2015_write_reg(client,0x99 , 0xf0);
	gc2015_write_reg(client,0x9b , 0x00);

	gc2015_write_reg(client,0xfe , 0x01);
	////////////////////////////////////////////////////////////////////
	/////////////////////////// AEC  ////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x10 , 0x45); //AEC mode 1
	gc2015_write_reg(client,0x11 , 0x32); //[7]fix target
	gc2015_write_reg(client,0x13 , 0x60);
	gc2015_write_reg(client,0x17 , 0x00);
	gc2015_write_reg(client,0x1c , 0x96);
	gc2015_write_reg(client,0x1e , 0x11);
	gc2015_write_reg(client,0x21 , 0xc0); //max_post_gain
	gc2015_write_reg(client,0x22 , 0x40); //max_pre_gain
	gc2015_write_reg(client,0x2d , 0x06); //P_N_AEC_exp_level_1[12:8]
	gc2015_write_reg(client,0x2e , 0x00); //P_N_AEC_exp_level_1[7:0]
	gc2015_write_reg(client,0x1e , 0x32);
	gc2015_write_reg(client,0x33 , 0x00); //[6:5]max_exp_level [4:0]min_exp_level

	////////////////////////////////////////////////////////////////////
	///////////////////////////  AWB  ////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x57 , 0x40); //number limit
	gc2015_write_reg(client,0x5d , 0x44); //
	gc2015_write_reg(client,0x5c , 0x35); //show mode,close dark_mode
	gc2015_write_reg(client,0x5e , 0x29); //close color temp
	gc2015_write_reg(client,0x5f , 0x50);
	gc2015_write_reg(client,0x60 , 0x50); 
	gc2015_write_reg(client,0x65 , 0xc0);

	////////////////////////////////////////////////////////////////////
	///////////////////////////  ABS  ////////////////////////////////
	////////////////////////////////////////////////////////////////////
	gc2015_write_reg(client,0x80 , 0x82);
	gc2015_write_reg(client,0x81 , 0x00);
	gc2015_write_reg(client,0x83 , 0x00); //ABS Y stretch limit

	gc2015_write_reg(client,0xfe , 0x00);

//////qhliu
	gc2015_write_reg(client,0x44 , 0xa2); //YUV sequence
	gc2015_write_reg(client,0x45 , 0x0f); //output enable
	gc2015_write_reg(client,0x46 , 0x03); //sync mode

/////qhliu add end

//---------------Updated By Mormo 2011/03/14 Start----------------//
	gc2015_write_reg(client,0xfe, 0x01);//page1
	gc2015_write_reg(client,0x34, 0x02); //Preview minimum exposure
	gc2015_write_reg(client,0xfe, 0x00); //page0
//---------------Updated By Mormo 2011/03/14 End----------------//


	return 0;
}

int gc2015_preview_set(struct cim_sensor *sensor_info)                   
{                               

	struct gc2015_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;
	return 0;
} 


int gc2015_size_switch(struct cim_sensor *sensor_info,int width,int height)
{	
	struct gc2015_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;

	if (width == 1600 && height == 1200)
	{
		dprintk("--------------------------------------- preview 1600x1200!");
		gc2015_write_reg(client,0xfe,0x00);
		gc2015_write_reg(client,0x02,0x00);
		gc2015_write_reg(client,0x2a,0x0a);

		gc2015_write_reg(client,0x59,0x11); 
		gc2015_write_reg(client,0x5a,0x06); 
		gc2015_write_reg(client,0x5b,0x00); 
		gc2015_write_reg(client,0x5c,0x00); 
		gc2015_write_reg(client,0x5d,0x00);
		gc2015_write_reg(client,0x5e,0x00);
		gc2015_write_reg(client,0x5f,0x00);
		gc2015_write_reg(client,0x60,0x00);  
		gc2015_write_reg(client,0x61,0x00);  
		gc2015_write_reg(client,0x62,0x00); 

		gc2015_write_reg(client,0x50,0x01); 
		gc2015_write_reg(client,0x51,0x00); 
		gc2015_write_reg(client,0x52,0x00); 
		gc2015_write_reg(client,0x53,0x00);
		gc2015_write_reg(client,0x54,0x00);
		gc2015_write_reg(client,0x55,0x04); 
		gc2015_write_reg(client,0x56,0xb0); 
		gc2015_write_reg(client,0x57,0x06);
		gc2015_write_reg(client,0x58,0x40);
	}
	else if (width == 1280 && height == 960)
	{
		dprintk("--------------------------------------- preview 1280x960!");
		gc2015_write_reg(client,0xfe,0x00);
		gc2015_write_reg(client,0x02,0x00);
		gc2015_write_reg(client,0x2a,0x0a);

		gc2015_write_reg(client,0x59,0x55); 
		gc2015_write_reg(client,0x5a,0x06); 
		gc2015_write_reg(client,0x5b,0x00); 
		gc2015_write_reg(client,0x5c,0x00); 
		gc2015_write_reg(client,0x5d,0x01);
		gc2015_write_reg(client,0x5e,0x23);
		gc2015_write_reg(client,0x5f,0x00);
		gc2015_write_reg(client,0x60,0x00);  
		gc2015_write_reg(client,0x61,0x01);  
		gc2015_write_reg(client,0x62,0x23); 

		gc2015_write_reg(client,0x50,0x01); 
		gc2015_write_reg(client,0x51,0x00); 
		gc2015_write_reg(client,0x52,0x00); 
		gc2015_write_reg(client,0x53,0x00);
		gc2015_write_reg(client,0x54,0x00);
		gc2015_write_reg(client,0x55,0x03); 
		gc2015_write_reg(client,0x56,0xc0); 
		gc2015_write_reg(client,0x57,0x05);
		gc2015_write_reg(client,0x58,0x00);
	}
	else if (width == 1024 && height == 768)
	{
		dprintk("--------------------------------------- preview 1024x768!");
		gc2015_write_reg(client,0xfe,0x00);
		gc2015_write_reg(client,0x02,0x00);
		gc2015_write_reg(client,0x2a,0x0a);

		gc2015_write_reg(client,0x59,0x33); 
		gc2015_write_reg(client,0x5a,0x06); 
		gc2015_write_reg(client,0x5b,0x00); 
		gc2015_write_reg(client,0x5c,0x00); 
		gc2015_write_reg(client,0x5d,0x00);
		gc2015_write_reg(client,0x5e,0x01);
		gc2015_write_reg(client,0x5f,0x00);
		gc2015_write_reg(client,0x60,0x00);  
		gc2015_write_reg(client,0x61,0x00);  
		gc2015_write_reg(client,0x62,0x01); 

		gc2015_write_reg(client,0x50,0x01); 
		gc2015_write_reg(client,0x51,0x00); 
		gc2015_write_reg(client,0x52,0x10); 
		gc2015_write_reg(client,0x53,0x00);
		gc2015_write_reg(client,0x54,0x14);
		gc2015_write_reg(client,0x55,0x03); 
		gc2015_write_reg(client,0x56,0x00); 
		gc2015_write_reg(client,0x57,0x04);
		gc2015_write_reg(client,0x58,0x00);
	}
/*
	else if (width == 800 && height == 600)
	{
		dprintk("--------------------------------------- preview 800x600!");
	}
*/
	else if (width == 640 && height == 480)
	{
		dprintk("--------------------------------------- preview 640x480!");
		gc2015_write_reg(client,0xfe,0x00);
		gc2015_write_reg(client,0x02,0x01);
		gc2015_write_reg(client,0x2a,0xca);

		gc2015_write_reg(client,0x59,0x55); 
		gc2015_write_reg(client,0x5a,0x06); 
		gc2015_write_reg(client,0x5b,0x00); 
		gc2015_write_reg(client,0x5c,0x00); 
		gc2015_write_reg(client,0x5d,0x01);
		gc2015_write_reg(client,0x5e,0x23);
		gc2015_write_reg(client,0x5f,0x00);
		gc2015_write_reg(client,0x60,0x00);  
		gc2015_write_reg(client,0x61,0x01);  
		gc2015_write_reg(client,0x62,0x23); 

		gc2015_write_reg(client,0x50,0x01); 
		gc2015_write_reg(client,0x51,0x00); 
		gc2015_write_reg(client,0x52,0x00); 
		gc2015_write_reg(client,0x53,0x00);
		gc2015_write_reg(client,0x54,0x00);
		gc2015_write_reg(client,0x55,0x01); 
		gc2015_write_reg(client,0x56,0xe0); 
		gc2015_write_reg(client,0x57,0x02);
		gc2015_write_reg(client,0x58,0x80);
	}
	else if (width == 352 && height == 288)
	{  
	    dprintk("--------------------------------------- preview 352x288!");
		gc2015_write_reg(client,0xfe,0x00);
		gc2015_write_reg(client,0x02,0x01);
		gc2015_write_reg(client,0x2a,0xca);

		gc2015_write_reg(client,0x59,0x22); 
		gc2015_write_reg(client,0x5a,0x06); 
		gc2015_write_reg(client,0x5b,0x00); 
		gc2015_write_reg(client,0x5c,0x00); 
		gc2015_write_reg(client,0x5d,0x00);
		gc2015_write_reg(client,0x5e,0x00);
		gc2015_write_reg(client,0x5f,0x00);
		gc2015_write_reg(client,0x60,0x00);  
		gc2015_write_reg(client,0x61,0x00);  
		gc2015_write_reg(client,0x62,0x00); 

		gc2015_write_reg(client,0x50,0x01); 
		gc2015_write_reg(client,0x51,0x00); 
		gc2015_write_reg(client,0x52,0x06); 
		gc2015_write_reg(client,0x53,0x00);
		gc2015_write_reg(client,0x54,0x18);
		gc2015_write_reg(client,0x55,0x01); 
		gc2015_write_reg(client,0x56,0x20); 
		gc2015_write_reg(client,0x57,0x01);
		gc2015_write_reg(client,0x58,0x60);
	} 
	else if (width == 176 && height == 144)
	{
		dprintk("--------------------------------------- preview 176x144!");
		gc2015_write_reg(client,0xfe,0x00);
		gc2015_write_reg(client,0x02,0x01);
		gc2015_write_reg(client,0x2a,0xca);

		gc2015_write_reg(client,0x59,0x44); 
		gc2015_write_reg(client,0x5a,0x06); 
		gc2015_write_reg(client,0x5b,0x00); 
		gc2015_write_reg(client,0x5c,0x00); 
		gc2015_write_reg(client,0x5d,0x00);
		gc2015_write_reg(client,0x5e,0x00);
		gc2015_write_reg(client,0x5f,0x00);
		gc2015_write_reg(client,0x60,0x00);  
		gc2015_write_reg(client,0x61,0x00);  
		gc2015_write_reg(client,0x62,0x00); 

		gc2015_write_reg(client,0x50,0x01); 
		gc2015_write_reg(client,0x51,0x00); 
		gc2015_write_reg(client,0x52,0x02); 
		gc2015_write_reg(client,0x53,0x00);
		gc2015_write_reg(client,0x54,0x0c);
		gc2015_write_reg(client,0x55,0x00); 
		gc2015_write_reg(client,0x56,0x90); 
		gc2015_write_reg(client,0x57,0x00);
		gc2015_write_reg(client,0x58,0xb0);
	}

}



int gc2015_capture_set(struct cim_sensor *sensor_info)
{

	struct gc2015_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;
	return 0;
}

void gc2015_set_ab_50hz(struct i2c_client *client)
{

}

void gc2015_set_ab_60hz(struct i2c_client *client)
{
 
}

int gc2015_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gc2015_sensor *s;
	struct i2c_client * client;
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;

	dprintk("gc2015_set_antibanding");
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			break;
		case ANTIBANDING_50HZ :
			gc2015_set_ab_50hz(client);
			dprintk("ANTIBANDING_50HZ ");
			break;
		case ANTIBANDING_60HZ :
			gc2015_set_ab_60hz(client);
			dprintk("ANTIBANDING_60HZ ");
			break;
		case ANTIBANDING_OFF :
			dprintk("ANTIBANDING_OFF ");
			break;
	}


	return 0;
}

void gc2015_set_effect_normal(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x43,0x00);
	dprintk("--------------------------effect_normal ");
}

void gc2015_set_effect_grayscale(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x43,0x02);
	gc2015_write_reg(client,0xda,0x00); 
	gc2015_write_reg(client,0xdb,0x00);
	dprintk("--------------------------effect_grayscale ");
}

void gc2015_set_effect_sepia(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x43,0x02);
	gc2015_write_reg(client,0xda,0x28); 
	gc2015_write_reg(client,0xdb,0x28);
	dprintk("--------------------------effect_sepia ");
}

void gc2015_set_effect_colorinv(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x43,0x01);
	dprintk("--------------------------effect_colorinv ");
}

void gc2015_set_effect_sepiagreen(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x43,0x02);
	gc2015_write_reg(client,0xda,0xc0); 
	gc2015_write_reg(client,0xdb,0xc0);
	dprintk("--------------------------effect_sepiagreen ");
}

void gc2015_set_effect_sepiablue(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x43,0x02);
	gc2015_write_reg(client,0xda,0x50); 
	gc2015_write_reg(client,0xdb,0xe0);
	dprintk("--------------------------effect_sepiablue ");
}

void gc2015_set_wb_auto(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x42,0x76);
	dprintk("-------------------------------wb_auto");		
}

void gc2015_set_wb_cloud(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x42,0x74);
	gc2015_write_reg(client,0x7a,0x8c);
	gc2015_write_reg(client,0x7b,0x50);
	gc2015_write_reg(client,0x7c,0x40);
	dprintk("-------------------------------wb_cloud");
}

void gc2015_set_wb_daylight(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x42,0x74);
	gc2015_write_reg(client,0x7a,0x74);
	gc2015_write_reg(client,0x7b,0x52);
	gc2015_write_reg(client,0x7c,0x40);
	dprintk("-------------------------------wb_daylight");
}

void gc2015_set_wb_incandescence(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x42,0x74);
	gc2015_write_reg(client,0x7a,0x48);
	gc2015_write_reg(client,0x7b,0x40);
	gc2015_write_reg(client,0x7c,0x5c);	
	dprintk("-------------------------------wb_incandescence");                               
}

void gc2015_set_wb_fluorescent(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x42,0x74);
	gc2015_write_reg(client,0x7a,0x40);
	gc2015_write_reg(client,0x7b,0x42);
	gc2015_write_reg(client,0x7c,0x50);
	dprintk("-------------------------------wb_fluorescent");                        
}

void gc2015_set_wb_tungsten(struct i2c_client *client)
{
	gc2015_write_reg(client,0xfe,0x00);
	gc2015_write_reg(client,0x42,0x74);
	gc2015_write_reg(client,0x7a,0x40);
	gc2015_write_reg(client,0x7b,0x54);
	gc2015_write_reg(client,0x7c,0x70);
	dprintk("-------------------------------wb_tungsten");
}

int gc2015_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gc2015_sensor *s;
	struct i2c_client * client;
	
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;

	dprintk("gc2015_set_effect");
	switch(arg)
	{
		case EFFECT_NONE:
			gc2015_set_effect_normal(client);
			break;
		case EFFECT_MONO :
			gc2015_set_effect_grayscale(client);
			dprintk("EFFECT_MONO ");
			break;
		case EFFECT_NEGATIVE :
			gc2015_set_effect_colorinv(client);
			dprintk("EFFECT_NEGATIVE ");
			break;
		case EFFECT_SOLARIZE ://bao guang
			dprintk("EFFECT_SOLARIZE ");
			break;
		case EFFECT_SEPIA :
			gc2015_set_effect_sepia(client);
			dprintk("EFFECT_SEPIA ");
			break;
		case EFFECT_POSTERIZE ://se diao fen li
			dprintk("EFFECT_POSTERIZE ");
			break;
		case EFFECT_WHITEBOARD :
			dprintk("EFFECT_WHITEBOARD ");
			break;
		case EFFECT_BLACKBOARD :
			dprintk("EFFECT_BLACKBOARD ");
			break;
		case EFFECT_AQUA  ://qian lv se
			gc2015_set_effect_sepiagreen(client);
			dprintk("EFFECT_AQUA  ");
			break;
		case EFFECT_PASTEL:
			dprintk("EFFECT_PASTEL");
			break;
		case EFFECT_MOSAIC:
			dprintk("EFFECT_MOSAIC");
			break;
		case EFFECT_RESIZE:
			dprintk("EFFECT_RESIZE");
			break;
	}

}


int gc2015_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
	struct gc2015_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct gc2015_sensor, cs);
	client = s->client;

	dprintk("--------------------------------gc2015_set_balance");
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			gc2015_set_wb_auto(client);
			break;
		case WHITE_BALANCE_INCANDESCENT :
			gc2015_set_wb_incandescence(client);
			dprintk("WHITE_BALANCE_INCANDESCENT ");
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			gc2015_set_wb_fluorescent(client);
			dprintk("WHITE_BALANCE_FLUORESCENT ");
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			dprintk("WHITE_BALANCE_WARM_FLUORESCENT ");
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			gc2015_set_wb_daylight(client);
			dprintk("WHITE_BALANCE_DAYLIGHT ");
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			gc2015_set_wb_cloud(client);
			dprintk("WHITE_BALANCE_CLOUDY_DAYLIGHT ");
			break;
		case WHITE_BALANCE_TWILIGHT :
			dprintk("WHITE_BALANCE_TWILIGHT ");
			break;
		case WHITE_BALANCE_SHADE :
			dprintk("WHITE_BALANCE_SHADE ");
			break;
	}

	return 0;
}

