#ifndef adv7180_SET_H
#define adv7180_SET_H

#include<linux/i2c.h>

#define ADV7180_CTRL_BRIGHTNESS	0x0
#define ADV7180_CTRL_CONTRAST	0x1
#define ADV7180_CTRL_SATURATION	0x2
#define ADV7180_CTRL_HUE	0x3

/* supported controls */
struct adv7180_ctrl {
	__u32		     id;
	__s32		     minimum;	/* Note signedness */
	__s32		     maximum;
};

struct adv7180_control {
	__u32		     id;
	__s32		     value;
};

enum input_mode{
	CVBS,
	SVIDEO,
	YPbPr,
};

struct adv7180 {
	unsigned char reg[128];
	int norm;
	enum input_mode input;
	int enable;
};


static const unsigned char init_composite[] = {
	0x00, 0x04,
	0x04, 0xD7,
	0x17, 0x41,
	0x31, 0x02,
	0x37, 0x09,
	0x3D, 0xA2,
	0x3E, 0x6A,
	0x3F, 0xA0,
	0x0E, 0x80,
	0x55, 0x81,
	0x0E, 0x00,
};

static const unsigned char init_component[] = {

};

static const unsigned char init_svideo[] = {

};



void preview_set(struct i2c_client *client);
void capture_set(struct i2c_client *client);
void size_switch(struct i2c_client *client,int width,int height);

int adv7180_probe(struct i2c_client *c);
int adv7180_remove(struct i2c_client *c);
int adv7180_sensor_reset(struct i2c_client *c);

int adv7180_read(struct i2c_client *client, char *writebuf,
			int writelen, char *readbuf, int readlen);
int adv7180_write(struct i2c_client *client, char *writebuf, int writelen);
int adv7180_set_reg(struct i2c_client *client, u8 addr, u8 para);

int adv7180_log_status(struct i2c_client *c);

#endif

