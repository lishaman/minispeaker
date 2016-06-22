/*
 *      Real Time Clock Driver Test/Example Program
 *
 *      Compile with:
 *		     gcc -s -Wall -Wstrict-prototypes rtctest.c -o rtctest
 *
 *      Copyright (C) 1996, Paul Gortmaker.
 *
 *      Released under the GNU General Public License, version 2,
 *      included herein by reference.
 *
 */

#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

/*
 * This expects the new RTC class driver framework, working with
 * clocks that will often not be clones of what the PC-AT had.
 * Use the command line to specify another RTC if you need one.
 */
static const char default_rtc[] = "/dev/rtc0";
static int def, set, uie, al, wkal, alarm_signal;

static int rtc_default(int fd)
{
	struct tm tm_time = {
		.tm_sec = 0,
		.tm_min = 0,
		.tm_hour = 0,
		.tm_mday = 1,		/* day 1 */
		.tm_mon = (1 - 1),	/* month 1 */
		.tm_year = (2000 - 1900) /* year 2000 */
	};

	if (ioctl(fd, RTC_SET_TIME, &tm_time) < 0) {
		perror("RTC_SET_TIME ioctl");
		return -1;
	}

	return 0;
}

static int rtc_read_test(int fd)
{
	int retval;
	struct rtc_time rtc_tm;

	/* Read the RTC time/date */
	retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	if (retval == -1) {
		perror("RTC_RD_TIME ioctl");
		return -1;
	}

	fprintf(stderr, "Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
		rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
		rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	return 0;
}

static int rtc_set_test(int fd)
{
	int utc = 0;
	struct tm tm_time;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	/* Prepare tm_time from hwclock.c */
	if (sizeof(time_t) == sizeof(tv.tv_sec)) {
		if (utc)
			gmtime_r((time_t*)&tv.tv_sec, &tm_time);
		else
			localtime_r((time_t*)&tv.tv_sec, &tm_time);
	} else {
		time_t t = tv.tv_sec;
		if (utc)
			gmtime_r(&t, &tm_time);
		else
			localtime_r(&t, &tm_time);
	}
	tm_time.tm_isdst = 0;

	if (ioctl(fd, RTC_SET_TIME, &tm_time) < 0) {
		perror("RTC_SET_TIME ioctl");
		return -1;
	}

	rtc_read_test(fd);

	return 0;
}

static int rtc_uie_test(int fd)
{
	unsigned long data;
	int i, irqcount = 0, retval = 0;

	/* Turn on update interrupts (one per second) */
	if (ioctl(fd, RTC_UIE_ON, 0) < 0) {
		if (errno == ENOTTY) {
			printf("Update IRQS not supported.\n");
			return 0;
		}
		perror("RTC_UIE_ON ioctl");
		return -1;
	}

	fprintf(stdout, "Counting 5 update (1/sec) interrupts:");
	fflush(stdout);
	for (i = 0; i < 6; i++) {
		/* This read will block */
		retval = read(fd, &data, sizeof(unsigned long));
		if (retval == -1) {
			fprintf(stdout, "\n");
			perror("read");
			return -1;
		}
		fprintf(stdout, " %d", i);
		fflush(stdout);
		irqcount++;
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "Again, from using select(2) :");
	fflush(stdout);
	for (i = 0; i < 6; i++) {
		struct timeval tv = {5, 0};     /* 5 second timeout on select */
		fd_set readfds;

		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		/* The select will wait until an RTC interrupt happens. */
		retval = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (retval == -1) {
			perror("select");
			return -1;
		}
		/* This read won't block unlike the select-less case above. */
		retval = read(fd, &data, sizeof(unsigned long));
		if (retval == -1) {
			perror("read");
			return -1;
		}
		fprintf(stdout, " %d", i);
		fflush(stdout);
		irqcount++;
	}
	fprintf(stdout, "\n");

	/* Turn off update interrupts */
	retval = ioctl(fd, RTC_UIE_OFF, 0);
	if (retval == -1) {
		perror("RTC_UIE_OFF ioctl");
		return -1;
	}

	return 0;
}

static int rtc_set_alarm_test(int fd)
{
	int retval;
	unsigned long data;
	struct rtc_time rtc_tm;

	/* Read the RTC time/date */
	retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	if (retval == -1) {
		perror("RTC_RD_TIME ioctl");
		return -1;
	}

	printf("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
	       rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
	       rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Set the alarm to 5 sec in the future, and check for rollover */
	rtc_tm.tm_sec += 5;
	if (rtc_tm.tm_sec >= 60) {
		rtc_tm.tm_sec %= 60;
		rtc_tm.tm_min++;
	}
	if (rtc_tm.tm_min == 60) {
		rtc_tm.tm_min = 0;
		rtc_tm.tm_hour++;
	}
	if (rtc_tm.tm_hour == 24)
		rtc_tm.tm_hour = 0;


	retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
	if (retval == -1) {
		if (errno == ENOTTY) {
			printf("Alarm IRQS not supported.\n");
			return 0;
		}
		perror("RTC_ALM_SET ioctl");
		return -1;
	}

	/* Read the current alarm settings */
	retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
	if (retval == -1) {
		perror("RTC_ALM_READ ioctl");
		return -1;
	}

	/* Enable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_ON, 0);
	if (retval == -1) {
		perror("RTC_AIE_ON ioctl");
		return -1;
	}

	printf("Alarm time now set to %02d:%02d:%02d.\n",
	       rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
	fprintf(stdout, "Waiting 5 seconds for alarm...");
	fflush(stdout);
	/* This blocks until the alarm ring causes an interrupt */
	retval = read(fd, &data, sizeof(unsigned long));
	if (retval == -1) {
		perror("read");
		fprintf(stdout, "\n");
		return -1;
	}
	fprintf(stdout, " okay. Alarm rang.\n");

	/* Disable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_OFF, 0);
	if (retval == -1) {
		perror("RTC_AIE_OFF ioctl");
		return -1;
	}

	return 0;
}

static int rtc_set_wake_alarm_test(int fd)
{
	int retval;
	unsigned long data;
//	struct rtc_time rtc_tm;
	struct rtc_wkalrm wkalrm;

	wkalrm.enabled = 0;
	wkalrm.pending = 0;
	/* Read the RTC time/date */
	retval = ioctl(fd, RTC_RD_TIME, &wkalrm.time);
	if (retval == -1) {
		perror("RTC_RD_TIME ioctl");
		return -1;
	}

	printf("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
	       wkalrm.time.tm_year + 1900, wkalrm.time.tm_mon + 1, wkalrm.time.tm_mday,
	       wkalrm.time.tm_hour, wkalrm.time.tm_min, wkalrm.time.tm_sec);

	/* Set the alarm to 5 sec in the future, and check for rollover */
	wkalrm.time.tm_sec += 5;
	if (wkalrm.time.tm_sec >= 60) {
		wkalrm.time.tm_sec %= 60;
		wkalrm.time.tm_min++;
	}
	if (wkalrm.time.tm_min == 60) {
		wkalrm.time.tm_min = 0;
		wkalrm.time.tm_hour++;
	}
	if (wkalrm.time.tm_hour == 24)
		wkalrm.time.tm_hour = 0;


	retval = ioctl(fd, RTC_WKALM_SET, &wkalrm);
	if (retval == -1) {
		if (errno == ENOTTY) {
			printf("Alarm IRQS not supported.\n");
			return 0;
		}
		perror("RTC_WKALM_SET ioctl");
		return -1;
	}

	/* Read the current alarm settings */
	retval = ioctl(fd, RTC_WKALM_RD, &wkalrm);
	if (retval == -1) {
		perror("RTC_WKALM_RD ioctl");
		return -1;
	}

	/* Enable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_ON, 0);
	if (retval == -1) {
		perror("RTC_AIE_ON ioctl");
		return -1;
	}

	printf("Alarm time now set to %02d:%02d:%02d.\n",
	       wkalrm.time.tm_hour, wkalrm.time.tm_min, wkalrm.time.tm_sec);
	fprintf(stdout, "Waiting 5 seconds for alarm...");
	fflush(stdout);
	/* This blocks until the alarm ring causes an interrupt */
	retval = read(fd, &data, sizeof(unsigned long));
	if (retval == -1) {
		perror("read");
		fprintf(stdout, "\n");
		return -1;
	}
	fprintf(stdout, " okay. Alarm rang.\n");

	/* Disable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_OFF, 0);
	if (retval == -1) {
		perror("RTC_AIE_OFF ioctl");
		return -1;
	}

	return 0;
}

static int alarm_trigger = 0;
static void alarm_handler()
{
	printf("===> %s\n", __func__);
	alarm_trigger = 1;
}

static int rtc_alarm_signal_test(int fd)
{
	int i;

	signal(SIGALRM, alarm_handler);
	alarm(5);

	for (i = 1; i < 7; i++) {
		printf("sleep %ds ...\n", i);
		sleep(1);
	}

	if (alarm_trigger)
		return 0;
	else
		return -1;
}

static void usage(void)
{
	fprintf(stderr, "usage: rtc-test [options]...\n"
		"  -r read rtc time\n"
		"  -f set default time\n"
		"  -s set rtc time from system time\n"
		"  -u UIE\n"
		"  -a set alarm\n"
		"  -w set wake alarm\n"
		"  -g alarm signal\n"
		"  -d <rtcdev>\n");
}

int main(int argc, char **argv)
{
	int opt = 0, fd;
	const char *rtc = default_rtc;

	if (argc < 2) {
		usage();
		return -1;
	}

	while ((opt = getopt(argc, argv, "rfsuawgd:")) != -1) {
		switch (opt) {
		case 'r':
			break;
		case 'f':
			def = 1;
			break;
		case 's':
			set = 1;
			break;
		case 'u':
			uie = 1;
			break;
		case 'a':
			al = 1;
			break;
		case 'w':
			wkal = 1;
			break;
		case 'g':
			alarm_signal = 1;
			break;
		case 'd':
			rtc = optarg;
			break;
		default:
			usage();
			return -1;
		}
	}

	printf("======== %s: RTC Driver Test. ========\n", rtc);
	fd = open(rtc, O_RDONLY);
	if (fd ==  -1) {
		perror(rtc);
		exit(errno);
	}

	if (def) {
		printf("\n====> [TEST] rtc set default time <====\n");
		if (rtc_default(fd))
			goto err;
	}

	if (rtc_read_test(fd))
		return -1;

	if (set) {
		printf("\n====> [TEST] rtc set time from system time <====\n");
		if (rtc_set_test(fd))
			goto err;
	}

	if (uie) {
		printf("\n====> [TEST] UIE <====\n");
		if (rtc_uie_test(fd))
			goto err;
	}

	if (al) {
		printf("\n====> Set alarm <====\n");
		if (rtc_set_alarm_test(fd))
			goto err;
	}

	if (wkal) {
		printf("\n====> Set wake alarm <====\n");
		if (rtc_set_wake_alarm_test(fd))
			goto err;
	}

	if (alarm_signal) {
		printf("\n====> Alarm signal(5 second) <====\n");
		if (rtc_alarm_signal_test(fd))
			goto err;
	}

	printf("======== Test complete ========\n");
	close(fd);
	return 0;

err:
	printf("[ERROR] Test Fail!!!\n");
	close(fd);
	return -1;
}
