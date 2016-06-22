#ifndef __BATTERY_CAPACITY_H__
#define __BATTERY_CAPACITY_H__

#include <unistd.h>

#ifdef  __cplusplus
extern "C" {
#endif

	void register_battery_capacity(void);

	void unregister_battery_capacity(void);

#ifdef  __cplusplus
}
#endif
#endif
