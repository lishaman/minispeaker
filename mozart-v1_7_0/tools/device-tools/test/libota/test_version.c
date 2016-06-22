#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DRIVER_NAME "/dev/nv_wr"

#define CMD_WRITE_URL_1 _IOW('N', 0, char)
#define CMD_WRITE_URL_2 _IOW('N', 1, char)
#define CMD_READ_URL_1  _IOR('N', 3, char)
#define CMD_READ_URL_2  _IOR('N', 4, char)
#define CMD_EARASE_ALL  _IOR('N', 5, int)
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
	unsigned int resever[32 * 1024 - 1024 - 4 * 10];
	unsigned int write_count;
	unsigned int write_end;
};

int main(int argv,char *argc[])
{
	struct nv_area_wr *nv_wr;
	char *buf;
	int fd, ret;

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
	printf("nv_wr->url_1 is %s\n",nv_wr->url_1);
        printf("nv_wr->url_2 is %s\n",nv_wr->url_2);
        printf("nv_wr->current_version is %d\n",nv_wr->current_version);
        printf("nv_wr->update_version is %d\n",nv_wr->update_version);

	nv_wr->current_version = 20140812;
	nv_wr->update_version = 20150822;

	write(fd, nv_wr, 32*1024);

read_fail:
	free(nv_wr);
	close(fd);

	return 0;
}
