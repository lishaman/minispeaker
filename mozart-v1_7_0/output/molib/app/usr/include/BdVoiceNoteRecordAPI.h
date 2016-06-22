#ifndef __BD_VOICE_NOTE_RECORD_API_H_
#define __BD_VOICE_NOTE_RECORD_API_H_
#include "shopping_demo_switch.h"
#ifdef  __cplusplus
extern "C" {
#endif
#if NOTE_RECORD_DEMO_ENABLED

/*
 *  ========================= Interface for the app to use. ========================= 
 */

int vr_start_voice_note();

/*
 *  ========================= register voice note recording event handlers ========================= 
 */
void vr_reg_note_record_started_handler(void(*handler)(void));
void vr_reg_note_record_finished_handler(void(*handler)(const char* noteFileName));
void vr_reg_note_record_failed_handler(void(*handler)(int reason));

#endif

#ifdef  __cplusplus
}
#endif
#endif
