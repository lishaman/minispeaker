#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/jz_cim.h>

#include "ov2659_camera.h"
#include "ov2659_set.h"
#define CONFIG_SENSOR_I2C_RDWRCHK 0

/* 800*600 */
static struct reginfo sensor_init_data[] =
{
	{0x3000, 0x0f},
	{0x3001, 0xff},
	{0x3002, 0xff},
	//{0x0100, 0x01},	//software sleep : Sensor vsync singal may not output if haven't sleep the sensor when transfer the array
	{0x3633, 0x3d},
	{0x3620, 0x02},
	{0x3631, 0x11},
	{0x3612, 0x04},
	{0x3630, 0x20},
	{0x4702, 0x02},
	{0x370c, 0x34},
	{0x3004, 0x10},
	{0x3005, 0x18},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x5f},
	{0x3806, 0x04},
	{0x3807, 0xb7},
	{0x3808, 0x03},
	{0x3809, 0x20},
	{0x380a, 0x02},
	{0x380b, 0x58},
	{0x380c, 0x05},
	{0x380d, 0x14},
	{0x380e, 0x02},
	{0x380f, 0x68},
	{0x3811, 0x08},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3a02, 0x02},
	{0x3a03, 0x68},
	{0x3a08, 0x00},
	{0x3a09, 0x5c},
	{0x3a0a, 0x00},
	{0x3a0b, 0x4d},
	{0x3a0d, 0x08},
	{0x3a0e, 0x06},
	{0x3a14, 0x02},
	{0x3a15, 0x28},
	{0x4708, 0x01},
	{0x3623, 0x00},
	{0x3634, 0x76},
	{0x3701, 0x44},
	{0x3702, 0x18},
	{0x3703, 0x24},
	{0x3704, 0x24},
	{0x3705, 0x0c},
	{0x3820, 0x81},
	{0x3821, 0x01},
	{0x370a, 0x52},
	{0x4608, 0x00},
	{0x4609, 0x80},
	{0x4300, 0x32},
	{0x5086, 0x02},
	{0x5000, 0xfb},
	{0x5001, 0x1f},
	{0x5002, 0x00},
	{0x5025, 0x0e},
	{0x5026, 0x18},
	{0x5027, 0x34},
	{0x5028, 0x4c},
	{0x5029, 0x62},
	{0x502a, 0x74},
	{0x502b, 0x85},
	{0x502c, 0x92},
	{0x502d, 0x9e},
	{0x502e, 0xb2},
	{0x502f, 0xc0},
	{0x5030, 0xcc},
	{0x5031, 0xe0},
	{0x5032, 0xee},
	{0x5033, 0xf6},
	{0x5034, 0x11},
	{0x5070, 0x1c},
	{0x5071, 0x5b},
	{0x5072, 0x05},
	{0x5073, 0x20},
	{0x5074, 0x94},
	{0x5075, 0xb4},
	{0x5076, 0xb4},
	{0x5077, 0xaf},
	{0x5078, 0x05},
	{0x5079, 0x98},
	{0x507a, 0x21},
	{0x5035, 0x6a},
	{0x5036, 0x11},
	{0x5037, 0x92},
	{0x5038, 0x21},

	{0x5039, 0xe1},
	{0x503a, 0x01},
	{0x503c, 0x05},
	{0x503d, 0x08},
	{0x503e, 0x08},
	{0x503f, 0x64},
	{0x5040, 0x58},
	{0x5041, 0x2a},
	{0x5042, 0xc5},
	{0x5043, 0x2e},
	{0x5044, 0x3a},
	{0x5045, 0x3c},
	{0x5046, 0x44},
	{0x5047, 0xf8},
	{0x5048, 0x08},
	{0x5049, 0x70},
	{0x504a, 0xf0},
	{0x504b, 0xf0},
	{0x500c, 0x03},
	{0x500d, 0x20},
	{0x500e, 0x02},
	{0x500f, 0x5c},
	{0x5010, 0x48},
	{0x5011, 0x00},
	{0x5012, 0x66},
	{0x5013, 0x03},
	{0x5014, 0x30},
	{0x5015, 0x02},
	{0x5016, 0x7c},
	{0x5017, 0x40},
	{0x5018, 0x00},
	{0x5019, 0x66},
	{0x501a, 0x03},
	{0x501b, 0x10},
	{0x501c, 0x02},
	{0x501d, 0x7c},
	{0x501e, 0x3a},
	{0x501f, 0x00},
	{0x5020, 0x66},
	{0x506e, 0x44},
	{0x5064, 0x08},
	{0x5065, 0x10},
	{0x5066, 0x12},
	{0x5067, 0x02},
	{0x506c, 0x08},
	{0x506d, 0x10},
	{0x506f, 0xa6},
	{0x5068, 0x08},
	{0x5069, 0x10},
	{0x506a, 0x04},
	{0x506b, 0x12},
    /* saturation */
	{0x507e, 0x30},
	{0x507f, 0x30},
	{0x507b, 0x02},
	{0x507a, 0x01},
	{0x5084, 0x0c},
	{0x5085, 0x3e},
	{0x5005, 0x80},
	{0x3a0f, 0x30},
	{0x3a10, 0x28},
	{0x3a1b, 0x32},
	{0x3a1e, 0x26},
	{0x3a11, 0x60},
	{0x3a1f, 0x14},
	{0x5060, 0x69},
	{0x5061, 0x7d},
	{0x5062, 0x7d},
	{0x5063, 0x69},
	{0x3004, 0x20},
	{0x0100, 0x01},
	{0x0000, 0x00}
};


int ov2659_init(struct cim_sensor *sensor_info)
{
	struct ov2659_sensor *s;
	struct i2c_client * client;
    int ret = 0, i = 0, cnt = 5;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;
	/***************** init reg set **************************/
	/*** VGA preview (800X600) 30fps 24MCLK input ***********/

	printk("%s ---------------init\n", s->cs.name);
	ret = ov2659_write_reg(client, 0x0103, 0x01);
	if (ret != 0) {
		printk("%s soft reset sensor failed\n", s->cs.name);
		return -ENODEV;
	}
    mdelay(10);

#if CONFIG_SENSOR_I2C_RDWRCHK
	char valchk;
#endif

    while (sensor_init_data[i].reg != 0)
    {
    	ret = ov2659_write_reg(client, sensor_init_data[i].reg, sensor_init_data[i].val);
        if (ret < 0)
        {
            if (cnt-- > 0) {
			    printk("%s..write failed current reg:0x%x, Write array again !\n",
			    			s->cs.name, sensor_init_data[i].reg);
				i = 0;
				continue;
            } else {
                printk("%s..write array failed!!!\n", s->cs.name);
                return -EPERM;
            }
        } else {
        #if CONFIG_SENSOR_I2C_RDWRCHK
        	ov2659_read_reg(client, sensor_init_data[i].reg, &valchk);
			if (valchk != sensor_init_data[i].val)
				printk("%s Reg:0x%x write(0x%x, 0x%x) fail\n", s->cs.name,
						sensor_init_data[i].reg, sensor_init_data[i].val, valchk);
		#endif
        }
        i++;
    }

	return 0;
}

int ov2659_preview_set(struct cim_sensor *sensor_info)
{

	struct ov2659_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;
	/***************** preview reg set **************************/
	return 0;
}


int ov2659_size_switch(struct cim_sensor *sensor_info,int width,int height)
{
	return 0;
	struct ov2659_sensor *s;
	struct i2c_client * client;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov2659-----------------size switch %d * %d\n",width,height);

	if(width == 1600 && height == 1200)
    {
        ov2659_write_reg(client, 0x3013, 0xf2);
        ov2659_write_reg(client, 0x3014, 0x84);
        ov2659_write_reg(client, 0x3015, 0x01);
        ov2659_write_reg(client, 0x3012, 0x00);
        ov2659_write_reg(client, 0x302a, 0x05);
        ov2659_write_reg(client, 0x302b, 0xCB);
        ov2659_write_reg(client, 0x306f, 0x54);
        ov2659_write_reg(client, 0x3362, 0x80);
        ov2659_write_reg(client, 0x3070, 0x5d);
        ov2659_write_reg(client, 0x3072, 0x5d);
        ov2659_write_reg(client, 0x301c, 0x0f);
        ov2659_write_reg(client, 0x301d, 0x0f);
        ov2659_write_reg(client, 0x3020, 0x01);
        ov2659_write_reg(client, 0x3021, 0x18);
        ov2659_write_reg(client, 0x3022, 0x00);
        ov2659_write_reg(client, 0x3023, 0x0A);
        ov2659_write_reg(client, 0x3024, 0x06);
        ov2659_write_reg(client, 0x3025, 0x58);
        ov2659_write_reg(client, 0x3026, 0x04);
        ov2659_write_reg(client, 0x3027, 0xbc);
        ov2659_write_reg(client, 0x3088, 0x06);
        ov2659_write_reg(client, 0x3089, 0x40);
        ov2659_write_reg(client, 0x308A, 0x04);
        ov2659_write_reg(client, 0x308B, 0xB0);
        ov2659_write_reg(client, 0x3316, 0x64);
        ov2659_write_reg(client, 0x3317, 0x4B);
        ov2659_write_reg(client, 0x3318, 0x00);
        ov2659_write_reg(client, 0x3319, 0x6C);
        ov2659_write_reg(client, 0x331A, 0x64);
        ov2659_write_reg(client, 0x331B, 0x4B);
        ov2659_write_reg(client, 0x331C, 0x00);
        ov2659_write_reg(client, 0x331D, 0x6C);
        ov2659_write_reg(client, 0x3302, 0x01);
    }
    else if(width == 640 && height == 480)
    {
        ov2659_write_reg(client, 0x3012, 0x10);
        ov2659_write_reg(client, 0x302a, 0x02);
        ov2659_write_reg(client, 0x302b, 0xE6);
        ov2659_write_reg(client, 0x306f, 0x14);
        ov2659_write_reg(client, 0x3362, 0x90);
        ov2659_write_reg(client, 0x3070, 0x5D);
        ov2659_write_reg(client, 0x3072, 0x5D);
        ov2659_write_reg(client, 0x301c, 0x07);
        ov2659_write_reg(client, 0x301d, 0x07);
        ov2659_write_reg(client, 0x3020, 0x01);
        ov2659_write_reg(client, 0x3021, 0x18);
        ov2659_write_reg(client, 0x3022, 0x00);
        ov2659_write_reg(client, 0x3023, 0x06);
        ov2659_write_reg(client, 0x3024, 0x06);
        ov2659_write_reg(client, 0x3025, 0x58);
        ov2659_write_reg(client, 0x3026, 0x02);
        ov2659_write_reg(client, 0x3027, 0x61);
        ov2659_write_reg(client, 0x3088, 0x02);
        ov2659_write_reg(client, 0x3089, 0x80);
        ov2659_write_reg(client, 0x308A, 0x01);
        ov2659_write_reg(client, 0x308B, 0xe0);
        ov2659_write_reg(client, 0x3316, 0x64);
        ov2659_write_reg(client, 0x3317, 0x25);
        ov2659_write_reg(client, 0x3318, 0x80);
        ov2659_write_reg(client, 0x3319, 0x08);
        ov2659_write_reg(client, 0x331A, 0x28);
        ov2659_write_reg(client, 0x331B, 0x1e);
        ov2659_write_reg(client, 0x331C, 0x00);
        ov2659_write_reg(client, 0x331D, 0x38);
        ov2659_write_reg(client, 0x3302, 0x11);
    }
	else if(width == 352 && height == 288)
	{

	}
	else if(width == 176 && height == 144)
	{
	}
	else if(width == 320 && height == 240)
	{
	}
	else
		return 0;


	return 0;
}



int ov2659_capture_set(struct cim_sensor *sensor_info)
{

	struct ov2659_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;
	/***************** capture reg set **************************/

	dev_info(&client->dev,"------------------------------------capture\n");
	return 0;
}

void ov2659_set_ab_50hz(struct i2c_client *client)
{
#if 0
	int temp = ov2659_read_reg(client,0x3b);
	ov2659_write_reg(client,0x3b,temp|0x08);	    /* 50 Hz */
	ov2659_write_reg(client,0x9d,0x4c);
	ov2659_write_reg(client,0xa5,0x06);
#endif
}

void ov2659_set_ab_60hz(struct i2c_client *client)
{
#if 0
	int temp = ov2659_read_reg(client,0x3b);
	ov2659_write_reg(client,0x3b,temp&0xf7);	    /* 60 Hz */
	ov2659_write_reg(client,0x9e,0x3f);
	ov2659_write_reg(client,0xab,0x07);
#endif
}

int ov2659_set_antibanding(struct cim_sensor *sensor_info,unsigned short arg)
{
#if 0
	struct ov2659_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov2659_set_antibanding");
	switch(arg)
	{
		case ANTIBANDING_AUTO :
			ov2659_set_ab_50hz(client);
			dev_info(&client->dev,"ANTIBANDING_AUTO ");
			break;
		case ANTIBANDING_50HZ :
			ov2659_set_ab_50hz(client);
			dev_info(&client->dev,"ANTIBANDING_50HZ ");
			break;
		case ANTIBANDING_60HZ :
			ov2659_set_ab_60hz(client);
			dev_info(&client->dev,"ANTIBANDING_60HZ ");
			break;
		case ANTIBANDING_OFF :
			dev_info(&client->dev,"ANTIBANDING_OFF ");
			break;
	}
#endif
	return 0;
}
void ov2659_set_effect_normal(struct i2c_client *client)
{
   ov2659_write_reg(client,0x3A,0x0C);
   ov2659_write_reg(client,0x67,0x80);
   ov2659_write_reg(client,0x68,0x80);
   ov2659_write_reg(client,0x56,0x40);

}

void ov2659_set_effect_grayscale(struct i2c_client *client)
{
	return;
   ov2659_write_reg(client,0x3A,0x1C);
   ov2659_write_reg(client,0x67,0x80);
   ov2659_write_reg(client,0x68,0x80);
   ov2659_write_reg(client,0x56,0x40);
}

void ov2659_set_effect_sepia(struct i2c_client *client)
{
	return;
   ov2659_write_reg(client,0x3A,0x1C);
   ov2659_write_reg(client,0x67,0x40);
   ov2659_write_reg(client,0x68,0xa0);
   ov2659_write_reg(client,0x56,0x40);
}

void ov2659_set_effect_colorinv(struct i2c_client *client)
{
	return;
   ov2659_write_reg(client,0x3A,0x2C);
   ov2659_write_reg(client,0x67,0x80);
   ov2659_write_reg(client,0x68,0x80);
   ov2659_write_reg(client,0x56,0x40);
}

void ov2659_set_effect_sepiagreen(struct i2c_client *client)
{
	return;
   ov2659_write_reg(client,0x3A,0x1C);
   ov2659_write_reg(client,0x67,0x40);
   ov2659_write_reg(client,0x68,0x40);
   ov2659_write_reg(client,0x56,0x40);
}

void ov2659_set_effect_sepiablue(struct i2c_client *client)
{
	return;
   ov2659_write_reg(client,0x3A,0x1C);
   ov2659_write_reg(client,0x67,0xc0);
   ov2659_write_reg(client,0x68,0x80);
   ov2659_write_reg(client,0x56,0x40);
}


int ov2659_set_effect(struct cim_sensor *sensor_info,unsigned short arg)
{
#if 0
	struct ov2659_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;
	dev_info(&client->dev,"ov2659_set_effect");
		switch(arg)
		{
			case EFFECT_NONE:
				ov2659_set_effect_normal(client);
				dev_info(&client->dev,"EFFECT_NONE");
				break;
			case EFFECT_MONO :
				ov2659_set_effect_grayscale(client);
				dev_info(&client->dev,"EFFECT_MONO ");
				break;
			case EFFECT_NEGATIVE :
				ov2659_set_effect_colorinv(client);
				dev_info(&client->dev,"EFFECT_NEGATIVE ");
				break;
			case EFFECT_SOLARIZE ://bao guang
				dev_info(&client->dev,"EFFECT_SOLARIZE ");
				break;
			case EFFECT_SEPIA :
				ov2659_set_effect_sepia(client);
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
				ov2659_set_effect_sepiagreen(client);
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
#endif
		return 0;
}

void ov2659_set_wb_auto(struct i2c_client *client)
{
  int temp = ov2659_read_reg(client,0x13);
  ov2659_write_reg(client,0x13,temp|0x02);   // Enable AWB
  ov2659_write_reg(client,0x01,0x56);
  ov2659_write_reg(client,0x02,0x44);
}

void ov2659_set_wb_cloud(struct i2c_client *client)
{
  int temp = ov2659_read_reg(client,0x13);
  ov2659_write_reg(client,0x13,temp&~0x02);   // Disable AWB
  ov2659_write_reg(client,0x01,0x58);
  ov2659_write_reg(client,0x02,0x60);
  ov2659_write_reg(client,0x6a,0x40);
}

void ov2659_set_wb_daylight(struct i2c_client *client)
{
  int temp = ov2659_read_reg(client,0x13);
  ov2659_write_reg(client,0x13,temp&~0x02);   // Disable AWB
  ov2659_write_reg(client,0x01,0x56);
  ov2659_write_reg(client,0x02,0x5c);
  ov2659_write_reg(client,0x6a,0x42);

}


void ov2659_set_wb_incandescence(struct i2c_client *client)
{
  int temp = ov2659_read_reg(client,0x13);
  ov2659_write_reg(client,0x13,temp&~0x02);   // Disable AWB
  ov2659_write_reg(client,0x01,0x9a);
  ov2659_write_reg(client,0x02,0x40);
  ov2659_write_reg(client,0x6a,0x48);
}

void ov2659_set_wb_fluorescent(struct i2c_client *client)
{
  int temp = ov2659_read_reg(client,0x13);
  ov2659_write_reg(client,0x13,temp&~0x02);   // Disable AWB
  ov2659_write_reg(client,0x01,0x8b);
  ov2659_write_reg(client,0x02,0x42);
  ov2659_write_reg(client,0x6a,0x40);

}

void ov2659_set_wb_tungsten(struct i2c_client *client)
{
  int temp = ov2659_read_reg(client,0x13);
  ov2659_write_reg(client,0x13,temp&~0x02);   // Disable AWB
  ov2659_write_reg(client,0x01,0xb8);
  ov2659_write_reg(client,0x02,0x40);
  ov2659_write_reg(client,0x6a,0x4f);
}

int ov2659_set_balance(struct cim_sensor *sensor_info,unsigned short arg)
{
#if 0
	struct ov2659_sensor *s;
	struct i2c_client * client ;
	s = container_of(sensor_info, struct ov2659_sensor, cs);
	client = s->client;

	dev_info(&client->dev,"ov2659_set_balance");
	switch(arg)
	{
		case WHITE_BALANCE_AUTO:
			ov2659_set_wb_auto(client);
			dev_info(&client->dev,"WHITE_BALANCE_AUTO");
			break;
		case WHITE_BALANCE_INCANDESCENT :
			ov2659_set_wb_incandescence(client);
			dev_info(&client->dev,"WHITE_BALANCE_INCANDESCENT ");
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			ov2659_set_wb_fluorescent(client);
			dev_info(&client->dev,"WHITE_BALANCE_FLUORESCENT ");
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			dev_info(&client->dev,"WHITE_BALANCE_WARM_FLUORESCENT ");
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			ov2659_set_wb_daylight(client);
			dev_info(&client->dev,"WHITE_BALANCE_DAYLIGHT ");
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			ov2659_set_wb_cloud(client);
			dev_info(&client->dev,"WHITE_BALANCE_CLOUDY_DAYLIGHT ");
			break;
		case WHITE_BALANCE_TWILIGHT :
			dev_info(&client->dev,"WHITE_BALANCE_TWILIGHT ");
			break;
		case WHITE_BALANCE_SHADE :
			dev_info(&client->dev,"WHITE_BALANCE_SHADE ");
			break;
	}
#endif
	return 0;
}

