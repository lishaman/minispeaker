

#ifndef __VOICE_RECOGNIZER_H__
#define __VOICE_RECOGNIZER_H__

#include "BdAudioRecordAPI.h"
#include "shopping_demo_switch.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Init/open library
 * Must be called before calling any other interface
 */
void vr_open(vr_sampling_rate_t sampling_rate);

/*
 * Close and cleanup
 * 
 */
void vr_close(void);

/*
 * The GUID is needed to access some of the onlineVR services.
 */
void vr_set_device_GUID(const char* deviceGUID);

/*
 * Get the library version.
 */
char* vr_get_ver_str(void);


#ifdef  __cplusplus
}
#endif



#endif

