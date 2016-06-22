#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>

#include "atalk_control.h"
#include "wifi_interface.h"
#include "tips_interface.h"
#include "utils_interface.h"
#include "player_interface.h"
#include "volume_interface.h"
#include "sharememory_interface.h"

#define DEBUG 0
#if DEBUG
#define pr_debug(fmt, args...)			\
	printf(fmt, ##args)
#else
#define pr_debug(fmt, args...)			\
	do {} while (0)
#endif

#define DATA_NUM 512

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define barrier() __asm__ __volatile__("" : : : "memory")

struct test_case {
	char name[128];
	void *data[DATA_NUM];
	int result;
	void (*run)(struct test_case *);
	int  (*check)(struct test_case *);
	void (*error)(struct test_case *);
	void (*cleanup)(struct test_case *);
};

static int to_exit;
static wifi_info_t wifi_info = {
	.wifi_mode = STA,
};

static void cleanup_atalk(struct test_case *test);

static void sig_handler(int signo)
{
	/* cleanup_atalk(NULL); */
	to_exit = 1;
}

static int check_is_zero(struct test_case *test)
{
	return test->result == 0;
}

static void run_atalk_app(struct test_case *test)
{
	test->result = start_atalk_app();
}

/* #define SEND_WIFI_START */
static void run_net_state(struct test_case *test)
{
	char c;

	while (!to_exit) {
		printf("play...\n");
		c = getchar();
		if (c == 'q') {
			getchar();
			break;
		}
		if (to_exit)
			break;
#ifndef SEND_WIFI_START
		mozart_atalk_stop();
#endif
		atalk_network_trigger(false);
/* #ifdef SEND_WIFI_START */
		printf("sleep 15\n");
		sleep(15);
/* #endif */
		system("startap.sh >/dev/null 2>&1");
		wifi_info.wifi_mode = AP;

		printf("wifi config...\n");
		c = getchar();
		if (c == 'q') {
			getchar();
			break;
		}
		if (to_exit)
			break;

#ifndef SEND_WIFI_START
		mozart_atalk_stop();
#endif
		atalk_network_trigger(false);
		system("startsta.sh >/dev/null 2>&1");
		wifi_info.wifi_mode = STA;
		atalk_network_trigger(true);
	}

	test->result = 0;
}

static void run_atalk_ctl(struct test_case *test)
{
	char c;

	while (!to_exit) {
		c = getchar();
		if (c == 's') {
			mozart_atalk_stop();
			getchar();
		} else if (c == 'w') {
			mozart_atalk_start();
			getchar();
		} else if (c == 'q') {
			getchar();
			break;
		}
	}

	test->result = 0;
}

static void cleanup_atalk(struct test_case *test)
{
	system("startsta.sh >/dev/null 2>&1");
	stop_atalk_app();
}

static struct test_case test_case[] = {
	{
		.name = "start_atalk_app",
		.run = run_atalk_app,
		.check = check_is_zero,
	},
	{
		.name = "atalk_ctl",
		.run = run_atalk_ctl,
		.check = check_is_zero,
	},
	{
		.name = "atalk_net_state",
		.run = run_net_state,
		.check = check_is_zero,
		.cleanup = cleanup_atalk,
	},
};

int test_template(struct test_case *test)
{
	int i;

	pr_debug("\n>>>> [Test Case] %s <<<<\n", test->name);

	pr_debug("%s run\n", test->name);
	if (test->run)
		test->run(test);

	pr_debug("%s check\n", test->name);
	if (!test->check(test)) {
		printf("**** [ERROR] %s error!!! ****\n", test->name);
		if (test->error)
			test->error(test);
		return 1;
	}

	pr_debug("%s cleanup\n", test->name);
	if (test->cleanup)
		test->cleanup(test);
	for (i = 0; i < DATA_NUM + 1; i++) {
		if (test->data[i]) {
			free(test->data[i]);
			test->data[i] = NULL;
		}
	}

	return 0;
}

int main(void)
{
	int i;
	struct test_case *test;

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGABRT, sig_handler);
	signal(SIGPIPE, SIG_IGN);

	printf("\n=======> atalk test(%5d) <============\n", getpid());

	for (i = 0; i < ARRAY_SIZE(test_case); i++) {
		test = &test_case[i];
		test->result = test_template(test);
	}

	printf("\nResult:\n");
	for (i = 0; i < ARRAY_SIZE(test_case); i++) {
		test = &test_case[i];
		printf("    %s    %s\n", test->result ? "FAIL" : "OK  ",
			test->name);
	}

	printf("===========  atalk over  ===============\n\n");

	return 0;
}

/* fake interface */
static player_handler_t player;
static unsigned long current_s, time_s;
static struct timeval start, end;
player_handler_t *player_get_player_handler(char *uuid)
{
	player.player_status =
		(player_status_t *)calloc(1, sizeof(player_status_t));
	player.player_status->play_status = READY_STATUS;
	strncpy(player.uuid, uuid, strlen(uuid));
	return &player;
}

int player_play_url(player_handler_t *player_handler, char *url)
{
	if (player.player_status->url)
		free(player.player_status->url);
	player.player_status->url = strdup(url);

	gettimeofday(&start, NULL);
	player.player_status->play_status = PLAYING_STATUS;
	strncpy(player.player_status->uuid, player.uuid, strlen(player.uuid));
	current_s = 999;

	return 0;
}

player_status_t *player_get_player_status(player_handler_t *player_handler)
{
	if (player.player_status->play_status == STOP_STATUS) {
		memset(player.player_status->uuid, 0, sizeof(player.player_status->uuid) - 1);
		return player.player_status;
	} else if (player.player_status->play_status != PLAYING_STATUS) {
		gettimeofday(&start, NULL);
	} else if (player.player_status->play_status == PLAYING_STATUS) {
		gettimeofday(&end, NULL);
		time_s = end.tv_sec - start.tv_sec;

		if (time_s != current_s) {
			current_s = time_s;
			printf("%ld ", current_s);
			fflush(stdout);
		}

		if (time_s >= 10) {
			player.player_status->play_status = STOP_STATUS;
			printf("\n");
		}
	}

	return player.player_status;
}

char *player_get_url(player_handler_t *player_handler)
{
	return player.player_status->url;
}

int player_do_resume(player_handler_t *player_handler)
{
	gettimeofday(&start, NULL);
	player.player_status->play_status = PLAYING_STATUS;

	return 0;
}

int player_do_pause(player_handler_t *player_handler)
{
	player.player_status->play_status = PAUSE_STATUS;

	return 0;
}

void mozart_led_turn_on(int led_num)
{
}

void mozart_led_turn_off(int led_num)
{
}

static int volume = 50;
int mozart_volume_get(void)
{
	return volume;
}

int mozart_volume_set(int volume, volume_domain domain)
{
	volume = volume;
	return 0;
}

int player_stop_playback(player_handler_t *player_handler)
{
	player.player_status->play_status = STOP_STATUS;
	return 0;
}

wifi_info_t get_wifi_mode(void)
{
	return wifi_info;
}

int share_mem_init(void)
{
	return 0;
}

int share_mem_set(memory_domain domain, module_status status)
{
	return 0;
}

int share_mem_get(memory_domain domain, module_status *status)
{
	*status = STATUS_RUNNING;
	return 0;
}

int share_mem_stop_other(memory_domain domain)
{
	return 0;
}
