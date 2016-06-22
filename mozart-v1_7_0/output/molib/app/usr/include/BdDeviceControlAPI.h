
#ifndef __BD_DEVICE_CONTROL_API_H_
#define __BD_DEVICE_CONTROL_API_H_

typedef enum GetDeviceAlarmResult_t
{
    GET_ALARM_OK,
    GET_ALARM_REQUEST_FAIL,
    GET_ALARM_URL_RESOLVE_FAIL,
    GET_ALARM_MALLOC_FAIL,
    GET_ALARM_BAD_RESPONSE_FROM_SERVER,
    GET_ALARM_SERVER_RETURN_ERROR,
    GET_ALARM_URL_TOO_LONG,
    GET_ALARM_ACCOUNT_ERROR
}GetDeviceAlarmResult_t;

#ifdef  __cplusplus
extern "C" {
#endif
int dev_alarm_create(const char  * abstime, const char *comptime, const char * repeat);
int dev_alarm_remove(void);
int dev_alarm_disable(void);
int dev_alarm_enable(void);
int dev_alarm_query(void);
int dev_timezone_display(const char  * city, const char *timezone);
int dev_timezone_hide(const char * city);
int dev_play_localmusic(const char * path);
int dev_play_radio(char * radio_name, const char * type, const char * freq, const char * url);
int dev_play_fm_radio(const char * freq, char * radio_name);
int dev_play_internet_radio(const char * url, char * radio_name);
int dev_play_auxin(void);
int dev_play_bluetooth(void);
int dev_display_homepage(void);
int dev_display_onlinevr_testing_page(void);
int dev_play_indoortemphumidity(void);

int dev_player_stop(void);
int dev_player_pause(void);
int dev_player_resume(void);
int dev_player_volumeUp(void);
int dev_player_volumeDown(void);
int dev_player_previous(void);
int dev_player_next(void);
int dev_player_rewind(int comptime);
int dev_player_forward(int comptime);
int dev_player_jumpto(int abstime);
int dev_player_replay(void);
int dev_player_speaker_mute(void);
int dev_player_speaker_unmute(void);
int SetDeviceControlResultTtsString(const char * resultTts);
char * GetDeviceControlResultTtsString(void);
int ClearDeviceControlResultTtsString(void);
GetDeviceAlarmResult_t GetDeviceAlarmTone(char * url_buff, int len);
#ifdef  __cplusplus
}
#endif


#endif
