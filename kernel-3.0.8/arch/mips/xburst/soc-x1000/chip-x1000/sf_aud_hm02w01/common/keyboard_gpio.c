#include <linux/platform_device.h>

#include <linux/input.h>
#include <mach/jzgpio_keys.h>
#include <linux/input/matrix_keypad.h>
#include "board_base.h"

#ifdef CONFIG_KEYBOARD_MATRIX

#define KEY_R0 GPIO_PC(6)
#define KEY_R1 GPIO_PC(9)
#define KEY_R2 GPIO_PC(17)

#define KEY_C0 GPIO_PC(16)
#define KEY_C1 GPIO_PC(19)
#define KEY_C2 GPIO_PC(18)

static const unsigned int matrix_keypad_cols[] = {KEY_C0,KEY_C1,KEY_C2};
static const unsigned int matrix_keypad_rows[] = {KEY_R0,KEY_R1,KEY_R2};
static uint32_t canna_keymap[] =
{
	//KEY(row, col, keycode)
	KEY(0,0, KEY_PLAYPAUSE),	/* PLAY/STOP */
	KEY(0,1, KEY_PREVIOUSSONG),	/* PREVIOUS SONG */
	KEY(0,2, KEY_NEXTSONG),		/* NEXT SONG */
	KEY(1,0, KEY_CONFIG),
	KEY(1,1, KEY_VOLUMEDOWN),
	KEY(1,2, KEY_VOLUMEUP),
	KEY(2,0, KEY_RESERVED),
	KEY(2,1, KEY_RESERVED),
	KEY(2,2, KEY_MODE),		/* WIFI/AUX/USB/SD CARD */
};

static struct matrix_keymap_data canna_keymap_data =
{
	.keymap = canna_keymap,
	.keymap_size =ARRAY_SIZE(canna_keymap),
};

struct matrix_keypad_platform_data canna_keypad_data  =
{
	.keymap_data = &canna_keymap_data,
	.col_gpios = matrix_keypad_cols,
	.row_gpios = matrix_keypad_rows,
	.num_col_gpios = ARRAY_SIZE(matrix_keypad_cols),
	.num_row_gpios = ARRAY_SIZE(matrix_keypad_rows),
	.col_scan_delay_us = 10,
	.debounce_ms =100,
	.wakeup  = 1,
	.active_low  = 1,
	.no_autorepeat = 1,
};

struct platform_device jz_matrix_kdb_device = {
	.name		= "matrix-keypad",
	.id		= 0,
	.dev		= {
		.platform_data  = &canna_keypad_data,
	}
};
#endif /* CONFIG_KEYBOARD_MATRIX */

#ifdef CONFIG_KEYBOARD_JZGPIO
static struct jz_gpio_keys_button board_longbuttons[] = {
#ifdef GPIO_POWERDOWN
        {
                .gpio                           = GPIO_POWERDOWN,
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
#ifdef GPIO_BOOT_SEL1
	{
                .gpio                           = GPIO_BOOT_SEL1,
                .code = {
                        .shortpress_code        = KEY_F5,
                        .longpress_code         = KEY_RESERVED,
                },
                .desc                           = "music Shortcut key 3",
                .active_low                     = ACTIVE_LOW_F5,
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
