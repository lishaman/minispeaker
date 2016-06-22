#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "wifi_interface.h"

#define DRIVER_NAME "/dev/nv_wr"
#define INFO_NAME "/tmp/info"

#define UPDATE_START_FLAG 	(1 << 0)/* has update.zip status flag */
#define UPDATE_NEW_FS_FLAG      (1 << 1)
#define UPDATE_UPDATE_FS_FLAG   (1 << 2)
#define UPDATE_USER_FS_FLAG     (1 << 3)

struct wr_info {
	char *wr_buf[512];
	unsigned int size;
	unsigned int offset;
};

struct status_flag_bits {
	unsigned int ota_start:1;			//start
	unsigned int load_new_fs:1;			//new_flag
	unsigned int update_fs_finish:1;	//update_kernel_finish
	unsigned int user_fs_finish:1;		//update_fs_finish
};

struct nv_area_wr {
	unsigned int write_start;
	char url_1[512];
	char url_2[512];
	unsigned int current_version;
	unsigned int update_version;
	union {
		unsigned int update_status;
		struct status_flag_bits sfb;
	};
	unsigned int block_sum_count;
	unsigned int block_current_count;
	unsigned int kernel_size;
	unsigned int update_size;
	unsigned char resever[32 * 1024 - 1024 - 4 * 10];
	unsigned int write_count;
	unsigned int write_end;
};

static void wait_network_ready(void)
{
	wifi_info_t info;

	/* waiting for network ready. */
	info = get_wifi_mode();
	while (info.wifi_mode != STA && info.wifi_mode != STANET) {
		usleep(500 * 1000);
		printf("updater: waiting network ready!!\n");
	}

    return;
}

int main(int argv,char *argc[])
{
	struct nv_area_wr *nv_wr;
	char command[512];
	char line[1024];
	char version[32];
	char mode[2];
	char *buf;
	int fd, ret;
	FILE *fp;
	int t = 1800;

	nv_wr = malloc(32 * 1024);
	if(!nv_wr) {
		printf("malloc fail\n");
		return -1;
	}
	fd = open(DRIVER_NAME, O_RDWR);
	if(fd < 0) {
		free(nv_wr);
		printf("open %s fail\n", DRIVER_NAME);
		return -1;
	}

	lseek(fd, 0, SEEK_SET);

	ret = read(fd, nv_wr, 32*1024);
	if(ret < 0) {
		printf("read error\n");
		goto read_fail;
	}
	if((strlen(nv_wr->url_1) == 0) && (strlen(nv_wr->url_2) == 0)) {
		buf = "http://192.168.1.200/ota/download2";
		memcpy(nv_wr->url_1, buf, strlen(buf));
		memcpy(nv_wr->url_2, buf, strlen(buf));
	}
	//	sprintf(command, "wget %s/info -P /etc", nv_wr->url_1);
    wait_network_ready();
	sprintf(command, "wget -c %s/info -P /tmp", nv_wr->url_1);
	while(t){
		int i =system(command);
		if(i == 0)
			break;
		else
			sleep(2);
		t--;
	}
	fp = fopen(INFO_NAME, "r");
	if(fp == NULL) {
		printf("open %s fail\n", INFO_NAME);
		goto read_fail;
	}

	memset(line, 0, 1024);
	memset(version, 0, 32);

	while (!feof(fp)) {
		if(fgets(line, 1024, fp) == NULL)
			continue;
		buf = strchr(line, '=');
		if(buf) {
			if(strstr(line, "version")) {
				buf = buf + 1;
				memcpy(version, buf, strlen(buf));
				nv_wr->update_version = atoi(version);
				write(fd, nv_wr, 32*1024);
				read(fd, nv_wr, 32*1024);
			}
			if(strstr(line, "url")) {
				buf = buf + 1;
				memcpy(nv_wr->url_1, buf, strlen(buf) - 1);
				printf("%s", nv_wr->url_1);
			}
			if(strstr(line, "mode")) {
				buf = buf + 1;
				memcpy(mode, buf, strlen(buf) - 1);
			}
		}
		if (atoi(mode) == 0)
			nv_wr->sfb.update_fs_finish = 1;
		else
			nv_wr->sfb.update_fs_finish = 0;
		write(fd, nv_wr, 32*1024);
	}
	if(nv_wr->current_version < nv_wr->update_version) {
		nv_wr->sfb.ota_start = 1;
		nv_wr->sfb.load_new_fs = 0;


		nv_wr->sfb.user_fs_finish = 1;
		nv_wr->block_current_count = 0;
		nv_wr->block_sum_count = 0;
		write(fd, nv_wr, 32*1024);
	}

	fclose(fp);
read_fail:
	free(nv_wr);
	close(fd);

	system("reboot");
	sleep(10);

	return 0;
}
