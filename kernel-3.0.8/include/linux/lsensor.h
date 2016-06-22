#ifndef __JZ_LSENSOR_H__
#define __JZ_LSENSOR_H__
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

__attribute__((weak)) struct lsensor_platform_data {
	int gpio_int;
};

#if 0
static inline void sensor_swap_pr(u16 * p,u16 * r)
{
	u16 tmp = 0;
	tmp = *p;
	*p =  *r;
	*r = tmp;
}

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
