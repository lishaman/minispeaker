#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>

#include "app.h"

#if (SUPPORT_VR != VR_NULL)
static int wakeup_mode_mark = VOICE_KEY_MIX_WAKEUP;
#endif

#if (SUPPORT_DMR == 1)
bool dmr_started = false;

static void AVTransportAction_callback(char *ActionName, struct Upnp_Action_Request *ca_event)
{
    printf("Not support AVTransportAction: %s\n", ActionName);
    return;
}

static void ConnectionManagerAction_callback(char *ActionName, struct Upnp_Action_Request *ca_event)
{
    printf("Not support ConnectionManagerAction: %s\n", ActionName);
    return;
}

static void RenderingControlAction_callback(char *ActionName, struct Upnp_Action_Request *ca_event)
{
    printf("Not support RenderingControlAction: %s\n", ActionName);
    return;
}

void dmr_start(void)
{
	int ret = 0;
	render_device_info_t device;

	memset(&device, 0, sizeof(device));
	/*
	 * this is our default frendlyname rule in librender.so,
	 * and you can create your own frendlyname rule.
	 */
	char deviceName[64] = {};
	char macaddr[] = "255.255.255.255";
	memset(macaddr, 0, sizeof (macaddr));

	if (dmr_started) {
		stop(DMR);
		dmr_started = false;
	}

	//FIXME: replace "wlan0" with other way. such as config file.
	get_mac_addr("wlan0", macaddr, "");
	sprintf(deviceName, "SmartAudio-%s", macaddr + 4);
	device.friendlyName = deviceName;

	if (mozart_render_AVTaction_callback(AVTransportAction_callback,
					     ConnectionManagerAction_callback,
					     RenderingControlAction_callback))
		printf("[warning] register render action callback failed.\n");

	ret = mozart_render_start(&device);
	if (ret) {
		printf("[Error] start render service failed.\n");
	}

	dmr_started = true;
}
#endif

#if (SUPPORT_DMS == 1)
bool dms_started = false;
#endif

#if (SUPPORT_AIRPLAY == 1)
bool airplay_started = false;
#endif

#if (SUPPORT_LOCALPLAYER == 1)
bool localplayer_started = false;
#endif

#if (SUPPORT_BT != BT_NULL)
bool bt_started = false;
#endif

#if (SUPPORT_VR != VR_NULL)
bool vr_started = false;
#endif

#if (SUPPORT_LAPSULE == 1)
bool lapsule_started = false;
#endif

#if (SUPPORT_ATALK == 1)
bool atalk_started = false;
#endif

int start(app_t app)
{
	int ret = -1;
	module_status status;

	switch(app){
#if (SUPPORT_DMR == 1)
	case DMR:
		if (dmr_started)
			mozart_render_refresh();
		else
			dmr_start();
		break;
#endif
#if (SUPPORT_DMS == 1)
	case DMS:
		if (dms_started) {
			stop(app);
			dms_started = false;
		}

		if (start_dms()) {
                        printf("ushare service start failed.\n");
			return -1;
		}

		dms_started = true;
		return 0;
#endif
	case DMC:
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY:
		if (airplay_started) {
			stop(app);
			airplay_started = false;
		}

                mozart_airplay_init();
		mozart_airplay_start_service();

		airplay_started = true;
		return 0;
#endif
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER:
		if (localplayer_started) {
			stop(app);
			localplayer_started = false;
		}

		if (mozart_localplayer_startup()) {
			return -1;
		}

		localplayer_started = true;
		return 0;
#endif
#if (SUPPORT_BT != BT_NULL)
	case BT:
		if (bt_started) {
			stop(app);
			bt_started = false;
		}

		start_bt();
		bt_started = true;
		return 0;
#endif

#if (SUPPORT_VR != VR_NULL)
	case VR:
		if (vr_started) {
			stop(app);
			vr_started = false;
		}

#if (SUPPORT_VR_WAKEUP == VR_WAKEUP_VOICE)
		wakeup_mode_mark = VOICE_WAKEUP;
#elif (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_SHORTPRESS)
		wakeup_mode_mark = KEY_SHORTPRESS_WAKEUP;
#elif (SUPPORT_VR_WAKEUP == VR_WAKEUP_KEY_LONGPRESS)
		wakeup_mode_mark = KEY_LONGPRESS_WAKEUP;
#endif
#if (SUPPORT_VR == VR_BAIDU)
		if(!mozart_vr_get_status()) {
			mozart_vr_startup();
			vr_started = true;
		}
#elif (SUPPORT_VR == VR_SPEECH)
		if(mozart_vr_speech_get_status()) {
			mozart_vr_speech_shutdown();
		}
		mozart_vr_speech_startup(wakeup_mode_mark, mozart_vr_speech_interface_callback);

		share_mem_get(SDCARD_DOMAIN, &status);
		if (status == STATUS_EXTRACT) {
			if (get_wifi_mode().wifi_mode == STA ||
			    get_wifi_mode().wifi_mode == STANET)
				ximalaya_cloudplayer_start();
		} else if (status == STATUS_INSERT) {
			printf("sdcard found, will not play network music.\n");
		}

		vr_started = true;
#elif (SUPPORT_VR == VR_IFLYTEK)
		if(mozart_vr_iflytek_get_status()) {
			mozart_vr_iflytek_shutdown();
		}
		mozart_vr_iflytek_startup(wakeup_mode_mark, mozart_vr_iflytek_interface_callback);
		vr_started = true;
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound start\n");
#elif (SUPPORT_VR == VR_ATALK)
		if(mozart_vr_atalk_get_status()) {
			mozart_vr_atalk_shutdown();
		}
		mozart_vr_atalk_startup(wakeup_mode_mark, mozart_vr_atalk_interface_callback);
		vr_started = true;
#endif
		break;
#endif

#if (SUPPORT_LAPSULE == 1)
	case LAPSULE:
		if (lapsule_started) {
			stop(app);
			lapsule_started = false;
		}

                if (start_lapsule_app()) {
                        printf("start lapsule services failed in %s: %s.\n", __func__, strerror(errno));
			return -1;
                }

		lapsule_started = true;
		return 0;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK:
		if (atalk_started) {
			stop(app);
			atalk_started = false;
		}

		if (start_atalk_app()) {
			printf("start atalk services failed in %s: %s.\n", __func__, strerror(errno));
			return -1;
		}

		atalk_started = true;
		return 0;
#endif
	default:
		printf("WARNING: Not support this module(id = %d)\n", app);
		break;
	}

	return 0;
}

int stop(app_t app)
{
	switch(app){
#if (SUPPORT_DMR == 1)
	case DMR:
		mozart_render_terminate_fast();
		break;
#endif
#if (SUPPORT_DMS == 1)
	case DMS:
		if (dms_started) {
			stop_dms();
			dms_started = false;
		}
		break;
#endif
	case DMC:
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY:
		if (airplay_started) {
			mozart_airplay_shutdown();
			airplay_started = false;
		}
		break;
#endif
#if (SUPPORT_LOCALPLAYER == 1)
	case LOCALPLAYER:
		if (localplayer_started) {
			mozart_localplayer_shutdown();
			localplayer_started = false;
		}
		break;
#endif
#if (SUPPORT_BT != BT_NULL)
	case BT:
		if (bt_started) {
			stop_bt();
			bt_started = false;
		}
		break;
#endif
#if (SUPPORT_VR != VR_NULL)
	case VR:
		if (!vr_started)
			break;
#if (SUPPORT_VR == VR_BAIDU)
		mozart_vr_shutdown();
#elif (SUPPORT_VR == VR_SPEECH)
		mozart_vr_speech_shutdown();
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

#elif (SUPPORT_VR == VR_IFLYTEK)
		mozart_vr_iflytek_shutdown();
#elif (SUPPORT_VR == VR_UNISOUND)
		printf("TODO: vr unisound stop\n");
#elif (SUPPORT_VR == VR_ATALK)
		mozart_vr_atalk_shutdown();
#endif
		vr_started = false;
		break;
#endif
#if (SUPPORT_LAPSULE == 1)
	case LAPSULE:
		if (lapsule_started) {
			stop_lapsule_app();
			lapsule_started = false;
		}
		break;
#endif
#if (SUPPORT_ATALK == 1)
	case ATALK:
		if (airplay_started) {
			stop_atalk_app();
			atalk_started = false;
		}
		break;
#endif
	default:
		printf("WARNING: Not support this module(id = %d)\n", app);
		break;
	}

	return 0;
}


int startall(int depend_network)
{
	if (depend_network == -1 || depend_network == 1) {
#if (SUPPORT_DMR == 1)
		start(DMR);
#endif
#if (SUPPORT_DMS == 1)
		start(DMS);
#endif
		start(DMC);
#if (SUPPORT_AIRPLAY == 1)
		start(AIRPLAY);
#endif
#if (SUPPORT_VR != VR_NULL)
		start(VR);
#endif
#if (SUPPORT_LAPSULE == 1)
		start(LAPSULE);
#endif
#if (SUPPORT_ATALK == 1)
		atalk_network_trigger(true);
#endif
	}

	if (depend_network == -1 || depend_network == 0) {
#if (SUPPORT_ATALK == 1)
		start(ATALK);
#endif
#if (SUPPORT_BT != BT_NULL)
		start(BT);
#endif
#if (SUPPORT_LOCALPLAYER == 1)
		start(LOCALPLAYER);
#endif
	}

	return 0;
}

int stopall(int depend_network)
{
	if (depend_network == -1 || depend_network == 1) {
#if (SUPPORT_LAPSULE == 1 && SUPPORT_DMR == 1)
		stop(DMR);
#endif
#if (SUPPORT_DMS == 1)
		stop(DMS);
#endif
#if (SUPPORT_AIRPLAY == 1)
		stop(AIRPLAY);
#endif
#if (SUPPORT_VR != VR_NULL)
		stop(VR);
#endif
#if (SUPPORT_ATALK == 1)
		atalk_network_trigger(false);
#endif
	}

	if (depend_network == -1 || depend_network == 0) {
#if (SUPPORT_ATALK == 1)
		stop(ATALK);
#endif
#if (SUPPORT_BT != BT_NULL)
		stop(BT);
#endif
#if (SUPPORT_LOCALPLAYER == 1)
		stop(LOCALPLAYER);
#endif
	}

	return 0;
}

int restartall(int depend_network)
{
	stopall(depend_network);
	startall(depend_network);

	return 0;
}

int mozart_stop(void)
{
    memory_domain domain;
    int ret = -1;
    int cnt = 0;
#if (SUPPORT_DMR == 1 && SUPPORT_LAPSULE == 1)
    control_point_info *info = NULL;
#endif

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
        break;
#if (SUPPORT_LOCALPLAYER == 1)
    case LOCALPLAYER_DOMAIN:
        mozart_localplayer_stop_playback();
        break;
#endif
#if (SUPPORT_AIRPLAY == 1)
    case AIRPLAY_DOMAIN:
        if (mozart_airplay_stop_playback()) {
            printf("stop airplay playback error in %s:%s:%d, return.\n",
                   __FILE__, __func__, __LINE__);
            return 0;
        }
        break;
#endif
#if (SUPPORT_DMR == 1)
    case RENDER_DOMAIN:
#if (SUPPORT_LAPSULE == 1)
        info = mozart_render_get_controler();
        if(strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){        //is lapsule in control
            if(0 != lapsule_do_stop_play()){
                printf("lapsule_do_toggle failure.\n");
            }
        }else{
#endif
            if (mozart_render_stop_playback()) {
                printf("stop render playback error in %s:%s:%d, return.\n",
                       __FILE__, __func__, __LINE__);
                return 0;
            }
#if (SUPPORT_LAPSULE == 1)
        }
#endif
        break;
#endif
#if (SUPPORT_ATALK == 1)
    case ATALK_DOMAIN:
	mozart_atalk_stop();
	break;
#endif
#if (SUPPORT_VR != VR_NULL)
    case VR_DOMAIN:
#if (SUPPORT_VR == VR_BAIDU)
        if (mozart_vr_stop()) {
            printf("stop vr playback error in %s:%s:%d, return.\n",
                   __FILE__, __func__, __LINE__);
            return 0;
        }
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
#elif (SUPPORT_VR == VR_IFLYTEK)
        printf("TODO: vr iflytek stop music.\n");
#elif (SUPPORT_VR == VR_UNISOUND)
        printf("TODO: vr unisound stop music.\n");
#endif
        break;
#endif

#if (SUPPORT_BT != BT_NULL)
    case BT_HS_DOMAIN:
        printf("bt priority is highest, do nothing in %s:%d.\n", __func__, __LINE__);
        return 0;
#endif
#if (SUPPORT_BT == BT_BCM)
    case BT_AVK_DOMAIN:
        mozart_bluetooth_avk_stop_play();
        break;
#endif
    default:
        printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
        break;
    }

    // waiting for dsp idle.
    cnt = 500;
    while (cnt--) {
        if (check_dsp_opt(O_WRONLY))
            break;
        usleep(10 * 1000);
    }
    if (cnt < 0) {
        printf("5 seconds later, /dev/dsp is still in use(writing), try later in %s:%d!!\n", __func__, __LINE__);
        return -1;
    }

	return 0;
}
