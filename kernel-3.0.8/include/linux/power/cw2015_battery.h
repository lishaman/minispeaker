 /************************************************************************
 * Gas_Gauge driver for CW2015/2013
 * Copyright (C) 2012, CellWise
 * Update time 2014.08.21
 *
 * Authors: ChenGang <ben.chen@cellwise-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.And this driver depends on
 * I2C and uses IIC bus for communication with the host.
 *
 *************************************************************************/

#ifndef CW2015_BATTERY_H
#define CW2015_BATTERY_H

#define SIZE_BATINFO    64
#define INVALID_GPIO    0x00
#define GPIO_LOW		0
#define GPIO_HIGH		1

struct cw_bat_platform_data {
        int is_dc_charge;
        int is_usb_charge;
        int bat_low_pin;
        int bat_low_level;
        int chg_ok_pin;
        int chg_ok_level;
		int usb_dete_pin;
		int usb_dete_level;
        unsigned char *cw_bat_config_info;
};

#endif
