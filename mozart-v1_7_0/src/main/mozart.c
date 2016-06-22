#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>
#include <execinfo.h>

#include "battery_capacity.h"
#include "key_function.h"
#include "event_interface.h"
#include "wifi_interface.h"
#include "volume_interface.h"
#include "player_interface.h"
#include "tips_interface.h"
#include "app.h"
#include "airplay.h"
#include "linein.h"
#include "json-c/json.h"
#include "ximalaya.h"
#include "utils_interface.h"
#include "power_interface.h"
#include "sharememory_interface.h"
#include "ini_interface.h"
#include "mozart_config.h"
#include "alarm_interface.h"
#include "lcd_interface.h"
#if (SUPPORT_VR != VR_NULL)
#include "vr.h"
#endif

//after mozart_config.h
#if (SUPPORT_LAPSULE == 1)
#include "lapsule_control.h"
#endif

event_handler *e_handler = NULL;
char *app_name = NULL;
int tfcard_status = -1;
static int null_cnt = 0;

#if (SUPPORT_BT == BT_BCM)
int bsa_ble_start_regular_enable = 0;
int ble_service_create_enable = 0;
int ble_service_add_char_enable = 0;
int ble_service_start_enable = 0;
int ble_client_open_enable = 0;
int ble_client_read_enable = 0;
int ble_client_write_enable = 0;
static int bt_link_state = BT_LINK_DISCONNECTED;
#endif

static char *signal_str[] = {
	[1] = "SIGHUP",       [2] = "SIGINT",       [3] = "SIGQUIT",      [4] = "SIGILL",      [5] = "SIGTRAP",
	[6] = "SIGABRT",      [7] = "SIGBUS",       [8] = "SIGFPE",       [9] = "SIGKILL",     [10] = "SIGUSR1",
	[11] = "SIGSEGV",     [12] = "SIGUSR2",     [13] = "SIGPIPE",     [14] = "SIGALRM",    [15] = "SIGTERM",
	[16] = "SIGSTKFLT",   [17] = "SIGCHLD",     [18] = "SIGCONT",     [19] = "SIGSTOP",    [20] = "SIGTSTP",
	[21] = "SIGTTIN",     [22] = "SIGTTOU",     [23] = "SIGURG",      [24] = "SIGXCPU",    [25] = "SIGXFSZ",
	[26] = "SIGVTALRM",   [27] = "SIGPROF",     [28] = "SIGWINCH",    [29] = "SIGIO",      [30] = "SIGPWR",
	[31] = "SIGSYS",      [34] = "SIGRTMIN",    [35] = "SIGRTMIN+1",  [36] = "SIGRTMIN+2", [37] = "SIGRTMIN+3",
	[38] = "SIGRTMIN+4",  [39] = "SIGRTMIN+5",  [40] = "SIGRTMIN+6",  [41] = "SIGRTMIN+7", [42] = "SIGRTMIN+8",
	[43] = "SIGRTMIN+9",  [44] = "SIGRTMIN+10", [45] = "SIGRTMIN+11", [46] = "SIGRTMIN+12", [47] = "SIGRTMIN+13",
	[48] = "SIGRTMIN+14", [49] = "SIGRTMIN+15", [50] = "SIGRTMAX-14", [51] = "SIGRTMAX-13", [52] = "SIGRTMAX-12",
	[53] = "SIGRTMAX-11", [54] = "SIGRTMAX-10", [55] = "SIGRTMAX-9",  [56] = "SIGRTMAX-8", [57] = "SIGRTMAX-7",
	[58] = "SIGRTMAX-6",  [59] = "SIGRTMAX-5",  [60] = "SIGRTMAX-4",  [61] = "SIGRTMAX-3", [62] = "SIGRTMAX-2",
	[63] = "SIGRTMAX-1",  [64] = "SIGRTMAX",
};

static void usage(const char *app_name)
{
	printf("%s [-bsh] \n"
	       " -h     help (show this usage text)\n"
	       " -s/-S  TODO\n"
	       " -b/-B  run a daemon in the background\n", app_name);

	return;
}

char buf[16] = {};

void sig_handler(int signo)
{
	char cmd[64] = {};
	void *array[10];
	int size = 0;
	char **strings = NULL;
	int i = 0;

	printf("\n\n[%s: %d] mozart crashed by signal %s.\n", __func__, __LINE__, signal_str[signo]);

	printf("Call Trace:\n");
	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	if (strings) {
		for (i = 0; i < size; i++)
			printf ("  %s\n", strings[i]);
		free (strings);
	} else {
		printf("Not Found\n\n");
	}

	if (signo == SIGSEGV || signo == SIGBUS ||
	    signo == SIGTRAP || signo == SIGABRT) {
		sprintf(cmd, "cat /proc/%d/maps", getpid());
		printf("Process maps:\n");
		system(cmd);
	}

	printf("stop all services\n");
	stopall(-1);

	printf("unregister event manager\n");
	mozart_event_handler_put(e_handler);

	printf("unregister network manager\n");
	unregister_from_networkmanager();

	printf("unregister alarm manager\n");
	unregister_from_alarm_manager();

	share_mem_clear();
	share_mem_destory();

	exit(-1);
}

#if (SUPPORT_VR != VR_NULL)
#if (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_LONGPRESS)
int vr_flag = 0;
void *vr_longbutton_func(void *arg)
{
	vr_flag = 1;
	mozart_key_wakeup();
	vr_flag = 0;

	return NULL; // make compile happy.
}

int create_vr_longbutton_pthread()
{
	int ret = 0;
	pthread_t vr_longbutton_thread = 0;

	if(vr_flag == 0) {
		ret = pthread_create(&vr_longbutton_thread, NULL, vr_longbutton_func, NULL);
		if(ret != 0) {
			printf ("Create iflytek pthread failed: %s!\n", strerror(errno));
			return ret;
		}
		pthread_detach(vr_longbutton_thread);
	}

	return ret;
}
#endif
#endif

bool wifi_configing = false;
pthread_t wifi_config_pthread;
pthread_mutex_t wifi_config_lock = PTHREAD_MUTEX_INITIALIZER;

void *wifi_config_func(void *args)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	wifi_configing = true;

	mozart_stop();
	stopall(1);
	mozart_config_wifi();

	wifi_configing = false;

	return NULL; // make compile happy.
}

int create_wifi_config_pthread(void)
{
	pthread_mutex_lock(&wifi_config_lock);
	// cancle previous thread.
	if (wifi_configing)
		pthread_cancel(wifi_config_pthread);

	// enter wifi config mode.
	if (pthread_create(&wifi_config_pthread, NULL, wifi_config_func, NULL) == -1) {
		printf("Create wifi config pthread failed: %s.\n", strerror(errno));
		pthread_mutex_unlock(&wifi_config_lock);
		return -1;
	}
	pthread_detach(wifi_config_pthread);

	pthread_mutex_unlock(&wifi_config_lock);

	return 0;
}

bool wifi_switching = false;
pthread_t wifi_switch_pthread;
pthread_mutex_t wifi_switch_lock = PTHREAD_MUTEX_INITIALIZER;

void *wifi_switch_func(void *args)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	wifi_switching = true;

	mozart_stop();
	stopall(1);
	mozart_wifi_mode();

	wifi_switching = false;

	return NULL; // make compile happy.
}

int create_wifi_switch_pthread(void)
{
	pthread_mutex_lock(&wifi_switch_lock);
	// cancle previous thread.
	if (wifi_switching)
		pthread_cancel(wifi_switch_pthread);

	// enter wifi switch mode.
	if (pthread_create(&wifi_switch_pthread, NULL, wifi_switch_func, NULL) == -1) {
		printf("Create wifi switch pthread failed: %s.\n", strerror(errno));
		pthread_mutex_unlock(&wifi_switch_lock);
		return -1;
	}
	pthread_detach(wifi_switch_pthread);

	pthread_mutex_unlock(&wifi_switch_lock);

	return 0;
}

#if (SUPPORT_LOCALPLAYER == 1)
void tfcard_scan_1music_callback(void)
{
	tfcard_status = 1;
	mozart_stop_snd_source_play();
	mozart_play_key("native_mode");
	mozart_localplayer_start_playback();
	snd_source = SND_SRC_LOCALPLAY;
}

void tfcard_scan_done_callback(char *musiclist)
{
#if 0
	if (musiclist) {
		printf("musiclist:\n%s.\n", musiclist);
		free(musiclist);
	} else {
		printf("No music found.\n");
	}
#endif
}
#endif

static void key_bluetooth_handler(void)
{
#if (SUPPORT_BT == BT_BCM)
	int state = 0;

	state = mozart_bluetooth_hs_get_call_state();
	if (state == CALLSETUP_STATE_INCOMMING_CALL) {
		printf(">>>>>>>>>>>>>answer call>>>>>\n");
		mozart_bluetooth_hs_answer_call();
	} else if (state == CALL_STATE_LEAST_ONE_CALL_ONGOING) {
		printf(">>>>>>>>>>>>>hang up>>>>>\n");
		mozart_bluetooth_hs_hangup();
	}
#endif
}

/*******************************************************************************
 * long press
 *******************************************************************************/
enum key_long_press_state {
	KEY_LONG_PRESS_INVALID = 0,
	KEY_LONG_PRESS_WAIT,
	KEY_LONG_PRESS_CANCEL,
	KEY_LONG_PRESS_DONE,
};

struct input_event_key_info {
	char *name;
	int key_code;
	enum key_long_press_state state;
	pthread_t pthread;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int timeout_second;
	void (*handler)(bool long_press);
};

static void wifi_switch_handler(bool long_press)
{
	if (long_press)
		create_wifi_config_pthread();
	else
		create_wifi_switch_pthread();
}

static void volume_down_handler(bool long_press)
{
	if (long_press)
		mozart_previous_song();
	else
		mozart_volume_down();
}

static void volume_up_handler(bool long_press)
{
	if (long_press)
		mozart_next_song();
	else
		mozart_volume_up();
}

static void play_pause_handler(bool long_press)
{
	if (long_press)
		key_bluetooth_handler();
	else
		mozart_play_pause();
}

static struct input_event_key_info input_event_key_infos[] = {
	{
		.name = "wifi_switch_key",
		.key_code = KEY_MODE,
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.cond = PTHREAD_COND_INITIALIZER,
		.timeout_second = 1,
		.handler = wifi_switch_handler,
	},
	{
		.name = "volume_up_key",
		.key_code = KEY_VOLUMEUP,
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.cond = PTHREAD_COND_INITIALIZER,
		.timeout_second = 1,
		.handler = volume_up_handler,
	},
	{
		.name = "volume_down_key",
		.key_code = KEY_VOLUMEDOWN,
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.cond = PTHREAD_COND_INITIALIZER,
		.timeout_second = 1,
		.handler = volume_down_handler,
	},
	{
		.name = "play_pause_key",
		.key_code = KEY_PLAYPAUSE,
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.cond = PTHREAD_COND_INITIALIZER,
		.timeout_second = 1,
		.handler = play_pause_handler,
	},
};

static void *key_long_press_func(void *args)
{
	struct timeval now;
	struct timespec timeout;
	struct input_event_key_info *info = (struct input_event_key_info *)args;

	pthread_mutex_lock(&info->lock);
	if (info->state == KEY_LONG_PRESS_CANCEL) {
		info->state = KEY_LONG_PRESS_INVALID;
		printf("%s short press\n", info->name);
		if (info->handler)
			info->handler(false);
		pthread_mutex_unlock(&info->lock);
		return NULL;
	}

	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec + info->timeout_second;
	timeout.tv_nsec = now.tv_usec * 1000;

	pthread_cond_timedwait(&info->cond,
			       &info->lock, &timeout);

	if (info->state == KEY_LONG_PRESS_CANCEL) {
		info->state = KEY_LONG_PRESS_INVALID;
		printf("%s short press\n", info->name);
		if (info->handler)
			info->handler(false);
		pthread_mutex_unlock(&info->lock);
		return NULL;
	}

	info->state = KEY_LONG_PRESS_DONE;

	printf("%s long press\n", info->name);
	if (info->handler)
		info->handler(true);
	pthread_mutex_unlock(&info->lock);

	return NULL;
}

static void create_key_long_press_pthread(struct input_event_key_info *info)
{
	pthread_mutex_lock(&info->lock);
	if (info->state != KEY_LONG_PRESS_INVALID) {
		pthread_mutex_unlock(&info->lock);
		return ;
	}
	info->state = KEY_LONG_PRESS_WAIT;
	pthread_mutex_unlock(&info->lock);

	if (pthread_create(&info->pthread, NULL, key_long_press_func, (void *)info) == -1) {
		printf("Create key long press pthread failed: %s.\n", strerror(errno));
		return ;
	}
	pthread_detach(info->pthread);
}

static void key_long_press_cancel(struct input_event_key_info *info)
{
	pthread_mutex_lock(&info->lock);

	if (info->state == KEY_LONG_PRESS_DONE) {
		info->state = KEY_LONG_PRESS_INVALID;
	} else if (info->state == KEY_LONG_PRESS_WAIT) {
		info->state = KEY_LONG_PRESS_CANCEL;
		pthread_cond_signal(&info->cond);
	}

	pthread_mutex_unlock(&info->lock);
}

static int input_event_handler(struct input_event event)
{
	int i;
	struct input_event_key_info *info;

	for (i = 0; i < sizeof(input_event_key_infos) / sizeof(struct input_event_key_info); i++) {
		info = &input_event_key_infos[i];

		if (event.code == info->key_code) {
			if (event.value == 1)
				create_key_long_press_pthread(info);
			else
				key_long_press_cancel(info);

			return 0;
		}
	}

	return -1;
}

void event_callback(mozart_event event, void *param)
{
	switch (event.type) {
		case EVENT_KEY:
			if (event.event.key.key.type != EV_KEY) {
				printf("Only support keyboard now.\n");
				break;
			}
			printf("[DEBUG] key %s %s!!!\n",
			       keycode_str[event.event.key.key.code], keyvalue_str[event.event.key.key.value]);

			if (input_event_handler(event.event.key.key) == 0)
				break;

			if (event.event.key.key.value == 1) {
				switch (event.event.key.key.code)
				{
					case KEY_RECORD:
#if (SUPPORT_VR != VR_NULL)
#if (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_SHORTPRESS || SUPPORT_VR_WAKEUP == VR_WAKEUP_VOICE_KEY_MIX)
						mozart_key_wakeup();
#elif (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_LONGPRESS)
						if(create_vr_longbutton_pthread()) {  // in case of block.
							printf("create_iflytek_pthread failed!\n");
							break;
						}
#endif
#endif
						break;
					case KEY_F1:
						create_wifi_config_pthread(); // in case of block.
						break;
					case KEY_BLUETOOTH:
						key_bluetooth_handler();
						break;
					case KEY_PREVIOUSSONG:
						mozart_previous_song();
						break;
					case KEY_NEXTSONG:
						mozart_next_song();
						break;
					case KEY_MENU:
						mozart_snd_source_switch();
						break;
					case KEY_F3:            /* music music Shortcut key 1 */
						mozart_music_list(0);
						break;
					case KEY_F4:            /* music music Shortcut key 2 */
						mozart_music_list(1);
						break;
					case KEY_F5:            /* music music Shortcut key 3 */
						mozart_music_list(2);
						break;

					case KEY_POWER:
						mozart_power_off();
						break;
					default:
						//printf("UnSupport key down in %s:%s:%d.\n", __FILE__, __func__, __LINE__);
						break;
				}
			} else {
				switch (event.event.key.key.code) {
					case KEY_RECORD:
#if (SUPPORT_VR != VR_NULL)
#if (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_LONGPRESS)
						mozart_key_asr_record_end();
#endif
#endif
						break;
					default:
						break;
				}
			}
			break;
		case EVENT_MISC:
			if (!strcasecmp(event.event.misc.name, "linein")) {
				printf("[device hotplug event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type, "plugin")) { // linein plugin
#if (SUPPORT_LCD == 1)
					mozart_lcd_display(LINEIN_PNG_PATH,LINEIN_PLAY_CN);
#endif
					mozart_stop();
					mozart_linein_on();
#if (SUPPORT_LAPSULE == 1)
					lapsule_do_linein_on();
#endif
				} else if (!strcasecmp(event.event.misc.type, "plugout")) { // linein plugout
#if (SUPPORT_LCD == 1)
					mozart_lcd_display(LINEIN_PNG_PATH,LINEIN_DISCONNECT_CN);
#endif
					mozart_linein_off();
#if(SUPPORT_LAPSULE == 1)
					lapsule_do_linein_off();
#endif
				}
			} else if (!strcasecmp(event.event.misc.name, "tfcard")) {
				printf("[device hotplug event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type, "plugin")) { // tfcard plugin
					if (tfcard_status != 1) {
#if (SUPPORT_LOCALPLAYER == 1)
#if (SUPPORT_LCD == 1)
						mozart_lcd_display(TF_PNG_PATH,TF_PLAY_CN);
#endif
						snd_source = SND_SRC_LOCALPLAY;
#endif
						mozart_play_key("tf_add");
						tfcard_status = 1;
					} else {
						// do nothing.
					}
				} else if (!strcasecmp(event.event.misc.type, "plugout")) { // tfcard plugout
					if (tfcard_status != 0) {
#if (SUPPORT_LOCALPLAYER == 1)
#if (SUPPORT_LCD == 1)
						mozart_lcd_display(TF_PNG_PATH,TF_REMOVE_CN);
#endif
						mozart_localplayer_stop_playback();
#endif

						mozart_play_key("tf_remove");
						tfcard_status = 0;
					}
				}
			} else if (!strcasecmp(event.event.misc.name, "headset")) {
				printf("[device hotplug event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type, "plugin")) { // headset plugin
					printf("headset plugin.\n");
					// do nothing.
				} else if (!strcasecmp(event.event.misc.type, "plugout")) { // headset plugout
					printf("headset plugout.\n");
					// do nothing.
				}
			} else if (!strcasecmp(event.event.misc.name, "spdif")) {
				printf("[device hotplug event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type, "plugin")) { // spdif-out plugin
					printf("spdif plugin.\n");
					// do nothing.
				} else if (!strcasecmp(event.event.misc.type, "plugout")) { // spdif-out plugout
					printf("spdif plugout.\n");
					// do nothing.
				}
#if (SUPPORT_BT == BT_BCM)
			} else if (!strcasecmp(event.event.misc.name, "bluetooth")) {
				printf("[software protocol event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type, "hs_connected")) {
					if (mozart_bluetooth_get_link_status() == BT_LINK_CONNECTED) {
						if (bt_link_state == BT_LINK_DISCONNECTED) {
							bt_link_state = BT_LINK_CONNECTED;
							printf("hs_connected !!!!\n");
#if (SUPPORT_LCD == 1)
							mozart_lcd_display(BT_PNG_PATH, BT_PLAY_CN);
#endif
							mozart_stop();
							mozart_play_key("bluetooth_connect");
							snd_source = SND_SRC_BT_AVK;
						}
					}
				} else if (!strcasecmp(event.event.misc.type, "avk_connected")) {
					if (mozart_bluetooth_get_link_status() == BT_LINK_CONNECTED) {
						if (bt_link_state == BT_LINK_DISCONNECTED) {
							bt_link_state = BT_LINK_CONNECTED;
							printf("avk_connected !!!!\n");
#if (SUPPORT_LCD == 1)
							mozart_lcd_display(BT_PNG_PATH, BT_PLAY_CN);
#endif
							mozart_stop();

							mozart_play_key("bluetooth_connect");
							snd_source = SND_SRC_BT_AVK;
						}
					}
				} else if (!strcasecmp(event.event.misc.type, "hs_disconnected")) {
					if (mozart_bluetooth_get_link_status() == BT_LINK_DISCONNECTED) {
						if (bt_link_state == BT_LINK_CONNECTED) {
							bt_link_state = BT_LINK_DISCONNECTED;
							printf("hs_disconnected !!!\n");
							mozart_play_key("bluetooth_disconnect");
#if(SUPPORT_LCD == 1)
							mozart_lcd_display(BT_PNG_PATH, BT_DISCONNECT_CN);
#else
							;
#endif
						}
					}
				} else if (!strcasecmp(event.event.misc.type, "avk_disconnected")) {
					if (mozart_bluetooth_get_link_status() == BT_LINK_DISCONNECTED) {
						if (bt_link_state == BT_LINK_CONNECTED) {
							bt_link_state = BT_LINK_DISCONNECTED;
							printf("avk_disconnected !!!\n");
							mozart_play_key("bluetooth_disconnect");
#if(SUPPORT_LCD == 1)
							mozart_lcd_display(BT_PNG_PATH, BT_DISCONNECT_CN);
#else
							;
#endif
						}
					}
				} else if (!strcasecmp(event.event.misc.type, "connecting")) {
					; // do nothing if bluetooth device is connecting.
				} else if (!strcasecmp(event.event.misc.type, "disc_complete")) {
					printf("EVENT: disc_complete !\n");
					bsa_ble_start_regular_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "se_create")) {
					printf("EVENT: se_create complete !\n");
					ble_service_create_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "se_addchar")) {
					printf("EVENT: se_addchar complete !\n");
					ble_service_add_char_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "se_start")) {
					printf("EVENT: se_start complete !\n");
					ble_service_start_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "se_r_complete")) {
					printf("EVENT: se_read_complete !\n");
				} else if (!strcasecmp(event.event.misc.type, "se_w_complete")) {
					printf("EVENT: se_write_complete complete !\n");
				} else if (!strcasecmp(event.event.misc.type, "cl_open")) {
					printf("EVENT: cl_open complete !\n");
					ble_client_open_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "cl_read")) {
					printf("EVENT: cl_read complete !\n");
					ble_client_read_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "cl_write")) {
					printf("EVENT: cl_write complete !\n");
					ble_client_write_enable = 1;
				} else if (!strcasecmp(event.event.misc.type, "sec_link_down")) {
					printf("EVENT: sec_link_down complete !\n");
					/* Reason code 19: Mobile terminal to close bluetooth
					 * Reason code  8: connection timeout */
					UINT8 link_down_data = 0;
					link_down_data = mozart_bluetooth_get_link_down_data();
					if (link_down_data == 0x13)
						printf("Mobile terminal to close BT !\n");
					else if (link_down_data == 0x8)
						printf("BT connection timeout !\n");
				} else if (!strcasecmp(event.event.misc.type, "hs_ring_start")) {
					printf("EVENT: hs_ring_start complete !\n");
				} else if (!strcasecmp(event.event.misc.type, "hs_ring_hang_up")) {
					printf("EVENT: hs_ring_hang_up complete !\n");
				}
#endif
			} else if (!strcasecmp(event.event.misc.name, "dlna")) {
				printf("[software protocol event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type,"connected")) {
					; // do nothing on dlna device connected event
				} else if (!strcasecmp(event.event.misc.type,"disconnected")) {
					; // do nothing on dlna device connected event
				} else if (!strcasecmp(event.event.misc.type,"playurl")){
#if (SUPPORT_LCD == 1)
					mozart_lcd_display(WIFI_PNG_PATH,WIFI_PLAY_CN);
#else
					;
#endif
				}

			} else if (!strcasecmp(event.event.misc.name, "airplay")) {
				printf("[software protocol event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
				if (!strcasecmp(event.event.misc.type,"connected")) {
					; // do nothing on airplay device connected event
				} else if (!strcasecmp(event.event.misc.type,"disconnected")) {
					; // do nothing on airplay device disconnected event
				}
			}else if(!strcasecmp(event.event.misc.name,"vr")){
				printf("[vr event] %s.\n", event.event.misc.type);
#if (SUPPORT_LCD == 1)
				if(!strcasecmp(event.event.misc.type,"vr wake up")){
					mozart_lcd_display(NULL,VR_WAKE_UP_CN);
				}else if(!strcasecmp(event.event.misc.type,"vr failed")){
					mozart_lcd_display(NULL,VR_FAILED_CN);
				}else if(!strcasecmp(event.event.misc.type,"vr unclear")){
					mozart_lcd_display(NULL,VR_UNCLEAR_SPEECH_CN);
				}
#endif
			} else {
				printf("[misc event] %s : %s.\n", event.event.misc.name, event.event.misc.type);
			}
			break;
		default:
			break;
	}

	return;
}

int network_callback(const char *p)
{
	struct json_object *wifi_event = NULL;
	printf("\n\n[%s: %d] network event: %s\n\n", __func__, __LINE__, p);
	char test[64] = {0};
	wifi_ctl_msg_t new_mode;
	event_info_t network_event;
	wifi_event = json_tokener_parse(p);
	//printf("event.to_string()=%s\n", json_object_to_json_string(event));

	memset(&network_event, 0, sizeof(event_info_t));
	struct json_object *tmp = NULL;
	json_object_object_get_ex(wifi_event, "name", &tmp);
	if(tmp != NULL){
		strncpy(network_event.name, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
		//printf("name:%s\n", json_object_get_string(tmp));
	}

	json_object_object_get_ex(wifi_event, "type", &tmp);
	if(tmp != NULL){
		strncpy(network_event.type, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
		//printf("type:%s\n", json_object_get_string(tmp));
	}

	json_object_object_get_ex(wifi_event, "content", &tmp);
	if(tmp != NULL){
		strncpy(network_event.content, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
		//printf("content:%s\n", json_object_get_string(tmp));
	}

	memset(&new_mode, 0, sizeof(wifi_ctl_msg_t));
	if (!strncmp(network_event.type, event_type_str[STA_STATUS], strlen(event_type_str[STA_STATUS]))) {
		//printf("[%s]: %s\n", network_event.type, network_event.content);
		null_cnt = 0;
		if(!strncmp(network_event.content, "STA_CONNECT_STARTING", strlen("STA_CONNECT_STARTING"))){
#if (SUPPORT_LCD == 1)
			mozart_lcd_display(NULL,WIFI_CONNECTING_CN);
#endif
			mozart_play_key("wifi_linking");
		}

		if(!strncmp(network_event.content, "STA_CONNECT_FAILED", strlen("STA_CONNECT_FAILED"))){
			json_object_object_get_ex(wifi_event, "reason", &tmp);
			if(tmp != NULL){
				strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
				printf("STA_CONNECT_FAILED REASON:%s\n", json_object_get_string(tmp));
			}
			{
#if (SUPPORT_LCD == 1)
				mozart_lcd_display(NULL,WIFI_CONNECT_FAILED_CN);
#endif
				mozart_play_key("wifi_link_failed");
				new_mode.cmd = SW_NEXT_NET;
				new_mode.force = true;
				strcpy(new_mode.name, app_name);
				new_mode.param.network_config.timeout = -1;
				memset(new_mode.param.network_config.key, 0, sizeof(new_mode.param.network_config.key));
				if(request_wifi_mode(new_mode) != true)
					printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
			}
		}
		if(!strncmp(network_event.content, "STA_SCAN_OVER", strlen("STA_SCAN_OVER"))){
			new_mode.cmd = SW_NETCFG;
			new_mode.force = true;
			strcpy(new_mode.name, app_name);
			new_mode.param.network_config.timeout = -1;
			memset(new_mode.param.network_config.key, 0, sizeof(new_mode.param.network_config.key));
			new_mode.param.network_config.method |= 0x08;
			strcpy(new_mode.param.network_config.wl_module, wifi_module_str[BROADCOM]);
			if(request_wifi_mode(new_mode) != true)
				printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
		}
	} else if (!strncmp(network_event.type, event_type_str[NETWORK_CONFIGURE], strlen(event_type_str[NETWORK_CONFIGURE]))) {
		//printf("[%s]: %s\n", network_event.type, network_event.content);
		null_cnt = 0;
		if(!strncmp(network_event.content, network_configure_status_str[AIRKISS_STARTING], strlen(network_configure_status_str[AIRKISS_STARTING]))) {
#if (SUPPORT_LCD == 1)
			mozart_lcd_display(NULL,WIFI_CONFIG_CN);
#endif
			mozart_play_key("airkiss_config");
		}

		if(!strncmp(network_event.content, network_configure_status_str[AIRKISS_FAILED], strlen(network_configure_status_str[AIRKISS_FAILED]))) {
#if (SUPPORT_LCD == 1)
			mozart_lcd_display(NULL,WIFI_CONFIG_FAIL_CN);
#endif
			mozart_play_key("airkiss_config_fail");
			json_object_object_get_ex(wifi_event, "reason", &tmp);
			if(tmp != NULL){
				strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
				printf("NETWORK CONFIGURE REASON:%s\n", json_object_get_string(tmp));
			}
			//new_mode.cmd = SW_STA;
			new_mode.cmd = SW_AP;
			//new_mode.cmd = SW_NETCFG;
			new_mode.force = true;
			strcpy(new_mode.name, app_name);
			if(request_wifi_mode(new_mode) != true)
				printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
		} else if (!strncmp(network_event.content, network_configure_status_str[AIRKISS_SUCCESS], strlen(network_configure_status_str[AIRKISS_SUCCESS]))) {
#if (SUPPORT_LCD == 1)
			mozart_lcd_display(NULL,WIFI_CONFIG_SUC_CN);
#endif
			mozart_play_key("airkiss_config_success");
			{
#if 1
				json_object_object_get_ex(wifi_event, "ssid", &tmp);
				if(tmp != NULL){
					strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
					printf("ssid:%s\n", json_object_get_string(tmp));
				}
				json_object_object_get_ex(wifi_event, "passwd", &tmp);
				if(tmp != NULL){
					strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
					printf("passwd:%s\n", json_object_get_string(tmp));
				}
				json_object_object_get_ex(wifi_event, "ip", &tmp);
				if(tmp != NULL){
					strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
					printf("ip:%s\n", json_object_get_string(tmp));
				}
#endif
			}
		} else if(!strncmp(network_event.content, network_configure_status_str[AIRKISS_CANCEL], strlen(network_configure_status_str[AIRKISS_CANCEL]))) {
			mozart_play_key("airkiss_config_quit");
		}
	} else if (!strncmp(network_event.type, event_type_str[WIFI_MODE], strlen(event_type_str[WIFI_MODE]))) {
		//printf("[%s]: %s\n", network_event.type, network_event.content);
		json_object_object_get_ex(wifi_event, "last", &tmp);
		if(tmp != NULL){
			strncpy(test, json_object_get_string(tmp), strlen(json_object_get_string(tmp)));
			//printf("last:%s\n", json_object_get_string(tmp));
		}
		wifi_info_t infor = get_wifi_mode();
		mozart_system("killall ntpd > /dev/null 2>&1");
		mozart_system("killall dnsmasq > /dev/null 2>&1");
		//if(!strncmp(network_event.content, "AP", strlen("AP"))) {
		if (infor.wifi_mode == AP) {
			null_cnt = 0;
#if (SUPPORT_LCD == 1)
			mozart_lcd_display(NULL,WIFI_CONNECT_FAILED_CN);
#endif
			mozart_led_turn_off(LED_RECORD);
			mozart_led_turn_slow_flash(LED_WIFI);
			mozart_play_key("wifi_ap_mode");
			startall(1);
		} else if (infor.wifi_mode == STA) {
			//else if(!strncmp(network_event.content, "STA", strlen("STA"))) {
#if (SUPPORT_LCD == 1)
			mozart_lcd_display(NULL,WIFI_CONNECTED_CN);
#endif
			null_cnt = 0;
			mozart_led_turn_off(LED_RECORD);
			mozart_led_turn_on(LED_WIFI);
			mozart_play_key("wifi_sta_mode");

			// XXX
			// 1. sync network time(and update system time with CST format)
			// 2. write it to hardware RTC(with UTC format).
			mozart_system("ntpd -nq &");
			mozart_system("dnsmasq &");
			startall(1);
		} else if (infor.wifi_mode == WIFI_NULL) {
			//else if(!strncmp(network_event.content, "WIFI_NULL", strlen("WIFI_NULL"))) {
			null_cnt++;
			if(null_cnt >= 10){
				null_cnt = 0;
				new_mode.cmd = SW_NETCFG;
				new_mode.force = true;
				strcpy(new_mode.name, app_name);
				new_mode.param.network_config.timeout = -1;
				memset(new_mode.param.network_config.key, 0, sizeof(new_mode.param.network_config.key));
				new_mode.param.network_config.method |= 0x08;
				strcpy(new_mode.param.network_config.wl_module, wifi_module_str[BROADCOM]);
				if(request_wifi_mode(new_mode) != true)
					printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
			}
		}
	} else {
		printf("[ERROR]: Unknown event type!!!!!!\n");
	}

	json_object_put(wifi_event);
	return 0;
}

int alarm_callback(const char *p , Alarm a)
{

	printf("alarm callback to do %s \n",p);
	printf("alarm after send message ====> id: %d,  hour: %d, minute: %d,  timestamp: %ld url is:%s\n",
	       a.alarm_id,  a.hour, a.minute,
	       a.timestamp, a.programUrl);

	return 0;
}

void *module_switch_func(void *args)
{
	module_status status;

	while (1) {
		usleep(100000);

		if(share_mem_get(AIRPLAY_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_STOPPING) {
			printf("[%s:%s:%d] airplay will stop playback.\n", __FILE__, __func__, __LINE__);
			continue;
		}

#if (SUPPORT_DMR == 1)
		if(share_mem_get(RENDER_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_STOPPING) {
			if (mozart_render_stop_playback()) {
				printf("stop render playback error in %s:%s:%d, continue.\n",
				       __FILE__, __func__, __LINE__);
				continue;
			}
		}
#endif

#if (SUPPORT_LOCALPLAYER == 1)
		if(share_mem_get(LOCALPLAYER_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_STOPPING) {
			if (mozart_localplayer_stop_playback()) {
				printf("stop localplayer playback error in %s:%s:%d, continue.\n",
				       __FILE__, __func__, __LINE__);
				continue;
			}
		}
#endif

#if (SUPPORT_ATALK == 1)
		if(share_mem_get(ATALK_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_STOPPING) {
			if (mozart_atalk_stop()) {
				printf("stop atalk error in %s:%s:%d, continue.\n",
				       __FILE__, __func__, __LINE__);
				continue;
			}
		}
#endif

#if (SUPPORT_VR != VR_NULL)
		int ret = 0;
		if(share_mem_get(VR_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_STOPPING) {
#if (SUPPORT_VR == VR_BAIDU)
			ret = mozart_vr_stop();
#elif (SUPPORT_VR == VR_SPEECH)
#ifdef SUPPORT_SONG_SUPPLYER
			switch (SUPPORT_SONG_SUPPLYER) {
			case SONG_SUPPLYER_XIMALAYA:
				ximalaya_stop();
				break;
			case SONG_SUPPLYER_LAPSULE:
#if (SUPPORT_LAPSULE == 1)
				lapsule_do_stop_play();
#endif
				break;
			default:
				break;
			}
#endif
			ret = 0;
#elif (SUPPORT_VR == VR_IFLYTEK)
			printf("TODO: vr iflytek stop music.\n");
			ret = 0;
#elif (SUPPORT_VR == VR_UNISOUND)
			printf("TODO: vr unisound stop music.\n");
			ret = 0;
#endif

			if (ret) {
				printf("stop vr playback error in %s:%s:%d, continue.\n",
				       __FILE__, __func__, __LINE__);
				continue;
			}
		}
#endif

#if (SUPPORT_BT != BT_NULL)
		int cnt = 0;

		if(share_mem_get(BT_HS_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_BT_HS_AUD_REQ) {
#if (SUPPORT_VR != VR_NULL)
#if (SUPPORT_VR == VR_BAIDU)
			if (mozart_vr_get_status()) {
				printf("invoking mozart_vr_shutdown()\n");
				mozart_vr_shutdown();
			}
#elif (SUPPORT_VR == VR_SPEECH)
			if (mozart_vr_speech_get_status()) {
				printf("invoking mozart_vr_speech_shutdown()\n");
				mozart_vr_speech_shutdown();
			}
#elif (SUPPORT_VR == VR_IFLYTEK)
			if (mozart_vr_iflytek_get_status()) {
				printf("invoking mozart_vr_iflytek_shutdown()\n");
				mozart_vr_iflytek_shutdown();
			}
#elif (SUPPORT_VR == VR_UNISOUND)
			printf("TODO: shutdown vr unisound service.\n");
#elif (SUPPORT_VR == VR_ATALK)
			if (mozart_vr_atalk_get_status()) {
				printf("invoking mozart_vr_atalk_shutdown()\n");
				mozart_vr_atalk_shutdown();
			}
#endif	/* SUPPORT_VR == VR_XXX */
#endif	/* SUPPORT_VR != VR_NULL */
			if(AUDIO_OSS == get_audio_type()){
				cnt = 50;
				while (cnt--) {
					if (check_dsp_opt(O_WRONLY))
						break;
					usleep(100 * 1000);
				}
				if (cnt < 0) {
					printf("5 seconds later, /dev/dsp is still in use(writing), try later!!\n");
					continue;
				}
			}
#if (SUPPORT_VR != VR_NULL)
			if(AUDIO_OSS == get_audio_type()){
				cnt = 30;
				while (cnt--) {
					if (check_dsp_opt(O_RDONLY))
						break;
					usleep(100 * 1000);
				}
				if (cnt < 0) {
					printf("3 seconds later, /dev/dsp is still in use(reading), try later!!\n");
					continue;
				}
			}
#endif

			printf("You can answer the phone now.\n");
			//ensure dsp write _AND_ read is idle in here.
			if(share_mem_set(BT_HS_DOMAIN, STATUS_BT_HS_AUD_RDY))
				printf("share_mem_set failure.\n");
		}
#if (SUPPORT_VR != VR_NULL)
		else if (status == STATUS_RUNNING) {

			int wakeup_mode_mark = 0;
#if (SUPPORT_VR_WAKEUP == VR_WAKEUP_VOICE)
			wakeup_mode_mark = VOICE_WAKEUP;
#elif (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_SHORTPRESS)
			wakeup_mode_mark = KEY_SHORTPRESS_WAKEUP;
#elif (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_LONGPRESS)
			wakeup_mode_mark = KEY_LONGPRESS_WAKEUP;
#endif

#if (SUPPORT_VR == VR_BAIDU)
			if(!mozart_vr_get_status()) {
				printf("invoking mozart_vr_startup()\n");
				mozart_vr_startup();
			}
#elif (SUPPORT_VR == VR_SPEECH)
			if(!mozart_vr_speech_get_status()) {
				printf("invoking mozart_vr_speech_startup()\n");
				mozart_vr_speech_startup(wakeup_mode_mark, mozart_vr_speech_interface_callback);
			}
#elif (SUPPORT_VR == VR_IFLYTEK)
			if(!mozart_vr_iflytek_get_status()) {
				printf("invoking mozart_vr_iflytek_startup()\n");
				mozart_vr_iflytek_startup(wakeup_mode_mark, mozart_vr_iflytek_interface_callback);
			}
#elif (SUPPORT_VR == VR_UNISOUND)
			printf("TODO: restart vr unisound service.\n");
#elif (SUPPORT_VR == VR_ATALK)
			if (!mozart_vr_atalk_get_status()) {
				printf("invoking mozart_vr_atalk_startup()\n");
				mozart_vr_atalk_startup(wakeup_mode_mark, mozart_vr_atalk_interface_callback);
			}
#endif	/* SUPPORT_VR == VR_XXX */
		}
#endif	/* SUPPORT_VR != VR_NULL */
#if (SUPPORT_BT == BT_BCM)
		if(share_mem_get(BT_AVK_DOMAIN, &status))
			printf("share_mem_get failure.\n");
		if (status == STATUS_BT_AVK_AUD_REQ) {
			if(AUDIO_OSS == get_audio_type()){
				cnt = 50;
				while (cnt--) {
					if (check_dsp_opt(O_WRONLY))
						break;
					usleep(100 * 1000);
				}
				if (cnt < 0) {
					printf("5 seconds later, /dev/dsp is still in use(writing), try later!!\n");
					continue;
				}
			}

			printf("You can play music now.\n");
			//ensure dsp write _AND_ read is idle in here.
			if(share_mem_set(BT_AVK_DOMAIN, STATUS_BT_AVK_AUD_RDY))
				printf("share_mem_set failure.\n");
		} else if (status == STATUS_STOPPING) {
			mozart_bluetooth_avk_stop_play();
		}
#endif
#endif
	}

	return NULL;
}

static int initall(void)
{
	// recover volume in S01system_init.sh

	//share memory init.
	if(0 != share_mem_init()){
		printf("share_mem_init failure.\n");
	}
	if(0 != share_mem_clear()){
		printf("share_mem_clear failure.\n");
	}

	if (mozart_path_is_mount("/mnt/sdcard")) {
		tfcard_status = 1;
		share_mem_set(SDCARD_DOMAIN, STATUS_INSERT);
	} else {
		share_mem_set(SDCARD_DOMAIN, STATUS_EXTRACT);
	}

	if (mozart_path_is_mount("/mnt/usb"))
		share_mem_set(UDISK_DOMAIN, STATUS_INSERT);
	else
		share_mem_set(UDISK_DOMAIN, STATUS_EXTRACT);

	// shairport ini_interface.hinit(do not depend network part)

	// TODO: render init if needed.

	// TODO: localplayer init if needed.

	// TODO: bt init if needed.

	// TODO: vr init if needed.

	// TODO: other init if needed.

	return 0;
}

#if 0
void check_mem(int line)
{
	return ;
	pid_t pid = getpid();
	time_t timep;
	struct tm *p;
	FILE *fp;

	char rss[8] = {};
	char cmd[256] = {};
	char path[256] = {};

	// get current time.
	time(&timep);
	p=gmtime(&timep);


	printf("capture %d's mem info to %s.\n", pid, path);

	// get rss
	fp = popen("/bin/ps wl | grep mozart | grep -v grep | tr -s ' ' | cut -d' ' -f6", "r");
	if (!fp) {
		strcpy(rss, "99999");
	} else {
		fgets(rss, 5, fp);
		fclose(fp);
	}

	// capture exmap info.
	sprintf(path, "/tmp/%06d_%04d%02d%02d_%02d:%02d:%02d_%04d_%s.exmap",
		pid, (1900+p->tm_year), (1+p->tm_mon),p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, line, rss);

	sprintf(cmd, "echo %d > /proc/exmap; cat /proc/exmap > %s", pid, path);
	system(cmd);

	sprintf(cmd, "cat /proc/%d/maps >> %s", pid, path);

	system(cmd);
}
#endif

static void dump_compile_info(void)
{
	time_t timep;
	struct passwd *pwd;
	char hostname[16] = "Unknown";

	time(&timep);
	pwd = getpwuid(getuid());
	gethostname(hostname, 16);
	printf("mozart compiled at %s on %s@%s\n", asctime(gmtime(&timep)), pwd->pw_name, hostname);
}

int main(int argc, char **argv)
{
	int daemonize = 0;

	app_name = argv[0];
	wifi_ctl_msg_t new_mode;
	struct wifi_client_register wifi_info;
	struct alarm_client_register alarm_info;
	pthread_t module_switch_thread;

	/* Get command line parameters */
	int c;
	while (1) {
		c = getopt(argc, argv, "bBsSh");
		if (c < 0)
			break;
		switch (c) {
		case 'b':
		case 'B':
			daemonize = 1;
			break;
		case 's':
		case 'S':
			break;
		case 'h':
			usage(app_name);
			return 0;
		default:
			usage(app_name);
			return 1;
		}
	}

	/* run in the background */
	if (daemonize) {
		if (daemon(0, 1)) {
			perror("daemon");
			return -1;
		}
	}
	dump_compile_info();

#if (SUPPORT_LOCALPLAYER == 1)
	mozart_localplayer_scan_callback(tfcard_scan_1music_callback, tfcard_scan_done_callback);
#endif
	initall();

	// start modules do not depend network.
	startall(0);
	register_battery_capacity();

	// register key event
	e_handler = mozart_event_handler_get(event_callback, app_name);

	// register network manager
	wifi_info.pid = getpid();
	wifi_info.reset = 1;
	wifi_info.priority = 3;
	strcpy(wifi_info.name, app_name);
	if(register_to_networkmanager(wifi_info, network_callback) != 0) {
		printf("ERROR: [%s] register to Network Server Failed!!!!\n", app_name);
	} else if(!access("/usr/data/wpa_supplicant.conf", R_OK)) {
		memset(&new_mode, 0, sizeof(new_mode));
		new_mode.cmd = SW_STA;
		new_mode.force = true;
		strcpy(new_mode.name, app_name);
		if(request_wifi_mode(new_mode) != true)
			printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
	}

	//register alarm manager
	alarm_info.pid = getpid();
	alarm_info.reset = 1;
	alarm_info.priority = 3;
	strcpy(alarm_info.name, app_name);
	register_to_alarm_manager(alarm_info,alarm_callback);
	// switch render, airplay, localplayer, bt_audio, voice_recogition
	if (pthread_create(&module_switch_thread, NULL, module_switch_func, NULL) != 0) {
		printf("Can't create module_switch_thread in %s:%s:%d: %s\n",__FILE__, __func__, __LINE__, strerror(errno));
		exit(1);
	}
	pthread_detach(module_switch_thread);

	// process linein insert event on startup.
	if (mozart_linein_is_in()) {
		printf("linein detect, switch to linein mode...\n");
		mozart_linein_on();
	}

	// signal hander,
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGABRT, sig_handler);
	signal(SIGPIPE, SIG_IGN);

	while(1)
		sleep(60);

	return 0;
}
