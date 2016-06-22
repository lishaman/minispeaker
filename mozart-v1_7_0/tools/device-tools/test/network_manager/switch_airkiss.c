#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wifi_interface.h"

int main(int argc,char *argv[])
{
	bool status;
	wifi_ctl_msg_t switch_airkiss;
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

	memset(&switch_airkiss, 0, sizeof(wifi_ctl_msg_t));
	wifi_info_t infor = get_wifi_mode();
	printf("wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode], infor.ssid);

	switch_airkiss.force = true;
	switch_airkiss.cmd = SW_NETCFG;
	strcpy(switch_airkiss.name,argv[0]);
	strcpy(switch_airkiss.param.network_config.product_model, "ALINKTEST_LIVING_LIGHT_SMARTLED");
	switch_airkiss.param.network_config.method |= 0x05;
	strcpy(switch_airkiss.param.network_config.wl_module, wifi_module_str[BROADCOM]);
	switch_airkiss.param.network_config.timeout = -1;
	if (argc >= 2 && !strcmp(argv[1], "--smartlink")) {
		strcpy(switch_airkiss.param.network_config.key, "Q510101505000001");
	}
	status = request_wifi_mode(switch_airkiss);
	printf(">>>>>>>client register status is %d\n",status);

	memset(&infor, 0, sizeof(infor));
	infor = get_wifi_mode();
	printf("wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode], infor.ssid);

	return 0;
}
