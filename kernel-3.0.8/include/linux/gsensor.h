#ifndef __JZTSC_H__
#define __JZTSC_H__
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

__attribute__((weak)) struct gsensor_platform_data {
	int gpio_int;

	int poll_interval;
	int min_interval;
	int max_interval;

	u8 g_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	u8 ori_pr_swap;
	u8 ori_pith_negate;
	u8 ori_roll_negate;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
};

enum {
	GSENSOR_2G = 0,
	GSENSOR_4G,
	GSENSOR_8G,
	GSENSOR_16G,
};

static inline void sensor_swap_pr(u16 * p,u16 * r)
{
	u16 tmp = 0;
	tmp = *p;
	*p =  *r;
	*r = tmp;
}

#if 0
static inline int get_pin_status(struct jztsc_pin *pin)
{
	int val;

	if (pin->num < 0)
		return -1;
	val = gpio_get_value(pin->num);

	if (pin->enable_level == LOW_ENABLE)
		return !val;
	return val;
}

static inline void set_pin_status(struct jztsc_pin *pin, int enable)
{
	if (pin->num < 0)
		return;

	if (pin->enable_level == LOW_ENABLE)
		enable = !enable;
	gpio_set_value(pin->num, enable);
}
#endif
#endif
