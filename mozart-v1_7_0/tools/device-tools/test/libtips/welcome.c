#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include "mozart_config.h"
#include "tips_interface.h"
#include "lcd_interface.h"
#define MSEC	200

int main(int argc, char **argv)
{
	char keyword [] = "welcome";
	int count;
	int ret;
	mozart_led_turn_off(LED_WIFI);
	mozart_led_turn_slow_flash(LED_RECORD);

	count = 3 * 1000 / MSEC; /* wait 3 seconds */
#if (SUPPORT_LCD == 1)
	mozart_lcd_display(NULL,SYSTME_BOOTING_CN);
	system("echo 120 > /sys/devices/platform/pwm-backlight.0/backlight/pwm-backlight.0/brightness");
#endif
	while (count > 0) {
		ret = mozart_play_key(keyword);
		if (!ret)
			break;
		usleep(MSEC * 1000);
		count--;
	}

	if (count <= 0)
		printf("%s. Playing keyword \"%s\" connect timeout.\n", basename(argv[0]), keyword);

	return 0;
}
