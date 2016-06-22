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
#define CMD_GET_INFO    _IOR('N', 6, unsigned int)
#define CMD_GET_VERSION    _IOR('N', 7, unsigned int)

#define UPDATA_START_FLAG 	(1 << 0)/* has updata.zip status flag */
#define UPDATA_NEW_FS_FLAG      (1 << 1)
#define UPDATA_UPDATA_FS_FLAG   (1 << 2)
#define UPDATA_USER_FS_FLAG     (1 << 3)

struct wr_info {
	char *wr_buf[512];
	unsigned int size;
};

struct status_flag_bits {
	unsigned int ota_start:1;
	unsigned int load_new_fs:1;
	unsigned int updata_fs_finish:1;
	unsigned int user_fs_finish:1;
};

struct nv_area_wr {
	unsigned int write_start;
	char url_1[512];
	char url_2[512];
	unsigned int current_version;
	unsigned int updata_version;
	union {
		unsigned int updata_status;
		struct status_flag_bits sfb;
	};
	unsigned int block_sum_count;
	unsigned int block_current_count;
	unsigned int resever[32 * 1024 - 1024 - 4 * 8];
	unsigned int write_count;
	unsigned int write_end;
};
#define NV_AREA_START (256 * 1024)


static int set_url(char *url, unsigned int size, unsigned int url_num, int fd)
{
	struct wr_info wr_info;
	if(size > 512) {
		printf("url size more then 512\n");
		return -1;
	}

	memcpy(wr_info.wr_buf, url, size);
	wr_info.size = size;
	if(url_num == 1)
		ioctl(fd, CMD_WRITE_URL_1, &wr_info);
	else if(url_num == 2)
		ioctl(fd, CMD_WRITE_URL_2, &wr_info);
	else {
		printf("not support write other url\n");
	}
	return wr_info.size;
}
static int get_url(char *url, unsigned int size, unsigned int url_num, int fd)
{
	struct wr_info wr_info;

	wr_info.size = size;
	if(url_num == 1)
		ioctl(fd, CMD_READ_URL_1, &wr_info);
	else if(url_num == 2)
		ioctl(fd, CMD_READ_URL_2, &wr_info);
	else {
		printf("not support read other url\n");
		return -1;
	}

	memcpy(url, wr_info.wr_buf, wr_info.size);
	return wr_info.size;
}
static unsigned int get_version(int fd)
{
	unsigned int version;

	ioctl(fd, CMD_GET_VERSION, &version);
	return version;
}
int main(int argv, char *argc[])
{
	int fd, ret, i;
	struct nv_area_wr *nv_wr;
	struct wr_info wr_info;
	char *buf;
	unsigned int offset;
	unsigned int count;
	unsigned int write_off;
	unsigned int wr = 0, size;


	switch(argc[1][0]) {
	case 'r':
		wr = 0;
		break;
	case 'w':
		wr = 1;
		break;
	case 's':
		wr = 2;
		break;
	case 'g':
		wr = 3;
		break;
	case 'c':
		wr = 4;
		break;
	case 'v':
		wr = 5;
		break;
	default:
		printf("not support\n");
		return -1;
	}



	fd = open(DRIVER_NAME, O_RDWR);
	if(fd < 0) {
		printf("open error!\n");
		return -1;
	}
	if(wr < 2) {
		if(argv < 4) {
			printf("./nv [w/r] offset count\n");
			return -1;
		}
		offset = atoi(argc[2]);
		count = atoi(argc[3]);
		buf = malloc(count);
		memset(buf, 0, count);
	}
	lseek(fd, offset, SEEK_SET);
	switch(wr) {
	case 0:
		ret = read(fd, buf, count);
		if(ret < 0) {
			printf("read error\n");
			return -1;
		}
		for(i = 0; i < count; i ++)
			printf("buf[%d]= %d\n", i, buf[i]);
		break;
	case 1:
		for( i = 0; i < count; i++)
			buf[i] = i % 10;
		ret = write(fd, buf, count);
		if(ret < 0) {
			printf("write error\n");
			return -1;
		}
		break;
	case 2:
	{
		char *tmp = "http://192.168.4.13";
		unsigned int size = sizeof("http://192.168.4.13");
		set_url(tmp, size, 1, fd);
	}
	break;
	case 3:
	{
		char url[60] = {0};
		unsigned int size = 60;
		get_url(url, size, 1, fd);
		printf("url = %s\n", url);
	}
	break;
	case 4:
		ioctl(fd, CMD_GET_INFO,&size);
		printf("read url %d\n", size);
		break;
	case 5:
		printf("current version = %d\n", get_version(fd));
		break;
	}

	free(buf);
	close(fd);
	return 0;
}
