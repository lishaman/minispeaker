#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wifi_interface.h"

int main(int argc,char *argv[])
{
	bool status;
	wifi_ctl_msg_t switch_sta_test;
	struct wifi_client_register register_info;

	register_info.pid = getpid();
	register_info.reset = 1;
	register_info.priority = 3;
	strcpy(register_info.name,argv[0]);

	if(daemon(0, 1)){
		perror("daemon");
		return -1;
	}

	register_to_networkmanager(register_info, NULL);

	wifi_info_t infor = get_wifi_mode();
	printf("wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode],infor.ssid);

	switch_sta_test.force = true;
	switch_sta_test.param.switch_sta.sta_timeout = 60;
	switch_sta_test.cmd = SW_STA;
	strcpy(switch_sta_test.name,argv[0]);

	if (argc >= 2 && !strcmp(argv[1], "--config")) {
		printf("run in here: %s:%d.\n", __func__, __LINE__);
		strcpy(switch_sta_test.param.switch_sta.ssid,"iad-smart-config");
		strcpy(switch_sta_test.param.switch_sta.psk,"123456789");
		// bssid is the MAC address of the wifi to be connected:
		// strcpy(switch_sta_test.param.switch_sta.bssid,"5c:63:bf:3b:6d:92");
		memset(switch_sta_test.param.switch_sta.bssid, 0, sizeof(switch_sta_test.param.switch_sta.bssid));
	} else {
		printf("run in here: %s:%d.\n", __func__, __LINE__);
		memset(switch_sta_test.param.switch_sta.ssid, 0, sizeof(switch_sta_test.param.switch_sta.ssid));
		memset(switch_sta_test.param.switch_sta.psk, 0, sizeof(switch_sta_test.param.switch_sta.psk));
		memset(switch_sta_test.param.switch_sta.bssid, 0, sizeof(switch_sta_test.param.switch_sta.bssid));
	}

	status = request_wifi_mode(switch_sta_test);

	memset(&infor, 0, sizeof(infor));
	infor = get_wifi_mode();
	printf("wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode], infor.ssid);

	return 0;
}
