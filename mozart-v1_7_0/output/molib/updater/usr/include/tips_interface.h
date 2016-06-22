#ifndef __TIPS_INTERFACE_H__
#define __TIPS_INTERFACE_H__

#define	LED_RECORD 0
#define	LED_WIFI 1
#define	LED_BT 2

#define LED_RECORD_FILE "/sys/class/leds/led-record/brightness"
#define LED_RECORD_TRIGGER_FILE "/sys/class/leds/led-record/trigger"
#define LED_RECORD_DELAY_ON_FILE "/sys/class/leds/led-record/delay_on"
#define LED_RECORD_DELAY_OFF_FILE "/sys/class/leds/led-record/delay_off"

//#define LED_WIFI_FILE "/sys/class/leds/led-wifi/brightness"
//#define LED_WIFI_TRIGGER_FILE "/sys/class/leds/led-wifi/trigger"
//#define LED_WIFI_DELAY_ON_FILE "/sys/class/leds/led-wifi/delay_on"
//#define LED_WIFI_DELAY_OFF_FILE "/sys/class/leds/led-wifi/delay_off"
#define LED_WIFI_FILE "/sys/class/leds/wireless_AP/brightness"
#define LED_WIFI_TRIGGER_FILE "/sys/class/leds/wireless_AP/trigger"
#define LED_WIFI_DELAY_ON_FILE "/sys/class/leds/wireless_AP/delay_on"
#define LED_WIFI_DELAY_OFF_FILE "/sys/class/leds/wireless_AP/delay_off"

#define LED_BT_FILE "/sys/class/leds/led-bt/brightness"
#define LED_BT_TRIGGER_FILE "/sys/class/leds/led-bt/trigger"
#define LED_BT_DELAY_ON_FILE "/sys/class/leds/led-bt/delay_on"
#define LED_BT_DELAY_OFF_FILE "/sys/class/leds/led-bt/delay_off"

#ifdef  __cplusplus
extern "C" {
#endif

extern void mozart_led_turn_on(int led_num);
extern void mozart_led_turn_off(int led_num);
extern void mozart_led_turn_flash(int led_num, int num_on, int num_off);
extern void mozart_led_turn_fast_flash(int led_num);
extern void mozart_led_turn_slow_flash(int led_num);

extern void start_led(void);
extern void stop_led(void);
extern void start_motor(void);
extern void stop_motor(void);

extern int mozart_play_key(char *key);
extern int mozart_play_key_sync(char *key);

extern int mozart_play_tone(char *mp3_src);
extern int mozart_stop_tone(void);
extern int mozart_play_tone_sync(char *mp3_src);
extern int mozart_stop_tone_sync(void);

#ifdef  __cplusplus
}
#endif

#endif /** __TIPS_INTERFACE_H__ **/
