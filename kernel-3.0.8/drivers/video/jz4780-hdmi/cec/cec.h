
#ifndef CEC_H_
#define CEC_H_

#include "../util/types.h"
#include "../core/control.h"
#include "../bsp/access.h"
#include <linux/delay.h>


//opcode
//u8 POLLING  
#define IMAGE_VIEW_ON 0x04
#define ACTIVE_SOURCE 0X82
#define CEC_STANDBY 0x36
#define INACTIVE_SOURCE 0x9D



#define CEC_MAX_SIZE 16
#define CEC_TIME_OUT 5
void halCEC_WriteByte(u8 Addr, u8 bit);
u8 halCEC_ReadByte(u8 Addr);
void sendmessage(u8 *value,int size);
void cec_callback(void *param);

#endif
