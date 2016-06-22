/*
 * modem power manager driver data 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MODEM_PM_H__
#define __MODEM_PM_H__
#include <linux/gpio.h>
enum {
	CARDID_GSM_1 = 0,
	CARDID_GSM_2,
	CARDID_WCDMA_1,
	CARDID_WCDMA_2,
	CARDID_TDSCDMA_1,
	CARDID_TDSCDMA_2,
	CARDID_EVDO_1,
	CARDID_EVDO_2,
	CARDID_LTE_FDD,
	MAX_CARDS
};

struct modem_gpio {
	int gpio;
	int active_level;
};

struct modem_data {
	int id;
	char *name;
	struct modem_gpio bp_pwr;
	struct modem_gpio bp_onoff;
	struct modem_gpio bp_wake_ap;
	struct modem_gpio bp_status;
	struct modem_gpio ap_wake_bp;
	struct modem_gpio ap_status;
	struct modem_gpio bp_reset;
	struct modem_gpio sim_sw1;
	struct modem_gpio sim_sw2;
};

struct modem_ops {
	char *name;
	void (*init)(struct modem_data *bp);
	void (*poweron)(struct modem_data *bp);
	void (*poweroff)(struct modem_data *bp);
	void (*wakeup)(struct modem_data *bp);
	void (*suspend)(struct modem_data *bp);
	void (*resume)(struct modem_data *bp);
	void (*reset)(struct modem_data *bp);
};

struct modem_t {
	struct modem_data *bp_data;
	struct modem_ops *bp_ops;
};

struct modem_platform_data {
	int bp_nums;
	struct modem_data *bp;
};

static void inline modem_gpio_request(const struct modem_gpio *gpio, char *name)
{
	if (gpio->gpio) {
		gpio_request(gpio->gpio, name);
		gpio_direction_output(gpio->gpio, !gpio->active_level);
	}
}

static void inline modem_gpio_out(const struct modem_gpio *gpio, int active)
{
	if (gpio->gpio && active)
		gpio_direction_output(gpio->gpio, gpio->active_level);
	if (gpio->gpio && !active)
		gpio_direction_output(gpio->gpio, !gpio->active_level);
}

extern int modem_register_ops(struct modem_ops *ops);

#define BP_ACTIVE		1
#define BP_DEACTIVE		0

#define POWER_ON                _IOW(0xF5, 0, unsigned long)
#define RESET                   _IOW(0xF5, 1, unsigned long)
#define SIM1_SWITCH		_IOW(0xF5, 2, unsigned long)
#define SIM2_SWITCH		_IOW(0xF5, 3, unsigned long)
#define POWER_OFF               _IOW(0xF5, 4, unsigned long)
#define WAKEUP                  _IOW(0xF5, 5, unsigned long)
#define SYS_RESET               _IOW(0xF5, 6, unsigned long)
#define ENABLE_HOST_PHY         _IOW(0xF5, 7, unsigned long)
#define DISABLE_HOST_PHY        _IOW(0xF5, 8, unsigned long)

#endif

