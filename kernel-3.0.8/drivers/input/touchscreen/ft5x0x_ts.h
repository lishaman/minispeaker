#ifndef __LINUX_FT5X0X_TS_H__
#define __LINUX_FT5X0X_TS_H__

#define SCREEN_MAX_X    800
#define SCREEN_MAX_Y    480
#define PRESS_MAX       255
#define FT5X0X_NAME	"ft5x0x_ts"

struct ft5x0x_ts_platform_data{
	u16	x_res;
	u16	y_res;
	u16	pressure_min;
	u16 	pressure_max;
	u16	reset;		/* cy8c reset pin */
	u16	intr;		/* irq number	*/
	u16 	wake;           /* wake gpio	*/
};


enum ft5x0x_ts_regs {
	FT5X0X_REG_PMODE	= 0xA5,	/* Power Consume Mode		*/	
};

//FT5X0X_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03
#endif
