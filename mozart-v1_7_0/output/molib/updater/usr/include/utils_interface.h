/* ************************************************************************
 *       Filename:  utils_interface.h
 *    Description:
 *        Version:  1.0
 *        Created:  10/28/2014 06:57:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xllv (),
 *        Company:
 * ************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
	IDLE = 0,
	READING,
	WRITEING,
	BUSY,
} dsp_status;

typedef enum {
	AUDIO_UNKNOWN,
	AUDIO_OSS,
	AUDIO_ALSA,
} audio_type;

typedef struct sdcard_info{
	unsigned long long  totalSize;       //总的空间
	unsigned long long  freeSize;        //剩余空间
	unsigned long long  availableSize;   //可用空间
} sdinfo;

extern void set_owner(int fd, pid_t pid);
extern void set_flag(int fd, int flags);
extern void clr_flag(int fd, int flags);
extern void get_dsp_status(dsp_status *dsp_stat);

extern char *get_ip_addr(char *ifname);
extern char *get_mac_addr(char *ifname, char *macaddr, char *separator);
extern int mozart_system(const char *cmd);
extern char *mozart_itoa(int value, char *str);
extern bool mozart_path_is_mount(const char *path);
extern sdinfo mozart_get_sdcard_info(const char *sdpath);

/**
 * @Synopsis  判断DSP设备是否可以通过指定opt方式打开
 *
 * @Param int opt:O_RDONLY, O_WRONLY, or O_RDWR
 *
 * @Returns   可以打开返回1 否则返回0
 */
int check_dsp_opt(int opt);
extern audio_type get_audio_type();

#ifdef  __cplusplus
}
#endif

#endif /** __UTILS_H__ **/
