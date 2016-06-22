#include <stdio.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ota_interface.h"

int main()
{
       printf("start test ota\n");
	int fd =  mozart_ota_init();

	printf("start test set url\n");
	char *tmp = "http://192.168.1.200/ota/download2";
	unsigned int size = sizeof("http://192.168.1.200/ota/download2");

	mozart_ota_seturl(tmp,size,fd);

	printf("start test ota get url\n");
	mozart_ota_geturl(fd);

	printf("start test ota get software version\n");
        unsigned int version = mozart_ota_get_version(fd);
	printf("version is %d\n",version);


	printf("start test ota set version\n");
	unsigned int version_set = 20150833;
	mozart_ota_set_version(version_set,fd);

	printf("start test ota get software version\n");
	version = mozart_ota_get_version(fd);
	printf("version is %d\n",version);

	printf("start test ota close\n");
	mozart_ota_close(fd);

//	printf("start test ota update\n");
//	mozart_ota_start_update();

//	printf("start test ota recovery\n"); 
//	mozart_ota_start_recovery();

	printf("test ota finish\n");
	return 0;
}
