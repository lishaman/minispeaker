#ifndef __LCD_INTERFACE_H__
#define __LCD_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "_lcd_show.h"

#define LCD_PNG_PATH 	"/usr/share/lcd_show/"
#define WIFI_PNG_PATH 	LCD_PNG_PATH"cloud_play.png"
#define BT_PNG_PATH 	LCD_PNG_PATH"bt_play.png"
#define TF_PNG_PATH 	LCD_PNG_PATH"tf_play.png"
#define LINEIN_PNG_PATH LCD_PNG_PATH"aux_play.png"

#define WIFI_PLAY_CN 	"云播放"
#define BT_PLAY_CN 	"蓝牙播放"
#define TF_PLAY_CN 	"本地播放"
#define LINEIN_PLAY_CN "外接播放"

#define BT_DISCONNECT_CN 	"蓝牙已断开"
#define TF_REMOVE_CN		"TF卡已拔出"
#define LINEIN_DISCONNECT_CN  	"外接已断开"

#define SYSTME_BOOTING_CN 	"系统启动中"
#define WIFI_CONNECTING_CN 	"网络连接中"
#define WIFI_CONNECTED_CN 	"网络连接成功"
#define WIFI_CONNECT_FAILED_CN	 "网络连接失败"
#define WIFI_CONFIG_CN		"配置网络"
#define WIFI_CONFIG_SUC_CN	"配置网络成功"
#define WIFI_CONFIG_FAIL_CN	"配置网络失败"

#define VR_BEGIN_CN 	"语音识别开始"
#define VR_WAKE_UP_CN 	"请问您想说什么"
#define VR_FAILED_CN 	"语音识别失败"
#define SEARCH_SONG_CN 	"歌曲搜索中"
#define VR_UNCLEAR_SPEECH_CN 	"没听清楚说什么"

	extern void mozart_lcd_display(char * pic, char *str);
	extern void mozart_lcd_display_str(struct ft *f, char * str,int y);

#ifdef __cplusplus
}
#endif

#endif // __LCD_INTERFACE_H__
