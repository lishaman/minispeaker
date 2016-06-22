#ifndef _APP_H_
#define _APP_H_

#include "airplay.h"
#include "ximalaya.h"
#include "tts.h"
#include "mozart_config.h"

#include "utils_interface.h"
#include "sharememory_interface.h"
#include "volume_interface.h"
#include "tips_interface.h"
#include "wifi_interface.h"

#if (SUPPORT_LOCALPLAYER == 1)
#include "localplayer_interface.h"
#endif

#if (SUPPORT_DMR == 1)
#include "render_interface.h"
#endif

#if (SUPPORT_DMS == 1)
#include "dms.h"
#endif

#if (SUPPORT_LAPSULE == 1)
#include "lapsule_control.h"
#endif

#if (SUPPORT_ATALK == 1)
#include "atalk/atalk_control.h"
#endif

#if (SUPPORT_BT != BT_NULL)
#include "bt.h"
#include "bluetooth_interface.h"
#endif

#if (SUPPORT_VR != VR_NULL)
#include "vr.h"
#endif

typedef enum app_t{
#if (SUPPORT_DMR == 1)
	DMR = 1,
#endif
	DMC,
#if (SUPPORT_DMS == 1)
	DMS,
#endif
	AIRPLAY,
#if (SUPPORT_LOCALPLAYER == 1)
	LOCALPLAYER,
#endif
#if (SUPPORT_BT != BT_NULL)
	BT,
#endif
#if (SUPPORT_VR != VR_NULL)
	VR,
#endif
#if (SUPPORT_LAPSULE == 1)
	LAPSULE,
#endif
#if (SUPPORT_ATALK == 1)
	ATALK,
#endif
}app_t;

extern int start(app_t app);
extern int stop(app_t app);

/**
 * @brief  启动所有服务
 *
 * @param depend_network [in] 是否依赖网络的标志
 * -1: 启动所有服务
 *  0: 启动所有不依赖网络的服务
 *  1: 启动所有依赖网络的服务
 *
 * @return   成功返回0
 */
extern int startall(int depend_network);

/**
 * @brief  关闭所有服务
 *
 * @param depend_network [in] 是否依赖网络的标志
 * -1: 关闭所有服务
 *  0: 关闭所有不依赖网络的服务
 *  1: 关闭所有依赖网络的服务
 *
 * @return   成功返回0
 */
extern int stopall(int depend_network);

/**
 * @brief  重启所有服务
 *
 * @param depend_network [in] 是否依赖网络的标志
 * -1: 重启所有服务
 *  0: 重启所有不依赖网络的服务
 *  1: 重启所有依赖网络的服务
 *
 * @return   成功返回0
 */
extern int restartall(int depend_network);

/**
 * @brief 停止音乐播放
 *
 * @return  成功返回0,失败返回-1.
 */
extern int mozart_stop(void);

#endif //_APP_H_
