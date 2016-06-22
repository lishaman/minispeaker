#ifndef __LINUX_SS253X_TS_H__
#define __LINUX_SS253X_TS_H__

#define PROCFS_NAME "ssd253x-mode"
#define SSD253X_I2C_NAME "ssd253x-ts"

struct ssd253x_ts_platform_data{
	u16	x_res;
	u16	y_res;
	u16	pressure_min;
	u16 pressure_max;
	u16	reset;		/* cy8c reset pin */
	u16	intr;		/* irq number	*/
	u16	wake;           /* wake gpio	*/
	u16 power;
};

/****** define your button codes ******/
//#define HAS_BUTTON
//#define SIMULATED_BUTTON

#define BUTTON0	59	//menu
#define BUTTON1	158	//back
#define BUTTON2	217	//search
#define BUTTON3	102	//home

typedef struct{
	unsigned long x;	//center x of button
	unsigned long y;	//center y of button
	unsigned long dx;	//x range (x-dx)~(x+dx)
	unsigned long dy;	//y range (y-dy)~(y+dy)
	unsigned long code;	//key code for simulated key
}SKey_Info,*pSKey_Info;

#define REG_DEVICE_ID		    0x02
#define FINGER_STATUS           0x79
#define FINGER_DATA             0x7C
#define BUTTON_RAW				0xB5
#define BUTTON_STATUS		   	0xB9

#endif
