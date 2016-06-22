#ifndef __SPEECH_INTERFACE_H_
#define __SPEECH_INTERFACE_H_

/* #define SPEECH_CLOUD_TTS */

enum recog_flag_status {
	ASR = 0,
	SEARCH_MUSIC,
	NATIVE_ASR_CMD,
	TTS_ANSWER,
	WAKE_UP,
	WRONG,
};

/**
 * @brief voice wakeup mode.
 */
enum wakeup_mode {
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
enum asr_mode {
	CLOUD = 0,
	NATIVE,
};

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
	/*
	 * recognize result
	 */
	char recog_result[128];
	/**
	 * @brief json length
	 */
	int json_len;
	/**
	 * @brief json_data
	 */
	char recog_asr_result[0];
} vr_info;

typedef int (*mozart_vr_speech_callback)(vr_info *recog_info);

/**
 * @brief checkout statue voice recognition of speech
 */
extern int mozart_vr_speech_get_status(void);

/**
 * @brief init voice recognition of speech
 */
extern int mozart_vr_speech_startup(int wakeup_mode,
		mozart_vr_speech_callback callback);

/**
 * @brief quit voice recognition of speech
 */
extern int mozart_vr_speech_shutdown(void);

/**
 * @brief wakeup voice recognition of speech
 */
extern int mozart_key_wakeup(void);

/**
 * @brief key control record end voice recognition of speech
 */
extern int mozart_key_asr_record_end(void);
#endif
