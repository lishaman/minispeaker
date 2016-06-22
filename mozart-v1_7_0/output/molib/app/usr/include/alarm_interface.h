/**
 * @file alarm_interface.h
 * @brief alarm_manager interface file
 * @author jian gao <jian.gao@ingenic.com>
 * @version 0.1
 * @date 2015-03-23
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

#ifndef _ALARM_INTERFACE_H_
#define _ALARM_INTERFACE_H_

#ifdef  __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <stdlib.h>
#include <fcntl.h>
#include "linklist_interface.h"

#define NUM_ALARMS 8
#define DESCRIPTION_LENGTH 9


typedef struct Alarm{
	int hour;
	int minute;
	int weekdays_active[7];
	int enabled;
	time_t timestamp;
	int alarm_id;
	int volume;
	char programData[256];
	char programUrl[128];
} Alarm;

Alarm alarm_local[8];

#define ALARM_FILE_PATH "/usr/data/alarm.list"
typedef int (*CallBackFun)(const char *p);

typedef struct alarm_notify_msg_s {
	long msgtype;
	char msgtext[32];
	Alarm alarm_now;
} alarm_notify_msg_t;
//////////////////////////////

#define OUT
#define IN

#define U32 unsigned int

#define S32 signed int

#define U16 unsigned short
#define S16 signed short
#define U8 unsigned char
#define S8 signed char

//#define bool U8
#define ULONG unsigned long

#define list_entry(node, type, member) ((type *)((U8*)(node) - (U32)(&((type *)0)->member)))


typedef struct LIST1 {
	struct LIST1 *prev;
	struct LIST1 *next;
} LIST1;

extern void list_init1(LIST1 *head);
extern void list_insert1(LIST1 *head, LIST1 *node);
extern void list_insert_spec1(LIST1 *head, LIST1 *node);
extern void list_delete1(LIST1 *node);
extern bool is_list_last1(LIST1 *node);
extern void list_insert_behind1(LIST1 *head, LIST1 *node);


enum {
	SUCCESS = 0,
	FAIL = -1,
	MANY_CASES = 100,
	EXIST = 101,
	ALL = 102
};

/*for search fit number*/
struct fit {
	char id;
	char num;
};


struct alarm {
	struct LIST1 list;
	/*bitmap according week in one alarm*/
	U8 rflag;
	U8 id;
	/*bitmap for set which day*/
	U8 wflag;
	/*for insert which queue*/
	U8 week;

	/*just for user*/
	bool weekdays_active[7];
	U8 hour;
	U8 minute;
	U8 second;
	U8 enable;
	ULONG timestamp;
};

/*user api*/
struct alarm *alarm_read(U8 id);
U32 alarm_add(U8 id, U8 week, U8 hour, U8 minute, U8 second, bool repeat);
U32 alarm_disable(U8 id);
U32 alarm_enable(U8);

/*return latest alarm to user*/
struct alarm *get_new_alarm(struct tm *now, Alarm a[50], int size);

/*system lib*/
struct tm *system_time_get(void);
struct alarm *alarm_search(char id, char week);
int compare(char c, struct fit in[7], OUT char fout[7], OUT char *num);
int min(IN char in[7], OUT char out[7], char *num);
struct alarm *_get_new_alarm(int start, int end, struct tm *now);
void print1(U8 week_queue);
int alarm_init(struct tm *now, Alarm array[50], int size);

////////////////////////////

//alarm interface
extern void mozart_alarm_init();
extern void mozart_alarm_list_save_to_file(List *list);
extern void mozart_alarm_list_load_from_file();
extern void mozart_alarm_add_new_alarm(Alarm *alarm);
extern void mozart_alarm_delete_alarm(int alarm_id);
extern Alarm * mozart_alarm_get_alarm_by_id(int alarm_id);
extern void mozart_alarm_update_alarm(int alarm_id, Alarm* alarm);
extern void mozart_alarm_reverse_list(List *list);
extern void mozart_alarm_sort_list(List *list);
extern void mozart_alarm_destroy_list(List *list);
extern List* mozart_alarm_get_alarm_list();
extern long mozart_alarm_get_alarm_list_size(List* list);
extern void mozart_alarm_enable_alarm(int alarm_id);
extern void mozart_alarm_disable_alarm(int alarm_id);

//cgi use
extern void mozart_alarm_init_to_app();
extern void mozart_print_to_app(List *list);
extern void mozart_alarm_sort_list_to_app(List *list);
extern void mozart_alarm_list_load_from_file_to_app();
extern void mozart_alarm_add_new_alarm_to_app(Alarm *alarm);

//time interface
extern void mozart_time_set_clock_timestamp(int timestamp);
extern int mozart_time_get_clock_timestamp();
extern void mozart_time_enable_internet_sync();
extern void mozart_time_disable_internet_sync();
extern void mozart_time_sync_internet_time();
extern bool mozart_time_get_internet_sync_status();
extern void mozart_time_set_shutdown_timer();
extern void mozart_time_get_shutdown_timer();


extern void alarm_toggle_enable(Alarm *alarm);
extern void alarm_schedule_wakeup(Alarm *alarm);
extern void alarm_cancel_wakeup(Alarm *alarm);
extern void alarm_reset(Alarm *alarm);
extern void reschedule_wakeup(Alarm *alarms);
extern bool alarm_is_one_time(Alarm *alarm);
extern bool alarm_is_set(Alarm *alarm);
extern void alarm_set_language(void);
extern bool alarm_has_description(Alarm *alarm);
extern int get_next_free_slot(Alarm *alarms);


/**
 * @brief alarm mode status
 */
typedef enum alarm_mode {
	/**
	 * @brief alarm is creating AP.
	 */
        TODO,
	/**
	 * @brief alarm is not in AP mode or STA mode.
	 */
} alarm_mode_t;

typedef struct alarm_info{
	/*
	 * Concurrent alarm Working mode.
	 */
	alarm_mode_t alarm_mode;
	/*
	 * Ap or Softap ssid name.
	 */
	char ssid[32];

} alarm_info_t;

/**
 * @brief readable string format of alarm_mode, NOTE: maintainer of network_manager should keep pace with `enum alarm_mode`!!!
 */
extern const char *alarm_mode_str[];


/**
 * @brief alarm-get and alarm-set command.
 */
typedef enum alarm_cmd {
	/**
	 * @brief  get current alarm list(refer to alarm_mode_t).
	 */
	GET_ALARM_LIST,
	/**
	 * @brief add a new alarm to alarmlist.
	 */
	ADD_NEW_ALARM,
	/**
	 * @brief update a alarm.
	 */
	UPDATE_ALARM,
	/**
	 * @brief delete a alarm.
	 */
	DELETE_ALARM
} alarm_cmd_t;

/**
 * @brief readable string format of alarm_cmd, NOTE: maintainer of network_manager should keep pace with `enum alarm_cmd`!!!
 *
 */
extern const char *alarm_cmd_str[];


/**
 * @brief alarm request ctrl msg
 */
typedef struct alarm_ctl_msg_s {
	/**
	 * @brief name of client, should euqal to alarm_client_register-->name
	 * in the same client.
	 */
	char name[32];
	/**
	 * @brief alarm request command.
	 */
	alarm_cmd_t cmd;
	/**
	 * @brief if you wish to force to switch alarm mode
	 * if the current alarm mode is the same as desired.
	 */
	bool force;

} alarm_ctl_msg_t;

/**
 * @brief register infomation to network_manager.
 */
typedef struct alarm_client_register {
	/**
	 * @brief the pid of client
	 */
	long pid;
	/**
	 * @brief USELESS so far.
	 */
	int reset;
	/**
	 * @brief priority of client, default is 3, USELESS so far.
	 */
	int priority;
	/**
	 * @brief the unique name of the client.
	 */
	char name[32];
} ALARM_CLI_REG;


/**
 * @brief get current alarm mode,
 * Do not need to register firstly, you can invoke it in any where.
 *
 * @return  return current alarm mode
 */
extern alarm_info_t get_alarm_mode(void);
extern alarm_info_t get_alarm_mode_to_app(void);
extern void add_alarm_mode_to_app(Alarm* alarm);

/**
 * @brief request a alarm mode
 *
 * @param msg [in] alarm request msg
 *
 * @return return true on Success, false on failure.
 */
extern bool request_alarm_mode(alarm_ctl_msg_t msg);

/**
 * @brief register to network_manager
 *
 * @param register_info [in] the client's description.
 * @param ptr [in] callback func on alarm mode changed.
 *
 * @return return 0 on Success, -1 on failure.
 */
extern int register_to_alarm_manager(struct alarm_client_register register_info, int (*ptr)(const char *p,Alarm a));

/**
 * @brief un register from network_manager
 */
extern void unregister_from_alarm_manager(void);

#ifdef  __cplusplus
}
#endif

#endif //_ALARM_INTERFACE_H_
