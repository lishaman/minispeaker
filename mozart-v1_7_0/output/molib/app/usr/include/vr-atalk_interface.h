#ifndef __VR_ATALK_INTERFACE_H__
#define __VR_ATALK_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief voice wakeup mode.
 */
enum wakeup_mode_type {
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
 * @brief voice recognition state
 */
enum asr_state_type {
	/**
	 * @brief shutdown state
	 */
	ASR_STATE_SHUTDOWN = 0,
	/**
	 * @brief idle state
	 */
	ASR_STATE_IDLE,
	/**
	 * @brief aec state
	 */
	ASR_STATE_AEC,
	/**
	 * @brief recognize state
	 */
	ASR_STATE_RECOG,
	/**
	 * @brief quit state
	 */
	ASR_STATE_QUIT,
	/**
	 * @brief error state
	 */
	ASR_STATE_ERROR,
};

typedef int (*mozart_vr_atalk_callback)(void *);

/**
 * @brief checkout statue voice recognition of atalk
 */
extern int mozart_vr_atalk_get_status(void);

/**
 * @brief start voice recognition of atalk
 */
extern int mozart_vr_atalk_startup(int wakeup_mode, mozart_vr_atalk_callback callback);

/**
 * @brief quit voice recognition of atalk
 */
extern int mozart_vr_atalk_shutdown(void);

/**
 * @brief wakeup voice recognition of atalk
 */
extern int mozart_key_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif	/* __VR_ATALK_INTERFACE_H__ */
