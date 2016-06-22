#ifndef _CTP_IT7260_H
#define _CTP_IT7260_H

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>

#define IT7260_NAME	"ctp_it7260"

struct ctp_it7260_gpio {
	struct jztsc_pin *irq;
	struct jztsc_pin *power;
};

struct Ctp_it7260_data {
	char  status;
	char  curr_tate;
	struct input_dev *input_dev;
	struct input_dev 	*input_dev1; 
	struct i2c_client *client;
	struct work_struct work;
	struct workqueue_struct *ctp_workq;
	struct early_suspend    early_suspend;
	u32 temp_x[2];
	u32 temp_y[2];
	struct ctp_it7260_gpio gpio;
};

struct ctp_it7260_platform_data {
	u16	intr;
    u16 power;
	u16	width;
	u16	height;
	u8	is_i2c_gpio;
};
//CTP_ERR_CODE
#define	ERR_SUCCESS   0x0
#define	ERR_BAD_CMD	0x1
#define	ERR_BAD_SUBCMD	0x2
#define	ERR_SIZE	0x3
#define	ERR_BAD_DATATYPE 	  0x4
#define	ERR_BOUNDARY      0x5
#define	ERR_BOUND_PARAMETER      0x6
#define	ERR_INVALIDKEY     0x10
#define	ERR_BAD_MODE    0x11
#define	ERR_BAD_OFFSET    0x12
#define	ERR_FLASHSIZE    0x13
#define	ERR_BAD_ID    0x14
#define	ERR_BOOTLOADER    0x15
#define	ERR_UNKNOWN    0xff



#endif
