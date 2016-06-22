#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wifi_interface.h"

int main(int argc,char *argv[])
{
	bool status;
	wifi_ctl_msg_t switch_ap_test;
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

	switch_ap_test.force = true;
	switch_ap_test.cmd = SW_AP;

	if (argc >= 2 && !strcmp(argv[1], "--config")) {
		printf("run in here: %s:%d.\n", __func__, __LINE__);
		strcpy(switch_ap_test.param.switch_ap.ssid, "wifi-created-by-switch_ap_test");
		strcpy(switch_ap_test.param.switch_ap.psk, "12345678");
	} else {
		printf("run in here: %s:%d.\n", __func__, __LINE__);
		memset(switch_ap_test.param.switch_ap.ssid, 0, sizeof(switch_ap_test.param.switch_ap.ssid));
		memset(switch_ap_test.param.switch_ap.psk, 0, sizeof(switch_ap_test.param.switch_ap.psk));
	}

	strcpy(switch_ap_test.name,argv[0]);
	status = request_wifi_mode(switch_ap_test);

	memset(&infor, 0, sizeof(infor));
	infor = get_wifi_mode();
	printf("wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode],infor.ssid);

	return 0;
}
