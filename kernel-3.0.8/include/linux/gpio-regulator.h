
#ifndef __INCLUDE_GPIO_REGULATOR_H__
#define __INCLUDE_GPIO_REGULATOR_H__

struct gpio_pwc_data {
	char *name;
	int flags;
	int gpio;
	char *parent;
};

struct gpio_pwc_platform_data {
	struct gpio_pwc_data *data;
	int count;
};

/*
static struct gpio_pwc_data pwcd[] = {
	{
		.name = "ldo1",
		.flags = ACTIVE_LOW,
		.gpio = GPA30,
		.parent = NULL,
	},
	{
		.name = "ldo2",
		.flags = 0,
		.gpio = GPA31,
		.parent = "ldo1",
	},
	{
		.name = "ldo3",
		.flags = 0,
		.gpio = GPB10,
		.parent = NULL,
	},
};

static struct gpio_pwc_platform_data gpio_pwc_pdata = {
	.data = pwcd,
	.count = ARRAY_SIZE(pwcd),
};

static struct platform_device gpio_pwc_device = {
	.name = "gpio_pwc",
	.dev = {
		.platform_data		= &gpio_pwc_pdata,
	},
};

*/
#endif
