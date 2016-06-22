#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "mozart_config.h"
#include "vr.h"
#include "sharememory_interface.h"
#include "wifi_interface.h"
#include "tips_interface.h"
#include "power_interface.h"
#include "lapsule_control.h"
#include "ximalaya.h"
#include "airplay.h"
#include "key_function.h"
#include "utils_interface.h"
#include "ini_interface.h"
#include "app.h"
#include "json-c/json.h"

#define DEBUG_PRT(format, ...) {printf("[%s : %s : %d] ",__FILE__,__func__,__LINE__); printf(format, ##__VA_ARGS__);}
#define ERROR(x,y...)	{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}
#define PLAY_MUSIC 2
#define PAUSE_MUSIC 1
#define STOP_MUSIC 0
static int play_status = -1;

int mozart_play_status(void)
{
    memory_domain domain;
    module_status status;
    int ret = -1;

    ret = share_mem_get_active_domain(&domain);
    if (ret) {
        printf("get active domain error in %s:%s:%d, do nothing.\n",
               __FILE__, __func__, __LINE__);
        return 0;
    }

    switch (domain) {
    case UNKNOWN_DOMAIN:
                printf("system is in idle mode in %s:%s:%d.\n",
				__FILE__, __func__, __LINE__);
		play_status = STOP_MUSIC;
		break;
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER_DOMAIN:
		if (0 != share_mem_get(LOCALPLAYER_DOMAIN, &status)) {
			ERROR("share_mem_get failed.");
			return -1;
		}
		if (STATUS_PLAYING == status){
			play_status = PLAY_MUSIC;
		}
		else if (STATUS_PAUSE == status){
			play_status = PAUSE_MUSIC;
		}
		break;
#endif
#if (SUPPORT_AIRPLAY == 1)
    case AIRPLAY_DOMAIN:
		play_status = PLAY_MUSIC;
                return 0;
#endif
#if (SUPPORT_DMR == 1)
    case RENDER_DOMAIN:
		if (0 != share_mem_get(RENDER_DOMAIN, &status)) {
			ERROR("share_mem_get failed.");
			return -1;
		}
		if (STATUS_PLAYING == status){
			play_status = PLAY_MUSIC;
		}
		else if (STATUS_PAUSE == status){
			play_status = PAUSE_MUSIC;
		}
                break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK_DOMAIN:
		if (share_mem_get(ATALK_DOMAIN, &status)) {
			ERROR("share_mem_get failed.");
			return -1;
		}
		if (STATUS_PLAYING == status)
			play_status = PLAY_MUSIC;
		else if (STATUS_PAUSE == status)
			play_status = PAUSE_MUSIC;
	    break;
#endif
#if (SUPPORT_VR != VR_NULL)
    case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
                play_status = PLAY_MUSIC;
#elif (SUPPORT_VR == VR_SPEECH)
                printf("TODO: vr speech play_status.\n");
#elif (SUPPORT_VR == VR_IFLYTEK)
                printf("TODO: vr iflytek play_status.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
                printf("TODO: vr unisound play_status.\n");
#endif
                break;
#endif

#if (SUPPORT_BT != BT_NULL)
    case BT_HS_DOMAIN:
        printf("bt priority is highest, WILL NOT play_status in %s:%d.\n", __func__, __LINE__);
        break;
#endif
#if (SUPPORT_BT == BT_BCM)
    case BT_AVK_DOMAIN:
        printf("TODO: bt play_status.\n");
        break;
#endif
    default:
	play_status = STOP_MUSIC;
        printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
        break;
    }
	return 0;
}

int mozart_pause(void)
{
    memory_domain domain;
    module_status status;
    int ret = -1;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
    control_point_info *info = NULL;
#endif

    ret = share_mem_get_active_domain(&domain);
    if (ret) {
        printf("get active domain error in %s:%s:%d, do nothing.\n",
               __FILE__, __func__, __LINE__);
        return -1;
    }

    switch (domain) {
    case UNKNOWN_DOMAIN:
        printf("system is in idle mode in %s:%s:%d.\n",
				__FILE__, __func__, __LINE__);
		break;
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER_DOMAIN:
		if (0 != share_mem_get(LOCALPLAYER_DOMAIN, &status)) {
			ERROR("share_mem_get failed.");
			return -1;
		}
		if (STATUS_PLAYING == status){
			mozart_localplayer_play_pause();
			play_status = PAUSE_MUSIC;
		}
		break;
#endif
#if (SUPPORT_AIRPLAY == 1)
    case AIRPLAY_DOMAIN:
        printf("airplay protocol is playing, do not support play/pause in %s:%s:%d, return.\n",
               __FILE__, __func__, __LINE__);
        return -1;
#endif
#if (SUPPORT_DMR == 1)
    case RENDER_DOMAIN:
#if (SUPPORT_LAPSULE == 1)
        info = mozart_render_get_controler();
        if(strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){        //is lapsule in control
            if(0 != lapsule_do_pause()){
				printf("lapsule_do_pause failure.\n");
			}
			play_status = PAUSE_MUSIC;
		}else{
#endif
			if (0 != share_mem_get(RENDER_DOMAIN, &status)) {
				ERROR("share_mem_get failed.");
				return -1;
			}
			if (STATUS_PLAYING == status){
				if (mozart_render_play_pause()) {
					printf("play/pause render playback error in %s:%s:%d, return.\n",
							__FILE__, __func__, __LINE__);
					return -1;
				}
				play_status = PAUSE_MUSIC;
			}
#if (SUPPORT_LAPSULE == 1)
		}
#endif
		break;
#endif
#if (SUPPORT_ATALK == 1)
    case ATALK_DOMAIN:
	mozart_atalk_pause();
	play_status = PAUSE_MUSIC;
	break;
#endif
#if (SUPPORT_VR != VR_NULL)
    case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
        if (mozart_vr_stop()) {
            printf("stop vr playback error in %s:%s:%d, return.\n",
                   __FILE__, __func__, __LINE__);
            return -1;
        }
#elif (SUPPORT_VR == VR_SPEECH)
	ximalaya_pause();
	play_status = PAUSE_MUSIC;
#elif (SUPPORT_VR == VR_IFLYTEK)
        printf("TODO: vr iflytek stop music.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound stop music.\n");
#endif
		break;
#endif

#if (SUPPORT_BT != BT_NULL)
	case BT_HS_DOMAIN:
		printf("bt priority is highest, WILL NOT play/pause in %s:%d.\n", __func__, __LINE__);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_play_pause();
		play_status = PAUSE_MUSIC;
		break;
#endif
	default:
		printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
		break;
	}
	return 0;
}

#if (SUPPORT_VR == VR_SPEECH)
int mozart_vr_speech_interface_callback(vr_info * recog_info)
#else
int mozart_vr_iflytek_interface_callback(vr_info * recog_info)
#endif
{
	int asr_mode_c = CLOUD;
	char *recog_asr_result = NULL;
	char *asr_result = NULL;
	wifi_info_t infor;

	if(recog_info->json_len != 0 && (recog_info->recog_asr_result + recog_info->json_len) != NULL){
		recog_asr_result = recog_info->recog_asr_result;
		asr_result = recog_info->recog_asr_result + strlen(recog_info->recog_asr_result) + 1;
		printf("recog_asr_result: %s, asr_result: %s.\n", recog_asr_result, asr_result);
#if ( SUPPORT_LCD == 1)
		mozart_lcd_display(NULL,recog_info->recog_result);
#endif
	}

	if(recog_info->recog_flag == WAKE_UP){ //语音识别结果：唤醒语音识别，关闭正在播放的音乐
		DEBUG_PRT("Speech wake up, if PLAY_MUSIC will close!!\n");
		mozart_play_status();
		if(!(play_status == PAUSE_MUSIC || play_status == STOP_MUSIC))
			mozart_pause();
	}
	else if(recog_info->recog_flag == SEARCH_MUSIC) //语音识别结果：搜索音乐
	{
		int ret = -1;
		char buf[1024] = {};
		json_object *asr_json = NULL;
		json_object *tmp = NULL;
		json_object *sem = NULL;
		json_object *input = NULL;
		json_object *songer = NULL;
		json_object *songname = NULL;
		char *songer_str = NULL;
		char *songname_str = NULL;

		asr_json = json_tokener_parse(recog_asr_result);
		if (asr_json) {
			ret = json_object_object_get_ex(asr_json, "result", &tmp);
			if (ret) {
				json_object_object_get_ex(tmp, "input", &input);
				ret = json_object_object_get_ex(tmp, "semantics", &sem);
				if (ret) {
					ret = json_object_object_get_ex(sem, "request", &tmp);
					if (ret) {
						ret = json_object_object_get_ex(tmp, "param", &tmp);
						if (ret) {
							json_object_object_get_ex(tmp, "歌手名", &songer);
							json_object_object_get_ex(tmp, "歌曲名", &songname);
						}
					}
					if (songer == NULL && songname == NULL) {
						ret = json_object_object_get_ex(sem, "nbest", &tmp);
						if (ret) {
							tmp = json_object_array_get_idx(tmp, 0);
							if (tmp != NULL) {
								ret = json_object_object_get_ex(tmp, "request", &tmp);
								if (ret) {
									ret = json_object_object_get_ex(tmp, "param", &tmp);
									if (ret) {
										json_object_object_get_ex(tmp, "歌手名", &songer);
										json_object_object_get_ex(tmp, "歌曲名", &songname);
									}
								}
							}
						}
					}
				}
			}
		}

#ifdef SUPPORT_SONG_SUPPLYER
		songer_str = songer ? json_object_get_string(songer): NULL;
		songname_str = songname ? json_object_get_string(songname): NULL;

		if (!songer_str) {
			if (songname_str)
				sprintf(buf, "正在为您搜索%s，请稍后", songname_str);
			else
				sprintf(buf, "语音识别失败,请重试");
		} else if (!songname_str) {
			sprintf(buf, "正在为您搜索%s的歌曲，请稍后", songer_str);
		} else {
			sprintf(buf, "正在为您搜索%s的%s，请稍后", songer_str, songname_str);
		}

		switch (SUPPORT_SONG_SUPPLYER) {
		case SONG_SUPPLYER_XIMALAYA:
			ximalaya_stop();

			mozart_tts(json_object_get_string(input));
			mozart_tts(buf);

			// cache url
			if (-1 == ximalaya_play(asr_result)) {
				if (!songer_str) {
					if (songname_str)
						sprintf(buf, "没有找到歌曲%s", songname_str);
					else
						sprintf(buf, "没有找到歌曲");
				} else if (!songname_str) {
					sprintf(buf, "没有找到%s的歌曲", songer_str);
				} else {
					sprintf(buf, "没有找到%s的%s", songer_str, songname_str);
				}
				mozart_tts(buf);
			} else {
				play_status = PLAY_MUSIC;
			}

			// waiting for all tones play over.
			mozart_tts_wait();

			// resume music
			ximalaya_resume();
			break;
		case SONG_SUPPLYER_LAPSULE:
#if (SUPPORT_LAPSULE == 1)
			lapsule_do_search_song(asr_result);
			play_status = PLAY_MUSIC;
#else
			printf("lapsule disabled, NO search song service.\n");
#endif
			break;
		default:
			printf("No song search supplyer found, do nothing.\n");
			break;
		}
#endif
		json_object_put(asr_json);
		asr_json = NULL;
	}
	else if(recog_info->recog_flag == NATIVE_ASR_CMD) //语音识别结果：简单的命令操作
	{
		infor = get_wifi_mode();
		DEBUG_PRT("native cmd : %s\n",asr_result);
		if(strcmp(asr_result, "net_status") == 0){
			if(infor.wifi_mode == AP)
				//mozart_tts("网络已断开");
				mozart_play_key("net_disconnected");
			else if(infor.wifi_mode == STA){
				//mozart_tts("网络已连接");
#if 1
				mozart_play_key("net_connected");
#else
				char conf[30];
				sprintf(conf, "设备以连接路由：%s", infor.ssid);
				mozart_tts(conf);
#endif
			}
		}
		else if(strcmp(asr_result, "battery") == 0){
			int capacity;
			char battery_buf[20] = "\0";
			capacity = mozart_get_battery_capacity();
			sprintf(battery_buf, "当前电池电量为 : %d", capacity);
			mozart_tts(battery_buf);
		}
		else if(strcmp(asr_result, "volume_up") == 0){
			mozart_volume_up();
		}
		else if(strcmp(asr_result, "volume_down") == 0){
			mozart_volume_down();
		}
		else if((strcmp(asr_result, "config_net") &&\
					strcmp(asr_result, "wifi_setting")) == 0){
			mozart_stop();
			play_status = STOP_MUSIC;
			stopall(1);
			mozart_config_wifi();
			goto asr_mode_cfg;
		}
		else if((strcmp(asr_result, "wifi_mode") &&\
					strcmp(asr_result, "wifi_off")) == 0){
			mozart_stop();
			play_status = STOP_MUSIC;
			stopall(1);
			mozart_wifi_mode();
			goto asr_mode_cfg;
		}
		else if((strcmp(asr_result, "previous_song") &&\
					strcmp(asr_result, "previous")) == 0){
			mozart_previous_song();
			goto asr_mode_cfg;
		}
		else if((strcmp(asr_result, "next_song") &&\
					strcmp(asr_result, "next")) == 0){
			mozart_next_song();
			goto asr_mode_cfg;
		}
		else if((strcmp(asr_result, "play") &&\
					strcmp(asr_result, "continue") &&\
					strcmp(asr_result, "continue_play")) == 0){
			if(play_status == PAUSE_MUSIC || play_status == STOP_MUSIC){
				mozart_play_pause();
				play_status = PLAY_MUSIC;
			}
			goto asr_mode_cfg;
		}
		else if((strcmp(asr_result, "stop") &&\
					strcmp(asr_result, "exitplayer")) == 0){
			if(play_status != STOP_MUSIC){
				mozart_stop();
				play_status = STOP_MUSIC;
			}
			goto asr_mode_cfg;
		}
		else if(strcmp(asr_result, "pause") == 0){
			if(play_status == PLAY_MUSIC){
				mozart_pause();
				play_status = PAUSE_MUSIC;
			}
			goto asr_mode_cfg;
		}
		if(play_status != STOP_MUSIC){
			mozart_play_status();
			if(play_status == PAUSE_MUSIC){
				mozart_play_pause();
				play_status = PLAY_MUSIC;
			}
		}
	}
	else if(recog_info->recog_flag == TTS_ANSWER) //语音识别结果：合成语音识别云端返回的结果
	{
		DEBUG_PRT("waiting for the answer!!\n");
#if defined(SPEECH_CLOUD_TTS) || defined(IFLYTEK_CLOUD_TTS)
#else
		mozart_tts_sync(asr_result);
#endif
		if(play_status == PAUSE_MUSIC){
			mozart_play_pause();
			play_status = PLAY_MUSIC;
		}
	}
	else if(recog_info->recog_flag == WRONG) //语音识别结果：语音识别出错（受网络状态，录音质量等影响）
	{
		if(play_status == PAUSE_MUSIC){
			mozart_play_pause();
			play_status = PLAY_MUSIC;
		}
	}

asr_mode_cfg:

	infor = get_wifi_mode();
	if(infor.wifi_mode == AP)
		asr_mode_c = NATIVE;
	else
		asr_mode_c = CLOUD;

	return asr_mode_c;
}

#if (SUPPORT_VR == VR_ATALK)
int mozart_vr_atalk_interface_callback(void *arg)
{
	static int play_status_bak = -1;
	enum asr_state_type asr_state = (enum asr_state_type)arg;

	mozart_play_status();
	if (asr_state == ASR_STATE_RECOG) {
		play_status_bak = play_status;
		if (play_status == PLAY_MUSIC)
			mozart_pause();
	} else if (asr_state == ASR_STATE_IDLE) {
		if ((play_status_bak == PLAY_MUSIC)
		    && (play_status != PLAY_MUSIC)) {
			mozart_play_pause();
			play_status = PLAY_MUSIC;
		}
	} else {
		printf("%s: asr_state = %d?\n", __func__, asr_state);
	}

	return 0;
}
#endif	/* SUPPORT_VR == VR_ATALK */
