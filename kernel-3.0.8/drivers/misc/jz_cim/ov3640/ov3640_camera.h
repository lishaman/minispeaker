#ifndef __ov3640_CAMERA_H__
#define __ov3640_CAMERA_H__

struct cam_sensor_plat_data {
	int facing;
	int orientation;
	int mirror;   //camera mirror
	//u16	gpio_vcc;	/* vcc enable gpio */   remove the gpio_vcc   , DO NOT use this pin for sensor power up ,cim will controls this
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int cap_wait_frame;   /* filter n frames when capture image */
};

struct ov3640_sensor {

	struct cim_sensor cs;
	struct i2c_client *client;
	//struct ov3640_platform_data *pdata;
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int mirror;   //camera mirror

	char name[8];

	unsigned int width;
	unsigned int height;
};



int ov3640_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val);
unsigned char ov3640_read_reg(struct i2c_client *client,unsigned short reg);

#endif
