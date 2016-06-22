#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <json-c/json.h>
#include <stdbool.h>

#include "atalk_control.h"
#include "wifi_interface.h"
#include "tips_interface.h"
#include "utils_interface.h"
#include "player_interface.h"
#include "volume_interface.h"
#include "sharememory_interface.h"
#include "_lcd_show.h"
#include "lcd_interface.h"

#define APP_PATH "/var/run/doug/hub_app.sock"
#define HOST_PATH "/var/run/doug/hub_host.sock"

#define ATALK_CTL_DEBUG
#ifdef ATALK_CTL_DEBUG
#define pr_debug(fmt, args...)			\
	printf(fmt, ##args)

#define atalk_dump()							\
	printf("#### {%s, %s, %d} state: %s_%s, play_state: %s ####\n",	\
	       __FILE__, __func__, __LINE__,				\
	       __atalk_is_attaching() ? "ATTACH" : "UNATTACH",		\
	       __atalk_is_online() ? "ONLINE" : "OFFLINE",		\
	       atalk_play_state_str[atalk.play_state])
#else
#define pr_debug(fmt, args...)			\
	do {} while (0)
#define atalk_dump()				\
	do {} while (0)
#endif
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct atalk_method {
	const char *name;
	int (*handler)(json_object *cmd);
	bool (*is_valid)(json_object *cmd);
};

static bool atalk_initialized;
static player_handler_t *atalk_player_handler;
static pthread_mutex_t atalk_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *atalk_play_state_str[] = {
	"ATALK_PLAY_IDLE",
	"ATALK_PLAY_RUN",
	"ATALK_PLAY_PLAY",
	"ATALK_PLAY_PAUSE",
};

enum atalk_state_bit {
	ATALK_STATE_NET_BIT = 0,		/* reserver */
	ATALK_STATE_ATTACH_BIT,
	ATALK_STATE_ONLINE_BIT,
};

/* effectivity if state is ATALK_STATE_ATTACH_ONLINE */
enum atalk_play_state {
	ATALK_PLAY_IDLE = 0,
	ATALK_PLAY_RUN,
	ATALK_PLAY_PLAY,
	ATALK_PLAY_PAUSE,
};

/* sharememroy:
 * shutdown: atalk app stop
 * running: switch mode false(UNATTACH)
 * pause: stop or pause (ATTACH)
 * playing: play (ATTACH)
 */

struct atalk_struct {
	/* socket */
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;
	int server_sockfd;
	int client_sockfd;
	/* state */
	enum atalk_play_state play_state;
	unsigned int state_map;
} atalk = {
	.server_sockfd = -1,
	.client_sockfd = -1,
	.play_state = ATALK_PLAY_IDLE,
	.state_map = (1 << ATALK_STATE_ATTACH_BIT) | (0 << ATALK_STATE_ONLINE_BIT),
};

static void __pause_handler(void);
static void __stop_handler(void);
static int __switch_mode_handler(bool attach);
static int mozart_player_sync(play_status_t status);

static inline bool __atalk_is_attaching(void)
{
	return atalk.state_map & (1 << ATALK_STATE_ATTACH_BIT);
}

static inline bool __atalk_is_online(void)
{
	return atalk.state_map & (1 << ATALK_STATE_ONLINE_BIT);
}

static inline void __atalk_set_attaching(void)
{
	atalk.state_map |= (1 << ATALK_STATE_ATTACH_BIT);
}

static inline void __atalk_clear_attaching(void)
{
	atalk.state_map &= ~(1 << ATALK_STATE_ATTACH_BIT);
}

static inline void __atalk_set_online(void)
{
	atalk.state_map |= (1 << ATALK_STATE_ONLINE_BIT);
}

static inline void __atalk_set_offline(void)
{
	atalk.state_map &= ~(1 << ATALK_STATE_ONLINE_BIT);
}

static inline bool __atalk_can_play(void)
{
	return __atalk_is_online() && __atalk_is_attaching();
}

static bool __atalk_app_is_stop(void)
{
	module_status status;

	if (share_mem_get(ATALK_DOMAIN, &status)) {
		printf("%s: share_mem_get fail!\n", __func__);
		return -1;
	}

	if (status == STATUS_SHUTDOWN)
		return 1;
	else
		return 0;
}

static int __atalk_prepare_play(void)
{
	if (!__atalk_can_play())
		return -1;

	share_mem_stop_other(ATALK_DOMAIN);
	/* running/shutdown -> pause */
	if (share_mem_set(ATALK_DOMAIN, STATUS_PAUSE))
		printf("%s: share_mem_set fail.\n", __func__);
	atalk.play_state = ATALK_PLAY_IDLE;

	    return 0;
}

static void __atalk_stop_play(void)
{
	if (atalk.play_state != ATALK_PLAY_IDLE) {
		/* Quick Stop */
		__stop_handler();
		mozart_player_sync(STOP_STATUS);
	}
	if (share_mem_set(ATALK_DOMAIN, STATUS_RUNNING))
		printf("%s: share_mem_set fail.\n", __func__);
}

/*******************************************************************************
 * hub send/recv
 *******************************************************************************/
static int hub_init(void)
{
	int sockfd;
	struct sockaddr_un *un;

	/* server */
	un = &atalk.server_addr;
	un->sun_family = AF_UNIX;
	strcpy(un->sun_path, HOST_PATH);
	unlink(HOST_PATH);
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		printf("%s: socket fail: %s\n", __func__, strerror(errno));
		return -1;
	}
	if (bind(sockfd, (struct sockaddr *)un, sizeof(struct sockaddr_un))) {
		printf("%s: bind fail: %s\n", __func__, strerror(errno));
		close(sockfd);
		return -1;
	}
	atalk.server_sockfd = sockfd;

	/* client */
	un = &atalk.client_addr;
	un->sun_family = AF_UNIX;
	strcpy(un->sun_path, APP_PATH);
	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		printf("%s: socket fail: %s\n", __func__, strerror(errno));
		close(atalk.server_sockfd);
		atalk.server_sockfd = -1;
		close(sockfd);
		return -1;
	}
	atalk.client_sockfd = sockfd;

	return 0;
}

static void hub_destory(void)
{
	close(atalk.server_sockfd);
	close(atalk.client_sockfd);
	atalk.server_sockfd = -1;
	atalk.client_sockfd = -1;
	unlink(HOST_PATH);
}

static inline int hub_recv(char *buffer, size_t len)
{
	return recv(atalk.server_sockfd, buffer, len, 0);
}

static inline int hub_send(char *buffer, size_t len)
{
	return sendto(atalk.client_sockfd, buffer, len, 0,
		      (struct sockaddr *)&atalk.client_addr,
		      sizeof(struct sockaddr_un));
}

#define send_result_obj(cmd, result_obj)	\
	send_obj(cmd, NULL, result_obj)
#define send_notification_obj(method, method_obj)	\
	send_obj(NULL, method, method_obj)
static int send_obj(json_object *cmd, char *method, json_object *obj)
{
	const char *s;
	json_object *o, *m, *id;

	o = json_object_new_object();
	json_object_object_add(o, "jsonrpc", json_object_new_string("2.0"));
	if (cmd && json_object_object_get_ex(cmd, "id", &id)) {
		/* result */
		json_object_object_get_ex(cmd, "method", &m);
		json_object_object_add(o, "method", m);
		json_object_object_add(o, "result", obj);
		json_object_object_add(o, "id", id);

	} else if (method) {
		/* notification */
		json_object_object_add(o, "method", json_object_new_string(method));
		json_object_object_add(o, "params", obj);
	} else {
		json_object_put(o);
		printf("%s: send fail!\n", __func__);
		return -1;
	}

	s = json_object_to_json_string(o);
	pr_debug("<<<< %s: %s: %s\n", __func__, id ? "Result" : "Notifcation", s);
	hub_send((char *)s, strlen(s));

	json_object_put(o);

	return 0;
}

enum player_state {
	player_play_state = 0,
	player_pause_state,
	player_stop_state,
};

static int send_player_state_change(enum player_state state)
{
	json_object *params;

	params = json_object_new_object();
	json_object_object_add(params, "state", json_object_new_int(state));

	if (send_notification_obj("player_state_change", params)) {
		json_object_put(params);
		return -1;
	}

	return 0;
}

enum wifi_state {
	wifi_none = -1,
	wifi_start,
	wifi_end_ok,
	wifi_end_fail,
};

static int send_wifi_state_change(enum wifi_state state)
{
	json_object *params;

	params = json_object_new_object();
	json_object_object_add(params, "state", json_object_new_int(state));

	if (send_notification_obj("setup_wifi_state_change", params)) {
		json_object_put(params);
		return -1;
	}

	return 0;
}

static int __send_play_done(const char *url, int error_no)
{
	json_object *params, *error = NULL;

	params = json_object_new_object();
	json_object_object_add(params, "uri", json_object_new_string(url));
	if (error_no) {
		error = json_object_new_object();
		json_object_object_add(error, "code", json_object_new_int(error_no));

		json_object_object_add(params, "status", json_object_new_int(1));
		json_object_object_add(params, "error", error);
	} else {
		json_object_object_add(params, "status", json_object_new_int(0));
	}

	if (send_notification_obj("play_done", params)) {
		if (error)
			json_object_put(error);
		json_object_put(params);
		return -1;
	}

	atalk.play_state = ATALK_PLAY_IDLE;

	return 0;
}

static int send_player_volume_change(int volume)
{
	json_object *params;

	params = json_object_new_object();
	json_object_object_add(params, "volume", json_object_new_int(volume));

	if (send_notification_obj("player_volume_change", params)) {
		json_object_put(params);
		return -1;
	}

	return 0;
}

static int send_button_event(char *name, char *str, char *value)
{
	json_object *params;

	params = json_object_new_object();
	json_object_object_add(params, "name", json_object_new_string(name));
	if (str && value)
		json_object_object_add(params, str, json_object_new_string(value));

	if (send_notification_obj("button", params)) {
		json_object_put(params);
		return -1;
	}

	return 0;
}

/*******************************************************************************
 * handler
 *******************************************************************************/
static bool atalk_is_attaching(json_object *cmd)
{
	return __atalk_is_attaching();
}

static int play_handler(json_object *cmd)
{
	struct ft f;
	const char *url;
	int show_url = 0;
	json_object *params, *uri, *artist, *title, *vendor;

	if (!json_object_object_get_ex(cmd, "params", &params))
		return -1;
	if (!json_object_object_get_ex(params, "uri", &uri))
		return -1;
	if (!json_object_object_get_ex(params, "title", &title))
		return -1;
	if (!json_object_object_get_ex(params, "artist", &artist))
		return -1;
	if (!json_object_object_get_ex(params, "vendor", &vendor))
		return -1;

	url = json_object_get_string(uri);

	pthread_mutex_lock(&atalk_state_mutex);
	if (!__atalk_is_online())
		printf("[Warning] %s: state_map = %d\n", __func__, atalk.state_map);
	if (!__atalk_is_attaching())
		__switch_mode_handler(true);
	if (share_mem_set(ATALK_DOMAIN, STATUS_PLAYING))
		printf("%s: share_mem_set fail.\n", __func__);
	atalk.play_state = ATALK_PLAY_RUN;
	send_player_state_change(player_play_state);

	printf("    url: %s\n", url);
	if (url[0] == '/' && access(url, R_OK)) {
		__send_play_done(url, 0);
	} else if (!__atalk_is_online()) {
		__send_play_done(url, 0);
	} else {
		player_play_url(atalk_player_handler, (char *)url);
		show_url = 1;
	}
	/* TODO */
	/* if (atalk.play_state == ATALK_PLAY_PAUSE) */
	pthread_mutex_unlock(&atalk_state_mutex);

	if (show_url) {
		ft_init(&f);
		lcd_clear(&f);
		print_fb(&f, (unsigned char *)json_object_get_string(vendor), 16, 16, 50);
		print_fb(&f, (unsigned char *)json_object_get_string(artist), 16, 58, 92);
		print_fb(&f, (unsigned char *)json_object_get_string(title), 16, 120, 280);
		fb_close(&f);
	}

	return 0;
}

static void __stop_handler(void)
{
	player_status_t *player_status;

	if (atalk.play_state == ATALK_PLAY_IDLE)
		return ;

	/* If ATALK_DOMAIN set STOPPING, will call switch_mode. */
	if (share_mem_set(ATALK_DOMAIN, STATUS_PAUSE))
		printf("%s: share_mem_set fail.\n", __func__);
	atalk.play_state = ATALK_PLAY_IDLE;
	send_player_state_change(player_stop_state);

	player_status = player_get_player_status(atalk_player_handler);
	if (player_status &&
	    !strcmp(atalk_player_handler->uuid, player_status->uuid))
		player_stop_playback(atalk_player_handler);
	else
		pr_debug("[Warning] %s: player uuid: %s\n",
			 __func__, player_status->uuid ? : "NULL");
}

static int stop_handler(json_object *cmd)
{
	pthread_mutex_lock(&atalk_state_mutex);
	__stop_handler();
	pthread_mutex_unlock(&atalk_state_mutex);

	return 0;
}

static void __pause_handler(void)
{
	player_status_t *player_status;

	if (share_mem_set(ATALK_DOMAIN, STATUS_PAUSE))
		printf("%s: share_mem_set fail.\n", __func__);
	atalk.play_state = ATALK_PLAY_PAUSE;
	send_player_state_change(player_pause_state);

	player_status = player_get_player_status(atalk_player_handler);
	if (player_status &&
	    !strcmp(atalk_player_handler->uuid, player_status->uuid))
		player_do_pause(atalk_player_handler);
	else
		pr_debug("[Warning] %s: player uuid: %s\n",
			 __func__, player_status->uuid ? : "NULL");
}

static int pause_handler(json_object *cmd)
{
	pthread_mutex_lock(&atalk_state_mutex);
	__pause_handler();
	pthread_mutex_unlock(&atalk_state_mutex);

	return 0;
}

static void __resume_handler(void)
{
	player_status_t *player_status;

	if (share_mem_set(ATALK_DOMAIN, STATUS_PLAYING))
		printf("%s: share_mem_set fail.\n", __func__);
	atalk.play_state = ATALK_PLAY_PLAY;
	send_player_state_change(player_play_state);

	player_status = player_get_player_status(atalk_player_handler);
	if (player_status &&
	    !strcmp(atalk_player_handler->uuid, player_status->uuid))
		player_do_resume(atalk_player_handler);
	else
		pr_debug("[Warning] %s: player uuid: %s\n",
			 __func__, player_status->uuid ? : "NULL");
}

static int resume_handler(json_object *cmd)
{
	pthread_mutex_lock(&atalk_state_mutex);
	if (!__atalk_is_online())
		printf("[Warning] %s: state_map = %d\n", __func__, atalk.state_map);
	if (!__atalk_is_attaching()) {
		__switch_mode_handler(true);
		/* TODO */
		__send_play_done("NULL", 0);
	}

	__resume_handler();
	pthread_mutex_unlock(&atalk_state_mutex);

	return 0;
}

static int pause_toogle_handler(json_object *cmd)
{
	pthread_mutex_lock(&atalk_state_mutex);
	if (atalk.play_state == ATALK_PLAY_PAUSE)
		__resume_handler();
	else if (atalk.play_state == ATALK_PLAY_RUN ||
		 atalk.play_state == ATALK_PLAY_PLAY)
		__pause_handler();
	pthread_mutex_unlock(&atalk_state_mutex);

	return 0;
}

static int set_volume_handler(json_object *cmd)
{
	json_object *params, *volume;

	if (!json_object_object_get_ex(cmd, "params", &params))
		return -1;

	if (!json_object_object_get_ex(params, "volume", &volume))
		return -1;

	mozart_volume_set(json_object_get_int(volume), MUSIC_VOLUME);

	return 0;
}

static int net_state_change_handler(json_object *cmd)
{
	json_object *params, *state;

	if (!json_object_object_get_ex(cmd, "params", &params))
		return -1;

	if (!json_object_object_get_ex(params, "state", &state))
		return -1;

	if (json_object_get_int(state) == 0) {
		printf("[Warning] %s: net_state_change.\n", __func__);
#if 0
		/* TODO: Need ? */
		stop_handler(cmd);
		mozart_player_sync(STOP_STATUS);
#endif
	}

	return 0;
}

static int is_attaching_handler(json_object *cmd)
{
	json_object *result = json_object_new_object();

	pthread_mutex_lock(&atalk_state_mutex);
	if (__atalk_is_attaching())
		json_object_object_add(result, "attach", json_object_new_string("true"));
	else
		json_object_object_add(result, "attach", json_object_new_string("false"));
	if (send_result_obj(cmd, result)) {
		json_object_put(result);
		pthread_mutex_unlock(&atalk_state_mutex);
		return -1;
	} else {
		pthread_mutex_unlock(&atalk_state_mutex);
		return 0;
	}
}

static int get_mac_address_handler(json_object *cmd)
{
	char macaddr[] = "000.000.000.000";
	json_object *result = json_object_new_object();

	get_mac_addr("wlan0", macaddr, NULL);
	json_object_object_add(result, "mac_address",
			       json_object_new_string(macaddr));

	if (send_result_obj(cmd, result)) {
		json_object_put(result);
		return -1;
	} else {
		return 0;
	}
}

static int get_setup_wifi_handler(json_object *cmd)
{
	json_object *result = json_object_new_object();

	pthread_mutex_lock(&atalk_state_mutex);
	if (__atalk_is_online())
		json_object_object_add(result, "state", json_object_new_int(1));
	else
		json_object_object_add(result, "state", json_object_new_int(0));
	if (send_result_obj(cmd, result)) {
		json_object_put(result);
		pthread_mutex_unlock(&atalk_state_mutex);
		return -1;
	} else {
		pthread_mutex_unlock(&atalk_state_mutex);
		return 0;
	}
}

static int get_volume_handler(json_object *cmd)
{
	char volume[8];
	json_object *result = json_object_new_object();

	sprintf(volume, "%d", mozart_volume_get());
	json_object_object_add(result, "volume", json_object_new_string(volume));

	if (send_result_obj(cmd, result)) {
		json_object_put(result);
		return -1;
	} else {
		return 0;
	}
}

static int __switch_mode_handler(bool attach)
{
	int ret = 0;

	if (attach)
		__atalk_set_attaching();
	else
		__atalk_clear_attaching();

	if (__atalk_app_is_stop())
		return -1;

	if (attach) {
		__atalk_prepare_play();
		ret = send_button_event("switch_mode", "attach", "true");
	} else {
		ret = send_button_event("switch_mode", "attach", "false");
		__atalk_stop_play();
	}

	return ret;
}

static struct atalk_method methods[] = {
	/* notification */
	{
		.name = "play",
		.handler = play_handler,
	},
	{
		.name = "stop",
		.handler = stop_handler,
	},
	{
		.name = "pause",
		.handler = pause_handler,
	},
	{
		.name = "resume",
		.handler = resume_handler,
	},
	{
		.name = "pause_toggle",
		.handler = pause_toogle_handler,
	},
	{
		.name = "set_volume",
		.handler = set_volume_handler,
		.is_valid = atalk_is_attaching,
	},
	{
		.name = "net_state_change",
		.handler = net_state_change_handler,
	},
	/* TODO: set_led, update_screen, play_voice_prompt */

	/* result */
	{
		.name = "is_attaching",
		.handler = is_attaching_handler,
	},
	{
		.name = "get_mac_address",
		.handler = get_mac_address_handler,
	},
	{
		.name = "get_setup_wifi_state",
		.handler = get_setup_wifi_handler,
	},
	{
		.name = "get_volume",
		.handler = get_volume_handler,
	},
};

static void *atalk_cli_func(void *args)
{
	char cmd[512];

	while (1) {
		int i;
		const char *method;
		json_object *c, *o;
		bool is_valid = true;

		memset(cmd, 0, sizeof(cmd));
		hub_recv(cmd, sizeof(cmd));
		pr_debug(">>>> %s: Recv: %s\n", __func__, cmd);

		c = json_tokener_parse(cmd);
		json_object_object_get_ex(c, "method", &o);
		method = json_object_get_string(o);

		for (i = 0; i < ARRAY_SIZE(methods); i++) {
			struct atalk_method *m = &methods[i];
			if (!strcmp(m->name, method)) {
				if (m->is_valid)
					is_valid = m->is_valid(c);
				if (is_valid)
					m->handler(c);
				else
					pr_debug("     %s invalid\n", cmd);
				break;
			}
		}

		if (i >= ARRAY_SIZE(methods))
			printf("%s: invalid command: %s\n", __func__, method);

		json_object_put(c);
	}

	hub_destory();

	return NULL;
}

static int mozart_player_sync(play_status_t status)
{
	int count = 0;
	player_status_t *player_status;

	while (1) {
		usleep(100000);
		if (count++ >= 50) {
			printf("%s timeout: wait %d(%d)\n",
			       __func__, status, player_status->play_status);
			return -1;
		}
		player_status = player_get_player_status(atalk_player_handler);
		if (player_status == NULL)
			continue;
		if (strcmp(atalk_player_handler->uuid, player_status->uuid))
			return -1;

		if (player_status->play_status == status)
			return 0;
		/* FIXME: */
		if ((status == STOP_STATUS) && (player_status->play_status == PAUSE_STATUS))
			return 0;
	}

	return 0;
}

static void *player_status_func(void *args)
{
	const char *url;
	player_status_t *player_status;

	if (atalk_player_handler == NULL) {
		printf("%s: atalk_player_handler is NULL?\n", __func__);
		return NULL;
	}

	while (1) {
		usleep(500000);
		player_status = player_get_player_status(atalk_player_handler);
		if (player_status == NULL)
			continue;
		if (strcmp(atalk_player_handler->uuid, player_status->uuid))
			continue;

		pthread_mutex_lock(&atalk_state_mutex);
		if (atalk.play_state == ATALK_PLAY_IDLE) {
			pthread_mutex_unlock(&atalk_state_mutex);
			continue;
		}

		switch (player_status->play_status) {
		case STOP_STATUS:
			/* state == ATALK_STATE_RUN, player haven't started.
			 * FIXME: Maybe skip 'playing' if music is very short.
			 */
			if (atalk.play_state != ATALK_PLAY_PLAY)
				break;

		case ERROR_STATUS:
			url = player_get_url(atalk_player_handler);
			if (player_status->play_status == ERROR_STATUS)
				__send_play_done(url, -1);
			else
				__send_play_done(url, 0);

		case PAUSE_STATUS:
		case READY_STATUS:
		case WAIT_PLAY_STATUS:
			mozart_led_turn_off(LED_RECORD);
			break;

		case PLAYING_STATUS:
			if (atalk.play_state != ATALK_PLAY_RUN)
				break;

			atalk.play_state = ATALK_PLAY_PLAY;
			mozart_led_turn_on(LED_RECORD);
			break;
		default:
			break;
		}
		pthread_mutex_unlock(&atalk_state_mutex);
	}

	return NULL;
}

/*******************************************************************************
 * interface
 *******************************************************************************/
int mozart_atalk_volume_set(int vol)
{
	mozart_volume_set(vol, MUSIC_VOLUME);
	send_player_volume_change(vol);
	return 0;
}

int mozart_atalk_prev(void)
{
	return send_button_event("previous", NULL, NULL);
}

int mozart_atalk_next(void)
{
	return send_button_event("next", NULL, NULL);
}

int mozart_atalk_next_channel(void)
{
	return send_button_event("next_channel", NULL, NULL);
}

int mozart_atalk_toogle(void)
{
	pr_debug("**** %s ****\n", __func__);
	return pause_toogle_handler(NULL);
}

int mozart_atalk_pause(void)
{
	enum atalk_play_state state;

	pthread_mutex_lock(&atalk_state_mutex);
	pr_debug("**** %s ****\n", __func__);
	state = atalk.play_state;
	if (state != ATALK_PLAY_RUN && state != ATALK_PLAY_PLAY) {
		/* TODO */
		/* if (state == ATALK_PLAY_IDLE) */
		pthread_mutex_unlock(&atalk_state_mutex);
		return 0;
	}
	__pause_handler();
	mozart_player_sync(PAUSE_STATUS);
	pthread_mutex_unlock(&atalk_state_mutex);

	return 0;
}

int mozart_atalk_switch_mode(void)
{
	int ret;

	pthread_mutex_lock(&atalk_state_mutex);
	if (__atalk_is_attaching())
		ret = __switch_mode_handler(false);
	else
		ret = __switch_mode_handler(true);
	pthread_mutex_unlock(&atalk_state_mutex);

	return ret;
}
/* TODO: love_audio */

int mozart_atalk_start(void)
{
	int ret = 0;

	pthread_mutex_lock(&atalk_state_mutex);

	pr_debug("**** %s ****\n", __func__);
	if (!__atalk_is_attaching())
		__switch_mode_handler(true);

	pthread_mutex_unlock(&atalk_state_mutex);

	return ret;
}

int mozart_atalk_stop(void)
{
	int ret = 0;

	pthread_mutex_lock(&atalk_state_mutex);

	pr_debug("**** %s ****\n", __func__);
	if (__atalk_is_attaching())
		__switch_mode_handler(false);

	pthread_mutex_unlock(&atalk_state_mutex);

	return ret;
}

int atalk_network_trigger(bool on)
{
	wifi_info_t wifi_info;

	pthread_mutex_lock(&atalk_state_mutex);

	if (__atalk_app_is_stop()) {
		pthread_mutex_unlock(&atalk_state_mutex);
		return -1;
	}

	wifi_info = get_wifi_mode();

	if (on && (wifi_info.wifi_mode == STA || wifi_info.wifi_mode == STANET)) {
		pr_debug("**** %s: wifi sta mode (state:%s_%s, play: %s) ****\n",
			 __func__, __atalk_is_attaching() ? "ATTACH" : "UNATTACH",
			 __atalk_is_online() ? "ONLINE" : "OFFLINE",
			 atalk_play_state_str[atalk.play_state]);
		if (!__atalk_is_attaching())
			__switch_mode_handler(true);
		if (!__atalk_is_online()) {
			send_wifi_state_change(wifi_end_ok);
			__atalk_set_online();
		}
		__atalk_prepare_play();
		atalk_dump();
	} else {
		pr_debug("**** %s: wifi ap mode (state:%s_%s, play: %s) ****\n",
			 __func__, __atalk_is_attaching() ? "ATTACH" : "UNATTACH",
			 __atalk_is_online() ? "ONLINE" : "OFFLINE",
			 atalk_play_state_str[atalk.play_state]);
#if 1
		/* TODO: play_voice_prompt 12.mp3 and 14.mp3 */
		if (!__atalk_is_attaching())
			__switch_mode_handler(true);
#endif
		if (__atalk_is_attaching() && __atalk_is_online()) {
			/* Cant send wifi_state_change twice! */
			send_wifi_state_change(wifi_start);
			__atalk_set_offline();
		}
		__atalk_stop_play();
		atalk_dump();
	}

	pthread_mutex_unlock(&atalk_state_mutex);

	return 0;
}

int start_atalk_app(void)
{
	pthread_t atalk_thread, player_status_thread;

	if (!atalk_initialized) {
		if (hub_init())
			return -1;

		if (share_mem_init()) {
			printf("%s: share_mem_init fail\n", __func__);
			return -1;
		}

		atalk_player_handler = player_get_player_handler("atalk");
		if (atalk_player_handler == NULL) {
			printf("%s: get_player_handler fail!\n", __func__);
			return -1;
		}

		if (pthread_create(&atalk_thread, NULL, atalk_cli_func, NULL) != 0) {
			printf("%s: Can't create atalk_thread: %s\n",
			       __func__, strerror(errno));
			return -1;
		}
		pthread_detach(atalk_thread);

		if (pthread_create(&player_status_thread, NULL, player_status_func, NULL) != 0) {
			printf("%s: Can't create player_status_thread: %s\n",
			       __func__, strerror(errno));
			return -1;
		}
		pthread_detach(player_status_thread);

		/* FIXME */
		mozart_system("date 2015-11-9");
		atalk_initialized = true;
	}

#ifdef ATALK_CTL_DEBUG
	if (!access("/mnt/sdcard/atalk_vendor_log.txt", R_OK | W_OK))
		mozart_system("atalk_vendor -c /usr/fs/etc/atalk/prodconf.json >"
			      "/mnt/sdcard/atalk_vendor_log.txt 2>&1 &");
	else
#endif
		mozart_system("atalk_vendor -c /usr/fs/etc/atalk/prodconf.json >/dev/null 2>&1 &");
	if (share_mem_set(ATALK_DOMAIN, STATUS_RUNNING))
		printf("%s: share_mem_set fail.\n", __func__);

	atalk_network_trigger(true);

	return 0;
}

void stop_atalk_app(void)
{
	pthread_mutex_lock(&atalk_state_mutex);
	if (__atalk_app_is_stop()) {
		pthread_mutex_unlock(&atalk_state_mutex);
		return ;
	}

	__atalk_set_attaching();
	__atalk_set_offline();
	__atalk_stop_play();
	if (share_mem_set(ATALK_DOMAIN, STATUS_SHUTDOWN))
		printf("%s: share_mem_set fail\n", __func__);

	mozart_system("killall atalk_vendor");
	unlink("/var/run/doug.pid");
	pthread_mutex_unlock(&atalk_state_mutex);
}
