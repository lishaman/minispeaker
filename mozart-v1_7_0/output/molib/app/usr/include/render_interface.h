/**
 * @file render_interface.h
 * @brief DLNA Render Init/Start/Stop API
 * @author Su Xiaolong <xiaolong.su@ingenic.com>
 * @version 1.1.0
 * @date 2015-01-21
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *
 * The program is not free, Ingenic without permission,
 * no one shall be arbitrarily (including but not limited
 * to: copy, to the illegal way of communication, display,
 * mirror, upload, download) use, or by unconventional
 * methods (such as: malicious intervention Ingenic data)
 * Ingenic's normal service, no one shall be arbitrarily by
 * software the program automatically get Ingenic data
 * Otherwise, Ingenic will be investigated for legal responsibility
 * according to law.
 */
#ifndef __RENDER_INTERFACE_H__
#define __RENDER_INTERFACE_H__

#include "upnp.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Summary information of control point.
 */
typedef struct {
	/**
	 * @brief IP address of control point.
	 */
	char ipaddr[32];
	/**
	 * @brief The description of control point.
	 */
	char *description;
	/**
	 * @brief The music provider.
	 */
	char *music_provider;
} control_point_info;

/**
 * @brief custom render device info.
 */
typedef struct render_device_info {
	/**
	 * @brief friendly name, default: SmartAudio-$MACID, Optional
	 */
	char *friendlyName;
	/**
	 * @brief manufacturer, default: Ingenic, Optional
	 */
	char *manufacturer;
	/**
	 * @brief manufacturer url, default: www.ingenic.com, Optional
	 */
	char *manufacturerURL;
	/**
	 * @brief model description, default: "dlna-render supported by ingenic", Optional
	 */
	char *modelDescription;
	/**
	 * @brief model name, default: ing, Optional
	 */
	char *modelName;
	/**
	 * @brief model number, default: 001, ptional
	 */
	char *modelNumber;
	/**
	 * @brief model url, default: www.ingenic.com, Optional
	 */
	char *modelURL;
	/**
	 * @brief serial number, default: 001, Optional
	 */
	char *serialNumber;
	/**
	 * @brief Universal Product Code, default: 001, Optional
	 */
	char *UPC;
	/**
	 * @brief Unique Device Name of the UPnP Device, Optional
	 */
	char *UDN;
} render_device_info_t;

/**
 * @brief Simple to Create a DLNA render. include init and run function.
 *
 * @param [in] device device info.
 *
 * @return Upon Success return 0, means render deploy ok. Otherwise it return -1 or errno indicate failed.
 */
extern int mozart_render_start(render_device_info_t *device);

/**
 * @brief Exit from render thread
 */
extern void mozart_render_terminate(void);

/**
 * @brief Set render audio volume to a value.
 *
 * @param Volume [in]	Value [0 ~ 100] of volume to set.
 */
extern void mozart_render_set_volume(int Volume);

/**
 * @brief Stop render audio play, but not termunate render.
 *
 * @return Send stop message success returned 0, others returned -1.
 */
extern int mozart_render_stop_playback(void);

/**
 * @brief Render play pause switching function.
 *
 * @return Success returned 0, others returned -1.
 */
extern int mozart_render_play_pause(void);

/**
 * @brief get control pointer info
 *
 * @return hold control pointer info.
 */
extern control_point_info *mozart_render_get_controler();

/**
 * @brief refresh dmr service, Generally, invoke it on ip changed.
 *
 * @return Success returned 0, others returned -1.
 */
extern int mozart_render_refresh(void);

/**
 * @brief similar to mozart_render_terminate(), but faster because of dmr funcs exited incompletely.
 */
extern void mozart_render_terminate_fast(void);

/**
 * @brief AVTransport Action callback define.
 *
 * @param ActionName [in] action name.
 * @param ca_event [in] upnp action request
 *
 */
typedef void (*UpnpActionCallback)(char *ActionName, struct Upnp_Action_Request *ca_event);

/**
 * @brief register upnp callback for custom action.
 *
 * @param AVTransportAction_callback [in] AVTransportAction callback
 * @param ConnectionManagerAction_callback [in] ConnectionManagerAction callback
 * @param RenderingControlAction_callback [in] RenderingControlAction callback
 *
 * @return return 0 indicate the thread is create success.
 */
extern int mozart_render_AVTaction_callback(UpnpActionCallback AVTransportAction_callback,
					    UpnpActionCallback ConnectionManagerAction_callback,
					    UpnpActionCallback RenderingControlAction_callback);

#ifdef __cplusplus
}
#endif

#endif /* __RENDER_INTERFACE_H__ */
