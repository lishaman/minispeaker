
#ifndef _LINUX_NOVATEK_TS_H
#define _LINUX_NOVATEK_TS_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

/*
#define SCREEN_MAX_HEIGHT 768  //800  // 480
#define SCREEN_MAX_WIDTH  1024 //600  // 640
#define TOOL_PRESSURE     100
#define TOUCH_MAX_HEIGHT  768 //1344	//1024  //480
#define TOUCH_MAX_WIDTH	  1024 //1856	//1536 //640
*/

#define SCREEN_MAX_HEIGHT 768  // 480
#define SCREEN_MAX_WIDTH  1024  // 640
#define TOOL_PRESSURE     100
#define TOUCH_MAX_HEIGHT  672 //1856	//1024  //480
#define TOUCH_MAX_WIDTH	  928 //1344	//1536 //640


#define READ_COOR_ADDR    0x00
#define IIC_BYTENUM       6
#define MAX_FINGER_NUM    5 
#define NOVATEK_I2C_NAME  "nt11003-ts"
#define TOOL_PRESSURE     100
#define POLL_TIME         10
static const char *novatek_ts_name = "nt11003-ts";
static struct workqueue_struct *novatek_wq;

struct novatek_platform_data {
	struct jztsc_pin	*gpio;
	unsigned int		x_max;
	unsigned int		y_max;

	char *power_name;
	int wakeup;

	void		*private;
};

#endif


