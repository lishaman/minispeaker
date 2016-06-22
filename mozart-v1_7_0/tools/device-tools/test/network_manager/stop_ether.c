#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wifi_interface.h"

int callback_handle_func(const char *p)
{
	printf(">>>>>>Hello,the change process of the network is %s<<<<<<\n",p);
	return 0;
}

int main(int argc,char *argv[])
{
	bool status;
	wifi_ctl_msg_t stop_ether;
	struct wifi_client_register register_info;

	register_info.pid = getpid();
	register_info.reset = 1;
	register_info.priority = 3;
	strcpy(register_info.name,argv[0]);

	if(daemon(0, 1)){
		perror("daemon");
		return -1;
	}

	register_to_networkmanager(register_info,callback_handle_func);

	//wifi_info_t infor = get_wifi_mode();
	//printf("wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode],infor.ssid);
	stop_ether.cmd = STOP_ETHERNET;
	strcpy(stop_ether.name,argv[0]);

	status = request_wifi_mode(stop_ether);
//	printf(">>>>>>>client register status is %d\n",status);
	return 0;
}
