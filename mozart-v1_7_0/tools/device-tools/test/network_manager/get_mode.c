#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wifi_interface.h"

int main(int argc,char *argv[])
{
	wifi_info_t infor = get_wifi_mode();

	printf("The current of wifi mode is %s, SSID is %s\n", wifi_mode_str[infor.wifi_mode],infor.ssid);
	if(!system("ifconfig -a | grep -v grep | grep eth0 > /dev/null"))
		printf("The current Ethernet mode is %s\n", ether_state_str[infor.ether_state]);
	return 0;
}
