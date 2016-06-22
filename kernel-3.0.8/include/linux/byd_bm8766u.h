/*
 * LCD driver data for BYD_BM8766U
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _BYD_BM8766U_H
#define _BYD_BM8766U_H

/**
 * @gpio_lcd_vsync: vertical sync input
 * @gpio_lcd_hsync: horizontal sync input
 * @gpio_lcd_disp: display on/off
 * @gpio_lcd_de: 1: select DE mode, 0: select SYNC mode
 */
struct platform_byd_bm8766u_data {
	unsigned int gpio_lcd_disp;
	unsigned int gpio_lcd_vsync;
	unsigned int gpio_lcd_hsync;
	unsigned int gpio_lcd_de;
};

#endif /* _BYD_BM8766U_H */

