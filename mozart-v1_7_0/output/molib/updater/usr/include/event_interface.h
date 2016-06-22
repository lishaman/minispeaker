#ifndef __KEY_INTERFACE_H__
#define __KEY_INTERFACE_H__

#include <unistd.h>
#include <linux/input.h>

#ifdef  __cplusplus
extern "C" {
#endif
        /**
         * @brief event type
         */
        typedef enum {
                /* Key up and Key down event */
                EVENT_KEY = 0x100,

                /* HotPlug event, Such as TFCard and Linein and Headset*/
                /* Protocol connect event, Such as DLNA/BT/AIRPLAY device connect/disconnect, */
                EVENT_MISC = 0x200,
        } EventType;
        extern char *EventType_str[];

        typedef struct key_event {
                EventType type; /* EVENT_KEY_DOWN/UP */
                struct input_event key;
        } key_event;

        typedef struct misc_event {
                char name[16]; /* Possible name: "tfcard", "linein", "headset", "dlna", "airplay", "bt", "udisk", "usb"... */
                char type[16]; /* EVENT_CONNECT_CONNECTED/DISCONNECTED */
        } misc_event;

        typedef struct mozart_event {
                union {
                        key_event key;
                        misc_event misc;
                } event;
                EventType type; /* EVENT_KEY/EVENT_MISC */
        } mozart_event;

        typedef void (*EventCallback) (mozart_event event, void *param);

        typedef struct {
                int sockfd;
                EventCallback callback;
                void *param;
        } event_handler;

        extern event_handler *mozart_event_handler_get(EventCallback callback, void *param);
        extern int mozart_event_send(mozart_event);
        extern void mozart_event_handler_put(event_handler *handler);

#ifdef  __cplusplus
}
#endif
#endif	/** __KEY_INTERFACE_H__ **/
