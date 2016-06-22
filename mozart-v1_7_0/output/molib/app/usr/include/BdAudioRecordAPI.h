
#ifndef __BD_AUDIO_RECORD_API_H__
#define __BD_AUDIO_RECORD_API_H__

typedef struct
{
    /* pcm buffers for max 2 channels */
    short * pcm[2];

    /* number of channels 1 or 2 */
    int channels;

    /* length of each buffer in pcm samples */
    int samples;
} bd_mic_data_t;

typedef enum {
	VR_SAMPLING_RATE_8K = 8000,
	VR_SAMPLING_RATE_16K = 16000
} vr_sampling_rate_t;

#ifdef  __cplusplus
extern "C" {
#endif

/*
 *  ========================= Interface for the app to use. =========================
 */

/*
 * There is no API for start record, this is managed by library internally.
 * Recording will start when someone needs it (onlineVR/offlineVR/record voice note/WiFi setting over sound receiver)
 * and end when nobody needs it anymore, or the app calls vr_stop_record or vr_stop_record_sync.
 */

/*
 * Stop the Voice Recorder, e.g. the Device Driver
 * NOTE: will cancel any operations using the recorder (onlineVR/offlineVR/record voice note/WiFi setting over sound receiver).
 * The call is asynchronous.
 * 
 * Legacy API, using this is not recommended. To cleanup everything, you should  use vr_close()
 * If called the OnlineVR, OfflineVR, WiFi setting over sound receiver and voice note recorder will be canceled. To recover, use
 * vr_start_offline_vr()
 * vr_start_online_vr()
 * vr_begin_listen_wifi_setting()
 * vr_start_voice_note()
 * 
 */
void vr_stop_record(void);

/*
 * Stop the Voice Recorder, e.g. the Device Driver, the function returns after the driver has been closed.
 * NOTE: will cancel any operations using the recorder (onlineVR/offlineVR/record voice note/WiFi setting over sound receiver).
 * This call is synchronous and will block till the recorder is closed.
 * 
 * Legacy API, using this is not recommended. To cleanup everything, you should  use vr_close()
 * If called the OnlineVR, OfflineVR, WiFi setting over sound receiver and voice note recorder will be canceled. To recover, use
 * vr_start_offline_vr()
 * vr_start_online_vr()
 * vr_begin_listen_wifi_setting()
 * vr_start_voice_note()
 * 
 */
void vr_stop_record_sync(void);

/*
 * int vr_recorder_set_sample_rate(int sampleRate);
 * int vr_recorder_set_sample_rate_sync(int sampleRate);
 *
 * interface to change recorder samplerate
 * Changing samplerate requires closing and reopening the recorder, this may lead to bad recognition results if onlineVR is active.
 * Therefore this interrface will refuce to change the sample rate if OnlineVR is running(listening).
 *
 * Return values:
 *     0    If samplerate was changed or was already the same as requested
 *     -1   If request was ignored due to ongoing OnlineVR
 */
int vr_recorder_set_sample_rate(int sampleRate);
int vr_recorder_set_sample_rate_sync(int sampleRate);

/*
 * void vr_recorder_mic_reopen();
 * Notifies the recorder to reopen microphone with current settings.
 * If microphone is used when this is called, it will get reopened, no matter who is using it.
 * If microphone is not open at the time call is made, the function does nothing.
 */
void vr_recorder_mic_reopen();

/*
 *  ========================= register recording interface callback handlers =========================
 */
/* Register handler to init recording hardware */
void vr_reg_recording_HW_init_handler(void*(*handler)(int, int*));
/* Register handler to deinit recording hardware */
void vr_reg_recording_HW_deinit_handler(void(*handler)(void* hwHandle));
/* Register handler to start recording */
void vr_reg_recording_start_handler(void(*handler)(void* hwHandle));
/* Register handler to stop recording */
void vr_reg_recording_stop_handler(void(*handler)(void* hwHandle));
/*
 * Register handler to get audio data from recording hardware.
 * Handler should copy available audio data to *data and return 0 if success or != 0 on failure
 * (failure will result in a new call to this handler later, to reinitiate the hardwware or to stop the recording, use
 * void vr_stop_record_sync(void);
 */
void vr_reg_recording_fill_buffer_handler(int(*handler)(void * hwHandle, bd_mic_data_t * data));

#ifdef  __cplusplus
}
#endif

#endif
