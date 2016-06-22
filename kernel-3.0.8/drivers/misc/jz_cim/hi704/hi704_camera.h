#ifndef __HI704_CAMERA_H__
#define __HI704_CAMERA_H__
#include <linux/csensor.h>

struct hi704_sensor {

	struct cim_sensor cs;
	struct i2c_client *client;
	//struct hi704_platform_data *pdata;
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int mirror;   //camera mirror

	char name[8];

	unsigned int width;
	unsigned int height;
};



int hi704_write_reg(struct i2c_client *client,unsigned char reg, unsigned char val);
unsigned char hi704_read_reg(struct i2c_client *client,unsigned char reg);

#endif

