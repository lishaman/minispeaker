#ifndef _VR_H_
#define _VR_H_

#if (SUPPORT_VR == VR_BAIDU)
#include "movr_interface.h"
#endif

#if (SUPPORT_VR == VR_SPEECH)
#include "vr-speech_interface.h"
extern int mozart_vr_speech_interface_callback(vr_info * recog_info);
#endif

#if (SUPPORT_VR == VR_IFLYTEK)
#include "vr-iflytek_interface.h"
extern int mozart_vr_iflytek_interface_callback(vr_info * recog_info);
#endif

#if (SUPPORT_VR == VR_ATALK)
#include "vr-atalk_interface.h"
extern int mozart_vr_atalk_interface_callback(void *);
#endif

#endif //_VR_H_
