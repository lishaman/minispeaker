/*
 * LCD driver data for HSD101PWW1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LP101WX1_SLN2_H
#define _LP101WX1_SLN2_H

/**
 * @gpio_rest: global reset pin, active low to enter reset state
 */
struct platform_lp101wx1_sln2_data {
	unsigned int gpio_rest;
};

#endif /* _HSD101PWW1_H */
