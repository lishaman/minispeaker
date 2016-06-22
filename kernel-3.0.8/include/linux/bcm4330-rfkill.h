/*
 * Bluetooth bcm4330 rfkill power control via GPIO
 * Create by Ingenic cljiang <cljiang@ingenic.cn>
 *
 */

#ifndef _LINUX_BCM4330_RFKILL_H
#define _LINUX_BCM4330_RFKILL_H

#include <linux/rfkill.h>



struct bcm4330_gpio {
	int bt_rst_n;
	int bt_reg_on;
	int bt_wake;
	int bt_int;
};



struct bcm4330_rfkill_platform_data {
	struct bcm4330_gpio gpio;

	struct rfkill *rfkill;  /* for driver only */
};

#endif
