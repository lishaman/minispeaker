#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#include <json-c/json.h>
#include "ximalaya_interface.h"
#include "ximalaya.h"
#include "sharememory_interface.h"
#include "player_interface.h"
#include "utils_interface.h"

#define DEBUG(x,y...)	{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}
#define ERROR(x,y...)	{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}

static player_handler_t *vr_handler = NULL;
static pthread_mutex_t vr_song_lock = PTHREAD_MUTEX_INITIALIZER;

static char *songlist[10];
static int songnum = -1;
static int songcnt = 0;
static bool autoplay_flag = false;

static int ximalaya_get_songurl(char *keyword)
{
	char *res = NULL;

	if (!keyword)
		return -1;

	res = mozart_ximalaya_search_music(keyword, 10);
	if (!res)
		return -1;

	songcnt = mozart_ximalaya_parse_result(res, XIMALAYA_AUDIO_FMT_32, songlist, 10);
	free(res);

	printf("%s: keyword: %s, songcnt: %d\n", __func__, keyword, songcnt);

	return 0;
}

static int vr_status_change_callback(player_snapshot_t *snapshot, struct player_handler *handler, void *param)
{
	if (strcmp(handler->uuid, snapshot->uuid)) {
		DEBUG("Not vr control mode, do nothing...\n");
		return 0;
	}

	DEBUG("vr recv player status: %d, %s.\n", snapshot->status, player_status_str[snapshot->status]);

	switch (snapshot->status) {
	case PLAYER_TRANSPORT:
	case PLAYER_PLAYING:
		if(share_mem_set(VR_DOMAIN, STATUS_PLAYING))
			ERROR("share_mem_set failure.");
		break;
	case PLAYER_PAUSED:
		if(share_mem_set(VR_DOMAIN, STATUS_PAUSE))
			ERROR("share_mem_set failure.");
		break;
	case PLAYER_UNKNOWN:
	case PLAYER_STOPPED:
		if(share_mem_set(VR_DOMAIN, STATUS_RUNNING))
			ERROR("share_mem_set failure.");
		if (autoplay_flag)
			ximalaya_next();
		break;
	default:
		break;
	}

	return 0;
}

int ximalaya_play(char *keyword)
{
	int i = 0;
	int ret = 0;

	autoplay_flag = false;

	pthread_mutex_lock(&vr_song_lock);

	if (vr_handler) {
		mozart_player_handler_put(vr_handler);
		vr_handler = NULL;
	}

	for (i = 0; i < songcnt; i++) {
		if (songlist[i]) {
			free(songlist[i]);
			songlist[i] = NULL;
		}
	}
	songnum = -1;
	songcnt = 0;

	if (ximalaya_get_songurl(keyword)) {
		printf("search song failed, play nothing(TODO: place a tone here).\n");
		ret = -1;
		goto err_exit;
	}
	if (songcnt == 0) {
		// TODO: place a tone here.
		printf("Found 0 music, Please re-try later.\n");
		ret = -1;
		goto err_exit;
	}

	/* XXX: connect to player on vr service startup. */
	vr_handler = mozart_player_handler_get("ingenic-vr", vr_status_change_callback, NULL);
	if (!vr_handler) {
		printf("get player handler by vr failed, exit...\n");
		ret = -1;
		goto err_exit;
	}

	pthread_mutex_unlock(&vr_song_lock);

	songnum = 0;
	if (mozart_player_cacheurl(vr_handler, songlist[songnum])) {
		ERROR("player play url by vr failed.");
		ret = -1;
		goto err_exit;
	}

	autoplay_flag = true;

	return ret;

err_exit:
	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

int ximalaya_stop(void)
{
	module_status status;
	int ret = 0;

	pthread_mutex_lock(&vr_song_lock);

	autoplay_flag = false;

	if (vr_handler)
		mozart_player_stop(vr_handler);

	if (AUDIO_OSS == get_audio_type()) {
		int cnt = 60;
		while (cnt--) {
			share_mem_get(VR_DOMAIN, &status);
			if (status == STATUS_RUNNING)
				break;
			usleep(50 * 1000);
		}

		if (cnt < 0) {
			ERROR("stop player stop play failed.");
			ret = -1;
			goto err_exit;
		}
	}

err_exit:
	if (vr_handler) {
		mozart_player_handler_put(vr_handler);
		vr_handler = NULL;
	}

	share_mem_get(VR_DOMAIN, &status);
	if (status == STATUS_STOPPING)
		if(share_mem_set(VR_DOMAIN, STATUS_RUNNING))
			ERROR("share_mem_set failure.");

	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

int ximalaya_pause(void)
{
	char *uuid = NULL;
	int ret = 0;
	player_status_t status = PLAYER_UNKNOWN;

	pthread_mutex_lock(&vr_song_lock);

	if (!vr_handler) {
		ret = -1;
		goto err_exit;
	}

	uuid = mozart_player_getuuid(vr_handler);
	if (!uuid) {
		printf("get player current controller's uuid failed, Cannot pause.\n");
		ret = -1;
		goto err_exit;
	} else {
		if (!strcmp(vr_handler->uuid, uuid)) {
			status = mozart_player_getstatus(vr_handler);
			if (status == PLAYER_PLAYING)
				ret = mozart_player_pause(vr_handler);
		}
	}

	free(uuid);

err_exit:
	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

int ximalaya_resume(void)
{
	char *uuid = NULL;
	int ret = 0;
	player_status_t status = PLAYER_UNKNOWN;

	pthread_mutex_lock(&vr_song_lock);

	if (!vr_handler) {
		ret = -1;
		goto err_exit;
	}

	uuid = mozart_player_getuuid(vr_handler);
	if (!uuid) {
		printf("get player current controller's uuid failed, Cannot resume.\n");
		ret = -1;
		goto err_exit;
	} else {
		if (!strcmp(vr_handler->uuid, uuid)) {
			status = mozart_player_getstatus(vr_handler);
			if (status == PLAYER_PAUSED)
				ret = mozart_player_resume(vr_handler);
		}
	}

	free(uuid);

err_exit:
	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

int ximalaya_play_pause(void)
{
	char *uuid = NULL;
	int ret = 0;
	player_status_t status = PLAYER_UNKNOWN;

	pthread_mutex_lock(&vr_song_lock);

	if (!vr_handler) {
		printf("vr_handler invalid, do nothing.\n");
		ret = -1;
		goto err_exit;
	}

	uuid = mozart_player_getuuid(vr_handler);
	if (!uuid) {
		printf("get player current controller's uuid failed, Cannot play/pause.\n");
		ret = -1;
		goto err_exit;
	} else {
		if (!strcmp(vr_handler->uuid, uuid)) {
			status = mozart_player_getstatus(vr_handler);
			if (status == PLAYER_PAUSED) {
				ret = mozart_player_resume(vr_handler);
			} else if (status == PLAYER_PLAYING) {
				ret = mozart_player_pause(vr_handler);
			}
		}
	}

	free(uuid);

err_exit:
	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

static int get_prev_idx(void)
{
	int idx = -1;

	/* If playing the first music or not play, cycle to the last music */
	if (songnum <= 0)
		idx = songcnt - 1;
	else
		idx = --songnum < 0 ? 0 : songnum;

	songnum = idx;

	return idx;
}

int ximalaya_pre(void)
{
	int ret = 0;

	pthread_mutex_lock(&vr_song_lock);

	if (!vr_handler) {
		ret = -1;
		goto exit_direct;
	}

	if (mozart_player_playurl(vr_handler, songlist[get_prev_idx()])) {
		ERROR("player play url by vr failed.");
		ret = -1;
		goto exit_direct;
	}

exit_direct:
	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

static int get_next_idx(void)
{
	int idx = -1;

	/* If playing the last music, cycle to the first music */
	idx = (++songnum) % songcnt;

	songnum = idx;

	return idx;
}

int ximalaya_next(void)
{
	int ret = 0;

	pthread_mutex_lock(&vr_song_lock);

	if (!vr_handler) {
		ret = -1;
		goto exit_direct;
	}

	if (mozart_player_playurl(vr_handler, songlist[get_next_idx()])) {
		ERROR("player play url by vr failed.");
		ret = -1;
		goto exit_direct;
	}

exit_direct:
	pthread_mutex_unlock(&vr_song_lock);

	return ret;
}

static char *artists[] = {"王菲", "刘德华", "陈奕迅", "汪峰", "那英", "刘德华", "凤凰传奇", "张学友"};

void ximalaya_cloudplayer_start(void)
{
	int idx = 0;
	char buf[1024] = {};

	srandom(time(NULL));
	idx = random() % (sizeof(artists) / sizeof(artists[0]));

	ximalaya_play(artists[idx]);

	sprintf(buf, "欢迎回来，为您播放%s的歌", artists[idx]);
	mozart_tts(buf);

	mozart_tts_wait();

	ximalaya_resume();
}
