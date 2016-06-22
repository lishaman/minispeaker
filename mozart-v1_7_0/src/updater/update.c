#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "wifi_interface.h"
#include "utils_interface.h"
#include "tips_interface.h"

#define DRIVER_NAME	"/dev/nv_wr"
#define TMPFILE_NAME	"/tmp/tmpfile"

#ifndef UPDATE_RELEASE
#define UPDATE_DEBUG
#endif

#ifdef UPDATE_DEBUG
#define pr_debug(name, fmt, args...)	printf("%s.%s [Debug] "fmt, name, __func__, ##args)
#else	/* UPDATE_DEBUG */
#define pr_debug(name fmt, args...)
#endif	/* UPDATE_DEBUG */

#define pr_info(name, fmt, args...)	printf("%s.%s [Info] "fmt, name, __func__, ##args)
#define pr_err(name, fmt, args...)	fprintf(stderr, "%s.%s [Error] "fmt, name, __func__, ##args)

enum {
	KERNEL_SIZE	= 1,
	UPDATEFS_SIZE	= 2,
	USRFS_SIZE	= 3,
};

struct wr_info {
	char *wr_buf[512];
	unsigned int size;
	unsigned int offset;
};

struct status_flag_bits {
	unsigned int ota_start:1;		/* start */
	unsigned int load_new_fs:1;		/* new_flag */
	unsigned int update_fs_finish:1;	/* update_kernel_finish */
	unsigned int user_fs_finish:1;		/* update_fs_finish */
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

static char *appName = NULL;
static char command[2048];

/* get sum from update.script */
static int get_sum()
{
	int fd_sh, sum;
	char buffer[256];
	char buf1[20], buf2[20], buf3[20];

	fd_sh = open(TMPFILE_NAME, O_RDONLY);
	read(fd_sh, buffer, sizeof(buffer));
	sscanf(buffer,"%[sum=]%[0-9]%s", buf1, buf2, buf3);
	sum = atoi(buf2);

	close(fd_sh);
	return sum;
}

static int get_reboot()
{
	int fd_sh, reboot;
	char buffer[512];
	char buf1[20], buf2[20], buf3[20], buf4[20], buf5[20];

	fd_sh = open(TMPFILE_NAME, O_RDONLY);
	read(fd_sh, buffer, sizeof(buffer));
	sscanf(buffer,"%[sum=]%[0-9]%s", buf1, buf2, buf3);
	sscanf(buf3,"%[reboot=]%[0-9]", buf4, buf5);
	reboot = atoi(buf5);

	close(fd_sh);
	return reboot;
}

static int get_size(int name)
{

	int fd_sh, size=0;
	char buffer[512];
	char buf1[20], buf2[20];

	fd_sh = open(TMPFILE_NAME, O_RDONLY);
	read(fd_sh, buffer, sizeof(buffer));

	switch (name) {
	case KERNEL_SIZE:
		sscanf(buffer,"%*s %*s %[kernel_size=]%[0-9]", buf1, buf2);
		size = atoi(buf2);
		break;
	case UPDATEFS_SIZE:
		sscanf(buffer,"%*s %*s %*s %[update_size=]%[0-9]", buf1, buf2);
		size = atoi(buf2);
		break;
	case USRFS_SIZE:
		sscanf(buffer,"%*s %*s %*s %*s %[usrfs_size=]%[0-9]", buf1, buf2);
		size = atoi(buf2);
		break;
	}

	int align = 512 * 1024;
	size = (size + align-1) & ~(align-1);
	size = size / 1024;

	close(fd_sh);

	return size;
}

static void stage_reboot(void)
{
	mozart_system("reboot");
	sleep(10);
}

static int update_network_initiate(char *name)
{
	struct wifi_client_register wifi_info;
	wifi_info_t infor;
	int try_cnt;

	/* wait for 5s if network is invalid now. */
	try_cnt = 0;
	while (try_cnt++ < 10) {
		if (!access("/var/run/wifi-server/register_socket", F_OK) &&
		    !access("/var/run/wifi-server/server_socket", F_OK))
			break;
		usleep(500 * 1000);
	}

	if (try_cnt >= 10) {
		pr_err(appName, "Can't connect to networkmanager\n");
		return -1;
	}

	/* register network manager */
	wifi_info.pid		= getpid();
	wifi_info.reset		= 1;
	wifi_info.priority	= 3;
	strcpy(wifi_info.name, name);
	register_to_networkmanager(wifi_info, NULL);

	try_cnt = 0;
	do {
		infor = get_wifi_mode();
		if (infor.wifi_mode == -1) {
			pr_debug(appName, "waiting network_manager ready in %s:%d!!\n", __FILE__, __LINE__);
			usleep(500 * 1000);
		}
	} while (infor.wifi_mode == -1 && try_cnt++ < 10);

	if (infor.wifi_mode != -1 && infor.wifi_mode == WIFI_NULL) {
		wifi_ctl_msg_t switch_sta;

		memset(&switch_sta, 0, sizeof(wifi_ctl_msg_t));
		switch_sta.param.switch_sta.sta_timeout = 60;
		switch_sta.cmd = SW_STA;
		if (!request_wifi_mode(switch_sta)) {
			pr_err(appName, "request_wifi_mode fail!\n");
			return -1;
		}
	}

	return 0;
}

static void wait_network_ready(void)
{
	wifi_info_t info;

	/* waiting for network ready. */
	while (1) {
		info = get_wifi_mode();
		if (info.wifi_mode == STA || info.wifi_mode == STANET)
			break;
		pr_debug(appName, "waiting for network ready!!\n");
		usleep(500 * 1000);
	}

    return;
}

int main(int argc, char *argv[])
{
	struct nv_area_wr *nv_wr;
	int fd;
	char *url;
	ssize_t rSize, wSize;
	int err;

	appName = basename(argv[0]);

	/* Get nv info */
	nv_wr = malloc(sizeof(struct nv_area_wr));
	if (!nv_wr) {
		pr_err(appName, "alloc nv_wr: %s\n", strerror(errno));
		return -1;
	}

	fd = open(DRIVER_NAME, O_RDWR);
	if (fd < 0) {
		pr_err(appName, "open %s: %s\n", DRIVER_NAME, strerror(errno));
		goto err_nv_open;
	}

	lseek(fd, 0, SEEK_SET);

	rSize = read(fd, nv_wr, sizeof(struct nv_area_wr));
	if (rSize < 0) {
		pr_err(appName, "read nv area: %s\n", strerror(errno));
		goto err_nv_read;
	}

	pr_info(appName, "start:%d new:%d update:%d user:%d\n",
			nv_wr->sfb.ota_start, nv_wr->sfb.load_new_fs,
			nv_wr->sfb.update_fs_finish, nv_wr->sfb.user_fs_finish);

	if (!nv_wr->sfb.ota_start) {
		close(fd);
		free(nv_wr);
		return 0 ;
	}

	if (nv_wr->url_1) {
		url = nv_wr->url_1;
	} else if (nv_wr->url_2) {
		url = nv_wr->url_2;
	} else {
		pr_err(appName, "url_1 and url_2 is all null\n");
		goto err_url;
	}

	pr_info(appName, "url = %s\n", url);

	/* Phase 1: Get kernel and update.cramfs, then
	 *          write to appfs.cramfs partition.
	 *
	 * Phase 2: Start with appfs.cramfs kernel and update.cramfs.
	 *          update kernel and update.cramfs by copy appfs.cramfs
	 *          kernel and update.cramfs to old kernel and old update.
	 *          cramfs.
	 *
	 * Phase 3: Start normal, then update appfs.cramfs from remote.
	 *
	 * Note:
	 * 	2 update ways:
	 *
	 * 	1. Phase_1 -> Phase_2 -> phase_3 -> finish_upate.
	 * 	2. Phase_3 -> finish_update.
	 */

	if (nv_wr->sfb.update_fs_finish == 1 && nv_wr->sfb.user_fs_finish == 1) {
		if (!nv_wr->sfb.load_new_fs) {
			/* Phase 1 */
			update_network_initiate(basename(argv[0]));
			wait_network_ready();

			for (;;) {
				/* begin wget and update kernel */
				sprintf(command, "unzip.sh %s %d %d ",
						url,
						nv_wr->update_version,
						nv_wr->block_current_count);

				pr_debug(appName, "%s\n", command);
				err = mozart_system(command);
				if (err == -1)
					pr_err(appName, "Run system failed\n");
				else if (WEXITSTATUS(err))
					pr_err(appName, "Run unzip.sh error\n");

				/* wget update and write to sfc nor */
				if (nv_wr->block_current_count == 0) {
					nv_wr->block_sum_count	= get_sum();
					nv_wr->kernel_size	= get_size(KERNEL_SIZE);
					nv_wr->update_size	= get_size(UPDATEFS_SIZE);

					pr_debug(appName, "nv_wr->kernel_size = %d\n", nv_wr->kernel_size);
					pr_debug(appName, "nv_wr->update_size = %d\n",nv_wr->update_size);
				}

				if (!get_reboot()) {
					nv_wr->block_current_count++;
					wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
					if (wSize < 0)
						pr_err(appName, "write nv back: %s\n", strerror(errno));
				} else {
					nv_wr->sfb.load_new_fs = 1;
					wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
					if (wSize < 0)
						pr_err(appName, "write nv back: %s\n", strerror(errno));

					stage_reboot();
					/* Should NOT be there */
				}
			}
		} else {
			/* Phase 2 */

			sprintf(command,"mtd_debug erase /dev/mtd2 0 0x%x",nv_wr->kernel_size*1024);
			mozart_system(command);
			sprintf(command,"mtd_debug erase /dev/mtd3 0 0x%x",nv_wr->update_size*1024);
			mozart_system(command);
			sprintf(command,"dd if=/dev/mtdblock4 of=/dev/mtdblock2 skip=9216 bs=512 count=%d",nv_wr->kernel_size*2);
			mozart_system(command);
			sprintf(command,"dd if=/dev/mtdblock4 of=/dev/mtdblock3 skip=0 bs=512 count=%d",nv_wr->update_size*2);
			mozart_system(command);


			nv_wr->sfb.load_new_fs		= 0;
			nv_wr->sfb.update_fs_finish	= 0;
			nv_wr->block_current_count++;

			/* change nv and write to nor */
			wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
			if (wSize < 0)
				pr_err(appName, "write nv back: %s\n", strerror(errno));

			stage_reboot();
			/* Should NOT be there */
		}
	}

	if (nv_wr->sfb.update_fs_finish == 0 && nv_wr->sfb.user_fs_finish == 1) {
		/* Phase 3 */
		update_network_initiate(basename(argv[0]));
		wait_network_ready();

		for (;;) {
			/* begin to wget usrfs */
			sprintf(command, "unzip.sh %s %d %d ",
					url, nv_wr->update_version, nv_wr->block_current_count);
			err = mozart_system(command);
			if (err == -1) {
				pr_err(appName, "Run system failed\n");
			} else if (WEXITSTATUS(err)) {
				pr_err(appName, "Run unzip.sh error\n");
			}

			if (nv_wr->block_current_count == 0) {
				nv_wr->block_sum_count = get_sum();
				wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
				if (wSize < 0)
					pr_err(appName, "write nv back: %s\n", strerror(errno));
			}

			if (nv_wr->block_current_count != (nv_wr->block_sum_count - 1)) {
				nv_wr->block_current_count++;
				wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
				if (wSize < 0)
					pr_err(appName, "write nv back: %s\n", strerror(errno));
			} else {
				/* last block */
				nv_wr->sfb.user_fs_finish = 0;
				/* change nv and write to nor */
				wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
				if (wSize < 0) {
					pr_err(appName, "write nv back: %s\n", strerror(errno));
				}

				pr_info(appName, "Update finished\n");

				nv_wr->current_version	= nv_wr->update_version;
				nv_wr->update_version	= 0;
				nv_wr->update_status	= 0;
				nv_wr->block_sum_count	= 0;
				wSize = write(fd, nv_wr, sizeof(struct nv_area_wr));
				if (wSize < 0) {
					pr_err(appName, "write nv back: %s\n", strerror(errno));
				}

				mozart_play_key("update_successed");

				stage_reboot();
				/* Should NOT be there */
			}
		}
	}

err_url:
err_nv_read:
	close(fd);

err_nv_open:
	free(nv_wr);

	return -1;
}
