#ifndef CT36X_TS_H
#define CT36X_TS_H

#include <linux/gpio.h>

//-----------------------------------------------------
// Constants
//-----------------------------------------------------
#define CT360_CHIP_VER 0x02	// CT360
#define CT36X_CHIP_VER 0x01	// ct363, ct365

#define CT360_FW_VER_OFFSET 16372	//
#define CT360_FW_SIZE 16384	//

#define CT36X_FW_VER_OFFSET	32756	//
#define CT36X_FW_SIZE 32768	//

//-----------------------------------------------------
// Options
//-----------------------------------------------------
#define CT36X_TS_DEBUG 0	// 1

#define CT36X_TS_CHIP_SEL CT360_CHIP_VER	// CT360_CHIP_VER //fix

#define CT36X_TS_POINT_NUM 10    // max touch points supported

//#define CT36X_TS_X_REVERSE	1

#ifdef CONFIG_BOARD_JI8070A
#define CT36X_TS_X_REVERSE	0
#else
#define CT36X_TS_X_REVERSE  1
#endif

#if (defined(CONFIG_BOARD_Q8) || defined(CONFIG_BOARD_JI8070A) || defined(CONFIG_BOARD_LEAF))
	#define CT36X_TS_Y_REVERSE	0
#else
	#define CT36X_TS_Y_REVERSE  1
#endif

#define CT36X_TS_XY_SWAP	0 //fix
#define CT36X_TS_PTS_VER	1// fix	// Touch Point protocol

#if (CT36X_TS_CHIP_SEL == CT360_CHIP_VER)
#define CT36X_TS_FW_VER_OFFSET			CT360_FW_VER_OFFSET
#define CT36X_TS_FW_SIZE				CT360_FW_SIZE
#elif (CT36X_TS_CHIP_SEL == CT36X_CHIP_VER)
#define CT36X_TS_FW_VER_OFFSET			CT36X_FW_VER_OFFSET
#define CT36X_TS_FW_SIZE				CT36X_FW_SIZE
#endif

#ifdef CONFIG_CT36X_FWUPDATE
	#define CT36X_TS_FW_UPDATE 1			// BOOT LOADER //fix
#else
	#define CT36X_TS_FW_UPDATE 0			// BOOT LOADER
#endif

#define CT36X_TS_I2C_SPEED			(200*1024)	// 200KHz

#define CT36X_TS_HAS_EARLYSUSPEND			1

#define CT36X_TS_ESD_TIMER_INTERVAL			0	// Sec

#if (CT36X_TS_CHIP_SEL == CT360_CHIP_VER)
struct ct36x_finger_info {
#if (!CT36X_TS_PTS_VER)
	unsigned char status : 4; 	// Action information, 1: Down; 2: Move; 3: Up
	unsigned char id : 4; 		// ID information, from 1 to CFG_MAX_POINT_NUM
#endif
	unsigned char xhi;			// X coordinate Hi
	unsigned char yhi;			// Y coordinate Hi
	unsigned char ylo : 4;		// Y coordinate Lo
	unsigned char xlo : 4;		// X coordinate Lo
#if (CT36X_TS_PTS_VER)
	unsigned char status : 4;	// Action information, 1: Down; 2: Move; 3: Up
	unsigned char id : 4;		// ID information, from 1 to CFG_MAX_POINT_NUM
#endif
};
#elif (CT36X_TS_CHIP_SEL == CT36X_CHIP_VER)
struct ct36x_finger_info {
#if (!CT36X_TS_PTS_VER)
	unsigned char status : 3;	// Action information, 1: Down; 2: Move; 3: Up
	unsigned char id : 5;		// ID information, from 1 to CFG_MAX_POINT_NUM
#endif
	unsigned char xhi;			// X coordinate Hi
	unsigned char yhi;			// Y coordinate Hi
	unsigned char ylo : 4;		// Y coordinate Lo
	unsigned char xlo : 4;		// X coordinate Lo
#if (CT36X_TS_PTS_VER)
	unsigned char status : 3;	// Action information, 1: Down; 2: Move; 3: Up
	unsigned char id : 5;		// ID information, from 1 to CFG_MAX_POINT_NUM
#endif
	unsigned char area;			// Touch area
	unsigned char pressure;		// Touch Pressure
};
#endif

union ct36x_i2c_data {
	struct ct36x_finger_info pts[CT36X_TS_POINT_NUM];
	unsigned char buf[CT36X_TS_POINT_NUM * sizeof(struct ct36x_finger_info)];
};

struct ct36x_ts_info {
	// Chip
	int	chip_id;
	// Communication settings
	int	spi_bus;
	int	i2c_bus;
	unsigned short i2c_address;
	struct i2c_client *client;
	// Devices
	struct input_dev *input;
	int irq;
	int rst;
	int ss;
	int	ready;
	// Early suspend
#ifdef CONFIG_HAS_EARLYSUSPEND
#if	(CT36X_TS_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#endif
	// Work thread settings
	struct work_struct event_work;
	// ESD 
#if (CT36X_TS_ESD_TIMER_INTERVAL)
	struct timer_list timer;
	int	timer_on;
#endif
	// touch event data
	union ct36x_i2c_data data;
	int	press;
	int	release;
	struct regulator *power;
	int x_max;
	int y_max;

	struct workqueue_struct *i2c_workqueue;
	struct input_member member;
};

#endif
