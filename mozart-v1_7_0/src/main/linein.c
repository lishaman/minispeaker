#include <stdio.h>
#include <stdbool.h>
#include <linux/soundcard.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "ini_interface.h"

#define SOUND_MIXER_WRITE_OUTSRC MIXER_WRITE(SOUND_MIXER_OUTSRC)

int linein_fd = -1;

static bool mozart_m150_linein_is_in(void)
{
    char buf[2] = {};
    int fd = -1;

    fd = open("/sys/devices/platform/jz-linein/state", O_RDONLY);
    if (fd == -1) {
        printf("open /sys/devices/platform/jz-linein/state failed, can not get linein status.\n");
        return false;
    }

    if (read(fd, buf, 1) == 1) {
        close(fd);
        return buf[0] == '1' ? true : false;
    } else {
        close(fd);
        return false;
    }
}

static bool mozart_x1000_linein_is_in(void)
{
    char buf[2] = {};
    int fd = -1;

    fd = open("/sys/devices/virtual/switch/linein/state", O_RDONLY);
    if (fd == -1) {
        printf("open /sys/devices/virtual/switch/linein/state failed, can not get linein status.\n");
        return false;
    }

    if (read(fd, buf, 1) == 1) {
        close(fd);
        return buf[0] == '1' ? true : false;
    } else {
        close(fd);
        return false;
    }
}

static int mozart_m150_linein_open(void)
{
        linein_fd = open("/dev/linein", O_RDWR);
        if(linein_fd < 0){
                printf("switch to linein mode fail, open /dev/linein error: %s", strerror(errno));
                return -1;
        }

        printf("[m150] switch to linein mode.\n");
        return linein_fd;
}

static int mozart_x1000_linein_open(void)
{
        int mode = SOUND_MIXER_LINE;
        int mixer_fd = -1;

        mixer_fd = open("/dev/mixer", O_RDWR);
        if (mixer_fd < 0) {
                perror("switch to linein mode fail, open /dev/mixer error");
                return -1;
        }

        if (ioctl(mixer_fd, SOUND_MIXER_WRITE_RECSRC, &mode) == -1) {
                perror("switch to linein mode failed, ioctl error");
                close(mixer_fd);
                return -1;
        }

        close(mixer_fd);
        printf("[x1000] switch to linein mode.\n");
        return 0;
}

static void mozart_m150_linein_close(void)
{
        if(linein_fd >= 0){
                close(linein_fd);
                linein_fd = -1;
        }

        printf("[m150] exit linein mode, recovery origin router.\n");
}

static void mozart_x1000_linein_close(void)
{
        int mode = SOUND_MIXER_SPEAKER;
        int mixer_fd = -1;

        mixer_fd = open("/dev/mixer", O_RDWR);
        if (mixer_fd < 0) {
                perror("close linein mode fail, open /dev/mixer error");
                return;
        }

        if (ioctl(mixer_fd, SOUND_MIXER_WRITE_OUTSRC, &mode) == -1) {
                perror("switch to speaker mode, ioctl error");
                close(mixer_fd);
                return;
        }

        close(mixer_fd);
        printf("[x1000] exit linein mode, switch to speaker.\n");
        return;
}

int mozart_linein_open(void)
{
        char buf[8] = {};

        if (mozart_ini_getkey("/usr/data/system.ini", "product", "cpu", buf))
                return mozart_m150_linein_open();

        if (!strcasecmp(buf, "x1000"))
                return mozart_x1000_linein_open();
        else if (!strcasecmp(buf, "m150"))
                return mozart_m150_linein_open();

        printf("Unknow cpu model, open linein failed.\n");
        return -1;
}

void mozart_linein_close(void)
{
        char buf[8] = {};

        if (mozart_ini_getkey("/usr/data/system.ini", "product", "cpu", buf))
                return mozart_m150_linein_close();

        if (!strcasecmp(buf, "x1000"))
                return mozart_x1000_linein_close();
        else if (!strcasecmp(buf, "m150"))
                return mozart_m150_linein_close();

        printf("Unknow cpu model, close linein failed.\n");
        return;
}

bool mozart_linein_is_in(void)
{
        char buf[8] = {};
        if (mozart_ini_getkey("/usr/data/system.ini", "product", "cpu", buf)) {
                return mozart_m150_linein_is_in();
        }

        if (!strcasecmp(buf, "x1000"))
                return mozart_x1000_linein_is_in();
        else if (!strcasecmp(buf, "m150"))
                return mozart_m150_linein_is_in();

        printf("Unknow cpu model, get linein state failed.\n");
        return false;
}
