#ifndef __BD_OFFLINE_VR_API_H_
#define __BD_OFFLINE_VR_API_H_

#ifdef  __cplusplus
extern "C" {
#endif
// ========================= Interface for the app to use =========================
/*
  * bool vr_start_offline_vr(const char* ptrigger);
  * Enable the offlineVR for triggering OnlineVR by voice command.
  * After calling this, the OfflineVR will stay enabled untill a call to vr_stop_offline_vr(void), vr_stop_record(void) or vr_stop_record_sync(void) is made.
  *
  * Parameters:
  *     ptrigger:   The voice command string
  *     
  */
bool vr_start_offline_vr(const char* ptrigger);

/*
  * void vr_stop_offline_vr(void);
  * Disable OfflineVR. 
  */
void vr_stop_offline_vr(void);

/*
  * bool vr_offline_vr_check_started(void);
  * Check if OfflineVR is enabled.
  * OfflineVR may be enabled, but not listening in certain cases such as when OnlineVR is active, but will resume listening automatically once OnlineVR is finished.
  * return value:
  *     true if OfflineVR is enabled.
  *     false if OfflineVR is disabled.
  */
bool vr_offline_vr_check_started(void);

// ========================= register callbacks for VR events ============================
/*
  *  void vr_reg_offline_vr_started_handler(void(*handler)(void));
  *  Function to register a callback, which will be called when ever OfflineVR starts listening.
  *  This callback will be called in following cases:
  *     - App calls vr_start_offline_vr, OfflineVR was not previously enabled and no higher priority task is using the microphone.
  *     - Some higher priority task (such as OnlineVR) is finished and OfflineVR resumes listening.
  */
void vr_reg_offline_vr_started_handler(void(*handler)(void));

#ifdef  __cplusplus
}
#endif

#endif /*__BD_OFFLINE_VR_API_H_*/

