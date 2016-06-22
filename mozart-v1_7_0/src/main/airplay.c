#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "utils_interface.h"
#include "sharememory_interface.h"

static char apname[64] = {};
static char ao_type[32] = {};
static char *cmdline[32] = {};

void mozart_airplay_shutdown(void)
{
	system("killall shairport");
}

void mozart_airplay_start_service(void)
{
	int i = 0;
#define CMDLINE_DEBUG 0
#if CMDLINE_DEBUG
	while (cmdline[i++])
		printf("argv[%d] is %s\n", i - 1, cmdline[i - 1]);
#endif

#if 0
	if (0 == fork())
		execvp(cmdline[0], cmdline);
#else
	char cmd[128] = {};
	i = 0;
	while (cmdline[i]) {
		strcat(cmd, cmdline[i]);
		strcat(cmd, " ");
		i++;
	}

#if CMDLINE_DEBUG
	printf("shairport cmd is `%s`\n", cmd);
#endif

	mozart_system(cmd);
#endif

	return;
}

int mozart_airplay_stop_playback(void)
{
	int cnt = 50;
	if(share_mem_set(AIRPLAY_DOMAIN, STATUS_STOPPING)){
		printf("share_mem_set failure.\n");
		return -1;
	}

	if(AUDIO_OSS == get_audio_type()){
		while (cnt--) {
			if (check_dsp_opt(O_WRONLY))
				break;
			usleep(100 * 1000);
		}

		if (cnt < 0) {
			printf("\n\n\ncheck dsp O_WRONLY for %.2f seconds in %s:%s:%d!!!!!!!!\n\n\n",
					(float)(100*1000*cnt/(1000 * 1000)), __FILE__, __func__, __LINE__);
			return -1;
		}
	}

	printf("stop airplay playback will be done in shairport really!!\n");

	return 0;
}

int mozart_airplay_init(void)
{
	int i = 0;
	int argc = 0;
	char mac[] = "00:11:22:33:44:55";
	char dev_playback[12] = "";

        // Parse playback dsp device.


	if(AUDIO_ALSA == get_audio_type()){
		strcpy(ao_type, "alsa");
	} else {
		strcpy(ao_type, "oss");
		strcat(ao_type, ":");
		if (mozart_ini_getkey("/usr/data/system.ini", "audio", "dev_playback", dev_playback)) {
			printf("[player/libaudio - OSS] Can't get record dsp device, force to /dev/dsp.\n");
			strcat(ao_type, "/dev/dsp");
		} else {
			strcat(ao_type, dev_playback);
		}
	}

    char *argv[] = {
		"shairport",
		"-o",
		ao_type,
		"-a",
		apname,
#if 0
		"-e",
		"/var/log/shairport.errlog",
#endif
#if 0 // soft volume set
		"-v",
		"soft",
#endif
		"-d",
		NULL
	};

    memset(mac, 0, sizeof(mac));
	memset(apname, 0, sizeof(apname));
	get_mac_addr("wlan0", mac, "");

	strcat(apname, "SmartAudio-");
	strcat(apname, mac+4);

	argc = sizeof(argv)/sizeof(argv[0]);

	for (i = 0; i < argc; i++)
		cmdline[i] = argv[i];

	cmdline[0] = "shairport";
	cmdline[argc] = NULL;

	return 0;
}
