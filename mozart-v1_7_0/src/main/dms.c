#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "utils_interface.h"
#include "ini_interface.h"

int start_dms(void)
{
    char ushare_cmd[] = "ushare -n XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX -i wlan0 -p 49200 -t -o -v -D";
    char mac[] = "00:11:22:33:44:55";
    char dms_name[64] = {};

	memset(mac, 0, sizeof(mac));
	memset(dms_name, 0, sizeof(dms_name));
	get_mac_addr("wlan0", mac, "");

	strcat(dms_name, "SmartAudio-");
	strcat(dms_name, mac+4);

    sprintf(ushare_cmd, "ushare -n %s -i wlan0 -p 49200 -t -o -v -D", dms_name);

    if (!mozart_system(ushare_cmd))
        return 0;

    return -1;
}

int stop_dms(void)
{
    pid_t pid = -1;
    char buf[] = "111111";

    if (mozart_ini_getkey("/var/run/ushare.info", "ushare", "pid", buf)) {
        return mozart_system("killall ushare");
    } else {
        pid = atoi(buf);
        return kill(pid, SIGINT);
    }

    return 0;
}
