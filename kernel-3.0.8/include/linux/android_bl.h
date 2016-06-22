/*
 * android bl no flash
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ANDROIDBL_H
#define _ANDROIDBL_H

struct android_bl_platform_data{
	unsigned int gpio_reset;
	int delay_reset;
	int delay_before_bkon;
	int bootloader_unblank;
	void (*notify_on)(int on);
};

#endif 
