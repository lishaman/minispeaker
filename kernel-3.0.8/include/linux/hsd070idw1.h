/*
 * LCD driver data for HSD070IDW1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _HSD070IDW1_H
#define _HSD070IDW1_H

/**
 * @gpio_rest: global reset pin, active low to enter reset state
 */
struct platform_hsd070idw1_data {
	unsigned int gpio_rest;
    void (*notify_on)(int on);
};

#endif /* _HSD070IDW1_H */
