
#ifndef __BD_ONLINE_VR_API_H_
#define __BD_ONLINE_VR_API_H_

#include "shopping_demo_switch.h"

typedef enum {
	VR_ST_STANDBY = 0,
	VR_ST_RECORDING,
	VR_ST_SPEECH_START,
	VR_ST_SPEECH_END,
	VR_ST_RECOGNIZING,
	VR_ST_RECOGNSE_FINISHED,
	VR_ST_ERROR,
} vr_status_t;

typedef enum {
    VOICE_CMD_TYPE_UNKNOWN = 0,
    VOICE_CMD_TYPE_CALL,
    VOICE_CMD_TYPE_SMS,
    VOICE_CMD_TYPE_MAIL,
    VOICE_CMD_TYPE_WEBSITE,
    VOICE_CMD_TYPE_APP,
    VOICE_CMD_TYPE_SEARCH,
    VOICE_CMD_TYPE_ALARM,
    VOICE_CMD_TYPE_QA,
    VOICE_CMD_TYPE_CONVERSATION,    // Identical to VOICE_CMD_TYPE_QA, but after tts the receiver should restart online vr (qa.pquery may be NULL)
#if NOTE_RECORD_DEMO_ENABLED
    VOICE_CMD_TYPE_RECORD_NOTE,    // Identical to VOICE_CMD_TYPE_CONVERSATION, but after tts the receiver should trigger voice note recording
#endif
    VOICE_CMD_TYPE_SETTING,
    VOICE_CMD_TYPE_SNS,
    VOICE_CMD_TYPE_CONTACT,
    VOICE_CMD_TYPE_MODE,
    VOICE_CMD_TYPE_ITEM,
    VOICE_CMD_TYPE_MUSIC,
    VOICE_CMD_TYPE_TTS,
    VOICE_CMD_TYPE_JASON,
    VOICE_CMD_TYPE_CONTROL,
    VOICE_CMD_TYPE_MUSICCONTROL,
    VOICE_CMD_TYPE_DEVICECONTROL,
    VOICE_CMD_TYPE_SHOPPING,
    VOICE_CMD_TYPE_NUM,
    VOICE_CMD_TYPE_ERROR,
	VOICE_CMD_TYPE_CUSTOMCONTROL,
} voice_cmd_type;

typedef enum {
    VOICE_CMD_MODE_AUX = 0,
    VOICE_CMD_MODE_WIFI,
    VOICE_CMD_MODE_RADIO,
    VOICE_CMD_MODE_BLUETOOTH,
    VOICE_CMD_MODE_LOCAL,
    VOICE_CMD_MODE_FM,
} voice_cmd_mode_type_t;

typedef enum {
    VOICE_CMD_CONTROL_VOL_UP = 0,
    VOICE_CMD_CONTROL_VOL_DOWN,
    VOICE_CMD_CONTROL_NEXT_MUSIC,
    VOICE_CMD_CONTROL_PREV_MUSIC,
    VOICE_CMD_CONTROL_MUTE,
    VOICE_CMD_CONTROL_LOUDSPEAK,
    VOICE_CMD_CONTROL_NEXT_PAGE,
    VOICE_CMD_CONTROL_PREV_PAGE,
    VOICE_CMD_CONTROL_POWEROFF,
    VOICE_CMD_CONTROL_STANDBY,
    VOICE_CMD_CONTROL_SETUP,
} voice_cmd_control_type_t;


#define VR_MODE_SINGLE_SHOT 0
#define VR_MODE_CONVERSATION 1
typedef enum {
    ONLINE_VR_PROPERTY_MODE = 0,	/* int, see ONLINE_VR_MODE_* definitions for possible values */
    ONLINE_VR_PROPERTY_GPS_LAT,     /* double, GPS lattitude range from -90 to 90, rough location for FM tuning and sending drift bottles to near by users etc.*/
    ONLINE_VR_PROPERTY_GPS_LON  	/* double, GPS longitude range from -180 to 180, rough location for FM tuning and sending drift bottles to near by users etc.*/
} online_vr_property_code_t;


typedef struct voice_cmd_music_st {
    voice_cmd_type type;
    char *purl;
    char *lrcurl;
    char *partist;
    char *pname;
} voice_cmd_music_t;

typedef struct voice_cmd_qa_st {
    voice_cmd_type type;
    char *pquery;
    char *pcontent;
} voice_cmd_qa_t;

typedef struct voice_cmd_mode_st {
    voice_cmd_mode_type_t mode;
    char* pcontext;
} voice_cmd_mode_t;

typedef struct voice_cmd_alarm_st {
    voice_cmd_type type;
    bool enable;
    //struct rtc_time alarm_time;
    bool repeat;
} voice_cmd_alarm_t;

typedef struct voice_cmd_control_st {
    voice_cmd_type type;
    voice_cmd_control_type_t control;
} voice_cmd_control_t;

typedef struct voice_cmd_error_st {
    voice_cmd_type type;
    int error;
} voice_cmd_error_t;

typedef union voice_cmd_result_ut {
    voice_cmd_type type;
    voice_cmd_music_t music;
    voice_cmd_qa_t qa;
    voice_cmd_mode_t mode;
    voice_cmd_alarm_t alarm;
    voice_cmd_control_t control;
    voice_cmd_error_t error;
} voice_cmd_result_t;

typedef enum {
    // 错误码类型:客户端语音预处理错误
    ERROR_CLIENT  = 0x20000,
    // 错误码类型:录音错误
    ERROR_RECORDER  = 0x30000,
    // 错误码类型:联网错误
    ERROR_NETWORK  = 0x40000,
    // 错误码类型:服务器识别错误
    ERROR_SERVER  = 0x50000,
    // unknown error source
    ERROR_UNKNOWN= 0x10000,
} error_type_t;

typedef enum {
// 语音识别状态错误通知:未知错误(异常)
    ERROR_CLIENT_UNKNOWN =  0x20001,
// 语音识别状态错误通知:用户未说话
    ERROR_CLIENT_NO_SPEECH  = 0x20002,
// 语音识别状态错误通知:用户说话声音太短
    ERROR_CLIENT_TOO_SHORT  = 0x20003,
// 语音识别状态错误通知:用户说话声音太长
    ERROR_CLIENT_TOO_LONG  = 0x20003,
// 语音识别状态错误通知:语音前端库检测异常
    ERROR_CLIENT_JNI_EXCEPTION  = 0x20005,
// 语音识别状态错误通知:整体识别时间超时
    ERROR_CLIENT_WHOLE_PROCESS_TIMEOUT =  0x20006,

// 语音识别状态错误通知:录音设备不可用 会自动结束本次识别
    ERROR_RECORDER_UNAVAILABLE  = 0x30001,
// 语音识别状态错误通知:录音中断 会自动结束本次识别
    ERROR_RECORDER_INTERCEPTED  = 0x30002,

// 网络工作状态:网络不可用
    ERROR_NETWORK_UNUSABLE =  0x40001,
// 网络工作状态:网络发生错误别
    ERROR_NETWORK_CONNECT_ERROR  = 0x40002,
// 网络工作状态:网络本次请求超时
    ERROR_NETWORK_TIMEOUT =  0x40003,
// 网络工作状态:解析失败
    ERROR_NETWORK_PARSE_EERROR  = 0x40004,

//服务器返回错误,协议参数错误
    ERROR_SERVER_PARAMETER_ERROR =  0x51001,
//服务器返回错误,识别过程出错
    ERROR_SERVER_RECOGNITION_ERROR  = 0x51002,
//服务器返回错误,未登记的应用程序包名
    ERROR_SERVER_INVALID_APP_NAME =  0x51005,
//服务器返回错误,后端错误
    ERROR_SERVER_BACKEND_ERROR  = 0x51006,
//服务器返回错误,语音质量问题
    ERROR_SERVER_SPEECH_QUALITY_ERROR  = 0x51007,
    ERROR_SERVER_SPEECH_TOO_LONG   = 0x51008,
// 整体识别时间超时
    ERROR_WHOLE_PROCESS_TIMEOUT  = 0x60000,

    ERROR_UNKNOWN_ERROR = 0x10001,
} error_code_t;

/* Custom keyword handler parameter */
typedef struct binary_lookup_user_param_t{
    const char* query_text;
    voice_cmd_result_t* query_presult;
}binary_lookup_user_param_t;

/* Custom keyword handler return value */
typedef enum{
    KEYWORD_PROCESSED = 0,
    KEYWORD_NOT_PROCESSED = 1,
}keyword_process_result_t;

#ifdef  __cplusplus
extern "C" {
#endif
// ========================= Interface for the app to use =========================
/*
  * int vr_start_online_vr();
  * Start Online Voice Recognition, e.g. which is called when user pressed button down
  * When OfflineVR triggers the OnlineVR, there is no need to call this.
  */
int vr_start_online_vr();

/*
  * int vr_cancel_online_vr();
  * Cancel ongoing OnlineVR
  * For instance when user short press button to cancel the OnlineVR.
  * return:
  *     0 if cancel was made and handler registered with vr_reg_online_vr_user_canceled_handler has been called
  *     -1 if recognition was not running
  *     1 if recognition was running and was canceled, but no event handler was registered with vr_reg_online_vr_user_canceled_handler
  */
int vr_cancel_online_vr();

/*
  * void vr_online_vr_speak_finish(void);
  * Notify the OnlineVR, that user has finished talking.
  * e.g. user pressed button up, to finish to recording actively
  * Calling this decreases the wait time for results.
  */
void vr_online_vr_speak_finish(void);

/*
  * int vr_online_vr_get_status(void);
  * Check if OnlineVR is running.
  * return:
  *     1 if OnlineVR is running.
  *     0 if OnlineVR is not running.
  */
int vr_online_vr_get_status(void);

// utility functions for managing voice_cmd_result_t passed to vr result handler
/*
  * void vr_copy_online_vr_result(const voice_cmd_result_t* src, voice_cmd_result_t** dest);
  * Utility function for making a copy of voice_cmd_result_t structure.
  * For example in OnlineVR result callback registered by vr_reg_online_vr_result_handler, you should make a copy. 
  * You should call vr_free_online_vr_result to free the dest, when it is not needed anymore.
  * Parameters:
  *     src: The source structure to copy from.
  *     dest: Target structure to copy to, if passed *dest is NULL, memory will be allocated, else existing will be used.
  */
void vr_copy_online_vr_result(const voice_cmd_result_t* src, voice_cmd_result_t** dest);

/*
  * void vr_free_online_vr_result(voice_cmd_result_t *presult);
  * Utility function to release the resources allocated for voice_cmd_result_t structure by vr_copy_online_vr_result.
  */
void vr_free_online_vr_result(voice_cmd_result_t *presult);

/*
  * void vr_set_online_vr_property(online_vr_property_code_t property_id, void* property_value, int value_size);
  * Set properties for onlineVR.
  * Parameters:
  *     property_id: Id of the property, possible values defined in online_vr_property_code_t
  *     property_value: Pointer to value
  *     value_size: Size of the value pointed by property_value in bytes.
  */
void vr_set_online_vr_property(online_vr_property_code_t property_id, void* property_value, int value_size);


// ========================= register callbacks for VR events ============================
/*
  * void vr_reg_online_vr_started_handler(void(*handler)(void));
  * Register a callback for getting notified when OnlineVR starts.
  * Registered callback will receive calls in following cases:
  * 1. App calls vr_start_online_vr() to start OnlineVR.
  * 2. OnlineVR is started by OfflineVR (in this case, application need not call vr_start_online_vr()).
  */
void vr_reg_online_vr_started_handler(void(*handler)(void));

/*
  * void vr_reg_online_vr_result_handler(void (*handler)(voice_cmd_result_t * presult));
  * Register a callback for receiving the OnlineVR results.
  * Registered callback will get called after successfull OnlineVR and receives the result as parameter.
  * The passed result is not valid after returning from callback, so it is recommended to use vr_copy_online_vr_result to make a copy of it inside the callback.
  */
void vr_reg_online_vr_result_handler(void (*handler)(voice_cmd_result_t * presult));

/*
  * void vr_reg_online_vr_error_handler(void (*handler)(int errorType, int errorCode, void* pData));
  * Register a callback for receiving OnlineVR error.
  * Registered callback will get called when OnlineVR ends with an error.
  * Parameters passed for handler:
  *     int errorType    Type of the error
  *     int errorCode   Error code
  *     void* pData     NULL or a string describing the error.
  * 
  */
void vr_reg_online_vr_error_handler(void (*handler)(error_type_t errorType, error_code_t errorCode, void* pData));

/*
  * void vr_reg_online_vr_user_canceled_handler(void (*handler)(void));
  * Register a handler for receiving notification when OnlineVR is canceled.
  * Registered callback will get a call when app calls vr_cancel_online_vr and OnlineVR is canceled.
  * If OnlineVR is not running when call is made, the callback will not get called.
  */
void vr_reg_online_vr_user_canceled_handler(void (*handler)(void));

/*
  * void vr_reg_online_vr_speech_finished_handler(void (*handler)(void));
  * Register a handler for receiving notification when OnlineVR detects speech has ended.
  * Registered callback will get called when
  * 1. OnlineVR VAD detected speech end
  * 2. App called vr_online_vr_speak_finish
  */
void vr_reg_online_vr_speech_finished_handler(void (*handler)(void));



/*
  * void vr_reg_internet_connection_check_handler(bool(*isInternetConnectionAvailable)(void));
  * Register a handler for the library to check if internet connection is available
  * The library will use this callback to check if internet connection is available before starting OnlineVR.
  * The implementation should return true, if there is a connection, false if there is not.
  *
  * If the registered callback returns false, error callback will be made with errorType = ERROR_NETWORK and errorCode = ERROR_NETWORK_UNUSABLE
  */
void vr_reg_internet_connection_check_handler(bool(*isInternetConnectionAvailable)(void));

/*
 * void vr_reg_keyword_handler(const char* keyword, keyword_process_result_t(*fn)(const char*, void*));
 * Application may use this method to register a custom handler for custom keyword.
 * Params:
 *    keyword (const char*) Custom keyword, zero terminated string. Passed pointer can be released after the register call.
 *    Handler_func (keyword_process_result_t(*)(const char*, void*)) pointer to the function processing the keyword.
 *                                                                   Same function pointer may be used for multiple keywords.
 * Params passed to handler callback:
 *     (const char*)    Reserved for future extensions. Should always be an empty string (not NULL, strlen() returns 0)
 *     (void*)          Points to a binary_lookup_user_param_t structure.
 *
 * Recommended implementation of callback:
 * keyword_process_result_t keyword_handler(const char* reserved, void* uParam){
 *    binary_lookup_user_param_t *params = (binary_lookup_user_param_t*)uParam;
 *    if(reserved == NULL || strlen(reserved) > 0)
 *    {
 *        return KEYWORD_NOT_PROCESSED;  // Reserved has been taken in use, don't process.
 *    }
 *
 *    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 *    // ADD YOUR CUSTOM PROCESSING CODE, DO NOT BLOCK FOR EXTENSIVE AMMOUNT OF TIME OR CALL ANY INTERFACE FROM libvr //
 *    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 *
 *    // You will still get a callback to online_vr_result_handler with the presult stored in params->query_presult. Set the type of the result to
 *    // VOICE_CMD_TYPE_CUSTOMCONTROL so the callback may ignore the result or do what ever you wish in this case.
 *    // params->query_presult->type is the only field you are allowed to modify. Please do not touch any other fields of params->query_presult.
 *    params->query_presult->type = VOICE_CMD_TYPE_CUSTOMCONTROL;
 *
 *    return KEYWORD_PROCESSED; // Indicate the keyword was processed to stop the processing of the result in libvr. 
 *                              
 * }
 */
void vr_reg_keyword_handler(const char* keyword, keyword_process_result_t(*handler_func)(const char*, void*));

#ifdef  __cplusplus
}
#endif

#endif /*__BD_ONLINE_VR_API_H_*/


