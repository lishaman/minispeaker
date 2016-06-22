#ifndef __OV5640_CAMERA_H__
#define __OV5640_CAMERA_H__

#define MODULE_VENDOR_TRULY		0x55	// 信利
#define MODULE_VENDOR_GLOBALOPTICS	0xaa	// 光阵

struct ov5640_platform_data {
    int facing;
    int orientation;
	int mirror;   //camera mirror
	//u16	gpio_vcc;	/* vcc enable gpio */   remove the gpio_vcc   , DO NOT use this pin for sensor power up ,cim will controls this 
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* sensor enable gpio */
	uint16_t	gpio_pwdn;	/* sensor powerdown gpio */
	int cap_wait_frame;   /* filter n frames when capture image */
};

struct ov5640_sensor {
	
	struct cim_sensor cs;
	struct i2c_client *client;
	//struct ov5640_platform_data *pdata;
	uint16_t	gpio_rst;	/* resert  gpio */
	uint16_t	gpio_en;	/* sensor enable gpio */
	uint16_t	gpio_pwdn;	/* sensor powerdown gpio */
	int mirror;   //camera mirror

	char name[8];

	unsigned int width;
	unsigned int height;

	struct workqueue_struct *wq;
	struct delayed_work work;
	struct timer_list timer;

	int fix_ae;			// flag
	int download_af_fw;		// flag
	bool first_use;			//flag
};

#endif

