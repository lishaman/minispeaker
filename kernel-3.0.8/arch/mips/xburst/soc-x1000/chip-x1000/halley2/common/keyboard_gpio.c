#include <linux/platform_device.h>
#include <linux/input.h>
#include <mach/jzgpio_keys.h>
#include "board_base.h"

#ifdef CONFIG_KEYBOARD_JZGPIO
static struct jz_gpio_keys_button board_longbuttons[] = {
#ifdef GPIO_WKUP
        {
                .gpio                           = GPIO_WKUP,
                .code = {
                        .shortpress_code        = KEY_SLEEP,
                        .longpress_code         = KEY_POWER,
                },
                .desc                           = "power down & wakeup",
                .active_low                     = ACTIVE_LOW_POWERDOWN,
                .longpress_interval             = 1800,
		.wakeup                         = 1,
                .debounce_interval              = 2,
        },
#endif
#ifdef GPIO_BOOT_SEL0
	{
                .gpio                           = GPIO_BOOT_SEL0,
                .code = {
                        .shortpress_code        = KEY_F4,
                        .longpress_code         = KEY_RESERVED,
                },
                .desc                           = "music Shortcut key 2",
                .active_low                     = ACTIVE_LOW_F4,
                .longpress_interval             = 0,
                .debounce_interval              = 2,
        },
#endif
#ifdef GPIO_MACTXEN
	{
                .gpio                           = GPIO_MACTXEN,
                .code = {
                        .shortpress_code        = KEY_MODE,
                        .longpress_code         = KEY_RESERVED,
                },
                .desc                           = "music Shortcut key 3",
                .active_low                     = ACTIVE_LOW_POWERDOWN,
                .longpress_interval             = 0,
                .debounce_interval              = 2,
        },

#endif
#ifdef GPIO_MACMDC
	{
                .gpio                           = GPIO_MACMDC,
                .code = {
                        .shortpress_code        = KEY_F1,
                        .longpress_code         = KEY_RESERVED,
                },
                .desc                           = "music Shortcut key 3",
                .active_low                     = ACTIVE_LOW_POWERDOWN,
                .longpress_interval             = 0,
                .debounce_interval              = 2,
        },

#endif
};

static struct jz_gpio_keys_platform_data board_longbutton_data = {
        .buttons        = board_longbuttons,
        .nbuttons       = ARRAY_SIZE(board_longbuttons),
};

struct platform_device jz_longbutton_device = {
        .name           = "jz-gpio-keys",
        .id             = -1,
        .num_resources  = 0,
        .dev            = {
                .platform_data  = &board_longbutton_data,
        }
};
#endif /* CONFIG_KEYBOARD_JZGPIO */
