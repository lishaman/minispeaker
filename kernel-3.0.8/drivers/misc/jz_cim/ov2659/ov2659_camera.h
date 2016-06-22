#ifndef __ov2659_CAMERA_H__
#define __ov2659_CAMERA_H__

struct ov2659_platform_data {
	int facing_f;
	int orientation_f;
	int mirror_f;

	int facing_b;
	int orientation_b;
	int mirror_b;

	uint16_t gpio_rst;

	uint16_t gpio_en_f;
	uint16_t gpio_en_b;
};

struct ov2659_sensor {

	struct cim_sensor cs;
	struct i2c_client *client;
	//struct ov2659_platform_data *pdata;
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* front camera enable gpio */
	int mirror;   //camera mirror

	char name[8];

	unsigned int width;
	unsigned int height;
};



int ov2659_write_reg(struct i2c_client *client,unsigned short reg, unsigned char val);
unsigned char ov2659_read_reg(struct i2c_client *client,unsigned short reg);

#endif

