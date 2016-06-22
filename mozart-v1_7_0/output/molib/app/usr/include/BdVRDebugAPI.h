#ifndef _BD_VR_DEBUG_API_H_
#define _BD_VR_DEBUG_API_H_

#include <inttypes.h>
#define ENABLE_DEBUG_APIS 1

#if ENABLE_DEBUG_APIS

#ifdef  __cplusplus
extern "C" {
#endif
// Edit log levels
void set_libvr_log_level(int logLevel);
void set_libbdasr_log_level(int logLevel);
int get_libvr_log_level();
int get_libbdasr_log_level();
void set_offlineVR_log_level(int level);
int get_offlineVR_log_level();

void set_libvr_offline_vr_record_state(bool enabled);
void set_libvr_online_vr_record_state(bool enabled);
void set_libvr_offline_vr_test_mode(bool enabled);

bool get_libvr_offline_vr_record_state();
bool get_libvr_online_vr_record_state();
bool get_libvr_offline_vr_test_mode();

// For debugging online VR
/*
 * Get the final result json from the library.
 */
void registerGetOnlineVRResultJSONCallback(void(*onlineVRResultJSON)(const char*));

/*
 * Get partial result from the library.
 * (not a JSON, just the recognized string so far)
 */
void registerGetPartialVRResultCallback(void(*partialVRResult)(const char*));

/*
 * Get notified when onlineVR notifies silence detected
 */
void registerSilenceDetectedCallback(void(*silenceDetected)(void));


// For debugging offline vr

/*
 * Gets called when offline vr triggers, afterSamples is the ammount of samples since start or last trigger.
 * To get the total offset, register a callback with registerOfflineVRProcessedSamplesCallback.
 * Registering this callback will put the library in test mode where the online VR will not be triggered when trigger word is detected.
 */
void registerOfflineVRDebugCallback(void(*offlineVRTrigger)(int));

/*
 * Gets called when offline VR processes more samples, sampleCount is the ammount of samples the offline VR just processed.
 * To get the total count of samples processed so far, sum the sampleCount passed to this callback, may be used to determine
 * when offlineVR has finished processing all the samples fed to the library
 */
void registerOfflineVRProcessedSamplesCallback(void(*processedSamples)(int));

/*
 * override the s-file paths and keyword
 */
void registerOfflineNetFilesGetter(void(*getter)(char** dict_file, char** user_file, char** mm_file, char** mmap_file, char** sync_file, char** wakeupWord));

/*
 * Set sample marks for benchmarking the delays.
 * Will print out time stamps when sample numbers indicated by marks have been read from driver and when they have been fed to library
 */
void vr_debug_set_benchmark_sample_numbers(uint32_t* markSamples, int markCount);

/*
 */
void vr_set_cardin(bool cardin);
int a_law_pcm_to_wav(const char *pcm_file, const char*wav);

/*
 * Interface for getting time trace of latest onlineVR session
 */
double dbg_get_vr_start_time();
double dbg_get_VAD_speech_start_time();
double dbg_get_VAD_speech_end_time();
double dbg_get_user_speech_end_time();
double dbg_get_final_result_arrive_time();
double dbg_get_music_detail_fetch_start_time();
double dbg_get_music_detail_fetch_end_time();
double dbg_get_json_parse_end_time();
double getCurrentSystemTime();
double timeSinceLastOnlineVRStart();

#ifdef  __cplusplus
}
#endif

#endif // ENABLE_DEBUG_APIS

#endif // _BD_VR_DEBUG_API_H_
