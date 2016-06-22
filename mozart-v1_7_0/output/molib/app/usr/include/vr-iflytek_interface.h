#ifndef __IFLYTEK_INTERFACE_H__
#define __IFLYTEK_INTERFACE_H__

/**
 * @brief current asr status.
 */
typedef enum aiengine_status {
	/**
	 * @brief waiting for wakeup.
	 */
	DORMANCY = 0,
	/**
	 * @brief wakeup.
	 */
	WAKEUP,
	/**
	 * @brief waiting for recognition.
	 */
	ASRING,
	/**
	 * @brief Online iflytek recognition success.
	 */
	CLOUD_ASR_SUCCESS,
	/**
	 * @brief Online iflytek synthesis.
	 */
	CLOUD_ASR_TTS,
	/**
	 * @brief Offline iflytek recognition success.
	 */
	NATIVE_ASR_SUCCESS,
	/**
	 * @brief Offline iflytek recognition over grammar.xbnf.
	 */
	NATIVE_ASR_OVER,
	/**
	 * @brief iflytek recognition failure.
	 */
	ASR_FAILED,
	/**
	 * @brief The voice is too small.
	 */
	UNCLEAR_SPEECH,

}aiengine_status;

/**
 * @brief asr result mark.
 */
typedef enum recog_flag_status {
	ASR = 0,
	SEARCH_MUSIC,
	NATIVE_ASR_CMD,
	TTS_ANSWER,
	WAKE_UP,
	WRONG,
}recog_flag_status;

/**
 * @brief voice wakeup mode.
 */
typedef enum wakeup_mode {
	/**
	 * @brief mix wake up: 1.voice 2.short press key
	 */
	VOICE_KEY_MIX_WAKEUP = 0,
	/**
	 * @brief voice
	 */
	VOICE_WAKEUP,
	/**
	 * @brief short press key
	 */
	KEY_SHORTPRESS_WAKEUP,
	/**
	 * @brief long press key
	 */
	KEY_LONGPRESS_WAKEUP,
};

/**
 * @brief cloud or native mode.
 */
typedef enum asr_mode {
	CLOUD = 0,
	NATIVE,
}asr_mode;

/**
 * @brief voice recognition result struct.
 */
typedef struct vr_info {
	/**
	 * @brief voice recognition status
	 * 0:asring
	 * 1:search music
	 * 2:native cmd
	 * 3:tts answer
	 * 4:wakeup
	 * 5:wrong
	 */
	int recog_flag;
	/**
	 * @brief json length
	 */
	int json_len;
	/**
	 * @brief json_data
	 */
	char recog_asr_result[0];
}vr_info;

typedef int (*mozart_vr_iflytek_callback)(vr_info * recog_info);

/**
 * @brief checkout statue voice recognition of iflytek
 */
extern int mozart_vr_iflytek_get_status(void);

/**
 * @brief init voice recognition of iflytek
 */
extern int mozart_vr_iflytek_startup(int wakeup_mode, mozart_vr_iflytek_callback callback);

/**
 * @brief quit voice recognition of iflytek
 */
extern int mozart_vr_iflytek_shutdown(void);

/**
 * @brief wakeup voice recognition of iflytek
 */
extern int mozart_key_wakeup(void);

/**
 * @brief key control record end voice recognition of iflytek
 */
extern int mozart_key_asr_record_end(void);
#endif
