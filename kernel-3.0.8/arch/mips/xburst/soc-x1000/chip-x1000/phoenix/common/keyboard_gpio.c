#include <linux/platform_device.h>

#include <linux/gpio_keys.h>
#include <mach/jzgpio_keys.h>
#include <linux/input.h>
#include "board_base.h"

#ifdef CONFIG_KEYBOARD_GPIO
struct gpio_keys_button __attribute__((weak)) board_buttons[] = {
#ifdef GPIO_HOME_KEY
	{
		.gpio		= GPIO_HOME_KEY,
		.code   	= KEY_HOME,
		.desc		= "home key",
		.active_low	= ACTIVE_LOW_HOME,
	},
#endif
#ifdef GPIO_MENU_KEY
	{
		.gpio		= GPIO_MENU_KEY,
		.code   	= KEY_MENU,
		.desc		= "menu key",
		.active_low	= ACTIVE_LOW_MENU,
	},
#endif
#ifdef GPIO_BACK_KEY
	{
		.gpio		= GPIO_BACK_KEY,
		.code   	= KEY_BACK,
		.desc		= "back key",
		.active_low	= ACTIVE_LOW_BACK,
	},
#endif
#ifdef GPIO_VOLUMEDOWN_KEY
	{
		.gpio		= GPIO_VOLUMEDOWN_KEY,
		.code   	= KEY_VOLUMEDOWN,
		.desc		= "volum down key",
		.active_low	= ACTIVE_LOW_VOLUMEDOWN,
	},
#endif
#ifdef GPIO_VOLUMEUP_KEY
	{
		.gpio		= GPIO_VOLUMEUP_KEY,
		.code   	= KEY_VOLUMEUP,
		.desc		= "volum up key",
		.active_low	= ACTIVE_LOW_VOLUMEUP,
	},
#endif

#ifdef GPIO_ENDCALL_KEY
	{
		.gpio           = GPIO_ENDCALL_KEY,
		.code           = KEY_POWER,
		.desc           = "end call key",
		.active_low     = ACTIVE_LOW_ENDCALL,
		.wakeup         = 1,
	},
#endif
};

static struct gpio_keys_platform_data board_button_data = {
	.buttons	= board_buttons,
	.nbuttons	= ARRAY_SIZE(board_buttons),
};

struct platform_device jz_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
                .platform_data	= &board_button_data,
	}
};
#endif	/* CONFIG_KEYBOARD_GPIO */

/* Support Long/Short Button */
#ifdef CONFIG_KEYBOARD_JZGPIO
#ifdef CONFIG_KEYBOARD_GPIO
#error "KEYBOARD_JZGPIO and KEYBOARD_GPIO Can NOT used at sametime."
#endif
static struct jz_gpio_keys_button board_longbuttons[] = {
#ifdef GPIO_HOME_KEY
	{
		.gpio			= GPIO_HOME_KEY,
		.code = {
			.shortpress_code	= KEY_PLAYPAUSE,
			.longpress_code		= KEY_RECORD,
		},
		.desc			= "home key",
		.active_low		= ACTIVE_LOW_HOME,
		.longpress_interval	= 1800,
		.debounce_interval	= 2,
	},
#endif
#ifdef GPIO_ENDCALL_KEY
	{
		.gpio			= GPIO_ENDCALL_KEY,
		.code = {
			.shortpress_code	= KEY_POWER,
			.longpress_code		= KEY_POWER,
		},
		.desc			= "end call key",
		.active_low		= ACTIVE_LOW_ENDCALL,
		.longpress_interval	= 0,
		.wakeup			= 1,
		.debounce_interval	= 2,
	},
#endif
};
static struct jz_gpio_keys_platform_data board_longbutton_data = {
	.buttons	= board_longbuttons,
	.nbuttons	= ARRAY_SIZE(board_longbuttons),
};

struct platform_device jz_longbutton_device = {
	.name		= "jz-gpio-keys",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &board_longbutton_data,
	}
};
#endif	/* CONFIG_KEYBOARD_JZGPIO */
