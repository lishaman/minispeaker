/*
 * LCD driver data for TM080TDH01
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TM080TDH01_H
#define _TM080TDH01_H

/**
 * @gpio_rest: global reset pin, active low to enter reset state
 */
struct platform_tm080tdh01_data {
	unsigned int gpio_rest;
};

#endif /* _TM080TDH01_H */
