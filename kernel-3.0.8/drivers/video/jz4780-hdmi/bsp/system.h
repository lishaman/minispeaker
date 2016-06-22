/*
 * system.h
 *
 *  Created on: Jun 25, 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 *
 *
 * 	@note: this file should be re-written to match the environment the
 * 	API is running on
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "../util/types.h"

typedef enum
{
//	RX_INT = 1,
	TX_INT = 3,
	TX_WAKEUP_INT,
} interrupt_id_t;

void system_SleepMs(unsigned ms);
int system_ThreadCreate(void* handle, thread_t pFunc, void* param);
int system_ThreadDestruct(void* handle);

int system_Start(thread_t thread);

int system_InterruptDisable(interrupt_id_t id);
int system_InterruptEnable(interrupt_id_t id);
int system_InterruptAcknowledge(interrupt_id_t id);
int system_InterruptHandlerRegister(interrupt_id_t id, handler_t handler, void * param);
int system_InterruptHandlerUnregister(interrupt_id_t id);

#endif /* SYSTEM_H_ */
