/*
 * SettingServer.h
 * Interface for starting the setting server and passing events to server.
 *
 */


#ifndef __SETTING_SERVER_H_
#define __SETTING_SERVER_H_
#include <inttypes.h>
#include <stdbool.h>
#include "DeviceSettingInterface.h"

#define LIB_TRACE_LEVEL_OFF 0
#define LIB_TRACE_LEVEL_ERROR 1
#define LIB_TRACE_LEVEL_WARNING 2
#define LIB_TRACE_LEVEL_DEBUG 3
#define LIB_TRACE_LEVEL_INFO 4
#define LIB_TRACE_LEVEL_TRACE 5

typedef struct
{
  //
  char wififlag;
  int ssidlen;
  int pwlen;
  int identlen;

  // some configurations for finish the configuration
  // setting server uses
  union ip_addr
  	{
  	  char ipv4[4];
	  char ipv6[16];
  	}ip;

  union ip_mask
  	{
  	  char ipv4[4];
	  char ipv6[16];
  	}mask;
  int port;

  // Wifi settings
  char ssid[38];
  char pw[38];
  char ident[38];
}sound_wifi_configs;

#ifdef  __cplusplus
extern "C" {
#endif
// Init and launch the server
int BDSettingServ_start(BDSettingServ_Callbacks* callbackInterface);
// Stop and cleanup
int BDSettingServ_stop();

// Pass wifi setting data received from LED or Sound
void BDSettingServ_receivedSettingsFromLED(uint8_t* settingsData);
void BDSettingServ_receivedSettingsFromSound(sound_wifi_configs *configData);

// Wifi state changes
// client mode
void BDSettingServ_WIFIConnected();
void BDSettingServ_WIFIDisconnected();
// Accesspoint mode
void BDSettingServ_WIFIAPOpened();
void BDSettingServ_WIFIAPClosed();

// result callback for controlling the offline VR.
// it is safe to call the BDSettingServ_OfflineVRStateChanged function when ever
// the state is changed, no matter what is changing it, if there is no client connected
// to server, the event will be ignored.
void BDSettingServ_OfflineVRStateChanged(bool newState);
void BDSettingServ_OfflineVRModeInStandbyChanged(bool newMode);
void BDSettingServ_OfflineVRControlError(int error);

// result callback for controlling the online VR
void BDSettingServ_OnlineVRConversationModeStateChanged(bool newState);
void BDSettingServ_OnlineVRControlError(int error);

// result callback for Baidu account modifications
void BDSettingServ_GotBaiduAccountAuthCredentials(const char* authURL, const char* authKey);
void BDSettingServ_BaiduAccountAuthStateChanged(bool authed);
void BDSettingServ_BaiduAccountAuthFailed(int error);

// Wifi scan results
void BDSettingServ_WifiScanResultsAvailable();

// Trace level
void set_library_debug_level(int debug_level);

#ifdef  __cplusplus
}
#endif

#endif
