/*
 * LCD driver data for KD50G2_40NM_A2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _KD50G2_40NM_A2_H
#define _KD50G2_40NM_A2_H

/**
 * @gpio_lcd_vsync: vertical sync input
 * @gpio_lcd_hsync: horizontal sync input
 * @gpio_lcd_disp: display on/off
 * @gpio_lcd_de: 1: select DE mode, 0: select SYNC mode
 */
struct platform_kd50g2_40nm_a2_data {
	unsigned int gpio_lcd_disp;
	unsigned int gpio_lcd_vsync;
	unsigned int gpio_lcd_hsync;
	unsigned int gpio_lcd_de;
};

#endif /* _KD50G2_40NM_A2_H */

