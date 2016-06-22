#ifndef __JZGPIO_KEYS_H__
#define __JZGPIO_KEYS_H__

struct jz_gpio_code {
	unsigned int shortpress_code;
	unsigned int longpress_code;
};

struct jz_gpio_keys_button {
	/* Configuration parameters */
	struct jz_gpio_code code; /* input event code (KEY_*, SW_*) */
	int gpio;
	int active_low;
	const char *desc;
	unsigned int type;	/* input event type (EV_KEY, EV_SW, EV_ABS) */
	int wakeup;		/* configure the button as a wake-up source */
	int debounce_interval;	/* debounce ticks interval in msecs */
	int lock_interval;	/* pause for a moment when the key is pressed */
	int longpress_interval;	/* long press key code interval in msecs. No long/short check by it is 0 */
	bool can_disable;
	int value;		/* axis value for EV_ABS */
};

struct jz_gpio_keys_platform_data {
	struct jz_gpio_keys_button *buttons;
	int nbuttons;
	unsigned int poll_interval;	/* polling interval in msecs -
					   for polling driver only */
	unsigned int rep:1;		/* enable input subsystem auto repeat */
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	const char *name;		/* input device name */
};


#endif /* __JZGPIO_KEYS_H__ */
