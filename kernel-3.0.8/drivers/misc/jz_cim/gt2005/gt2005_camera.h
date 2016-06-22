#ifndef __gt2005_CAMERA_H__
#define __gt2005_CAMERA_H__
#if 1
struct gt2005_platform_data {
	int facing;
	int orientation;
	int mirror;   //camera mirror
	//u16	gpio_vcc;	/* vcc enable gpio */   remove the gpio_vcc   , DO NOT use this pin for sensor power up ,cim will controls this
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int cap_wait_frame;
};
#else
struct gt2005_platform_data {
	int facing_f;
	int orientation_f;
	int mirror_f;

	int facing_b;
	int orientation_b;
	int mirror_b;

	uint16_t gpio_rst;

	uint16_t gpio_en_f;
	uint16_t gpio_en_b;
	int cap_wait_frame;    /* filter n frames when capture image */
};
#endif
struct gt2005_sensor {

	struct cim_sensor cs;
	struct i2c_client *client;
	//struct gt2005_platform_data *pdata;
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* camera enable gpio */
	int mirror;   //camera mirror

	char name[8];

	unsigned int width;
	unsigned int height;
};



int gt2005_write_reg16(struct i2c_client *client,unsigned short reg, unsigned char val);
unsigned char gt2005_read_reg16(struct i2c_client *client,unsigned short  reg);

#endif

