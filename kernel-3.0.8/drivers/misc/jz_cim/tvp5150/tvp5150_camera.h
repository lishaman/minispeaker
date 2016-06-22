#ifndef __TVP5150_CAMERA_H__
#define __TVP5150_CAMERA_H__
struct tvp5150_platform_data {
	int facing;
	int orientation;
	int mirror;
	uint16_t gpio_rst;
	uint16_t gpio_en;
	int cap_wait_frame;
	unsigned char pos;      /* pos @ cim, pos should be 0 while only cim */         //twxie
};

struct tvp5150_sensor {
	struct cim_sensor cs;
	struct i2c_client *client;
	//struct ov7675_platform_data *pdata;
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int mirror;   //camera mirror

	char name[8];

	unsigned int width;
	unsigned int height;
};


int tvp5150_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val);
unsigned char tvp5150_read_reg(struct i2c_client *client,unsigned char reg);
#endif
