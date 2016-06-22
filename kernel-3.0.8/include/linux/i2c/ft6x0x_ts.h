#ifndef __LINUX_FT6X0X_TS_H__
#define __LINUX_FT6X0X_TS_H__
#include <linux/tsc.h>
struct ft6x0x_platform_data {
	struct jztsc_pin	*gpio;
	unsigned int		x_max;
	unsigned int		y_max;
	unsigned int            fw_ver;

	char *power_name;
	int wakeup;

	int (*power_init)(struct device *dev);
	int (*power_on)(struct device *dev);
	int (*power_off)(struct device *dev);

	void		*private;
};
#endif
