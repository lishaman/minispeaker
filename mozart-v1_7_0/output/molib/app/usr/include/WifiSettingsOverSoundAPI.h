#ifndef _WIFI_SETTINGS_OVER_SOUND_API_H_
#define _WIFI_SETTINGS_OVER_SOUND_API_H_

// possible error codes to callback registered by vr_reg_wifi_settings_by_sound_error_handler
#define RECEIVE_ERROR_BAD_CRC 0
#define RECEIVE_ERROR_NO_PKG_END 1

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
}wifi_setting_sound_message;

#ifdef  __cplusplus
extern "C" {
#endif

/*
  * void vr_begin_listen_wifi_setting();
  * Call this when ever enter wifi setting mode
  * Starts the wifi setting over sound receiver if not already started.
  */
void vr_begin_listen_wifi_setting();

/*
  * void vr_stop_listen_wifi_setting();
  * Call this when exit wifi setting mode
  * Stops the wifi setting over sound receiver if not already stopped.
  */
void vr_stop_listen_wifi_setting();

/*
  * void vr_reg_received_wifi_settings_by_sound_handler(void(*handler)(wifi_setting_sound_message));
  * Register a callback to get the widfi settings received over sound.
  * You should use the settings passed to the callback to connect to wifi.
  */
void vr_reg_received_wifi_settings_by_sound_handler(void(*handler)(wifi_setting_sound_message));


void vr_reg_wifi_settings_by_sound_error_handler(void(*handler)(int));

#ifdef  __cplusplus
}
#endif
#endif
