#include <stdio.h>
#include <unistd.h>
#include <error.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include "battery_capacity.h"
#include "power_interface.h"
#include "tips_interface.h"

void *battery_capacity_thread(void *arg)
{
	int capacity;
	while(1)
	{
		sleep(120);
		if(mozart_power_supply_online(POWER_SUPPLY_TYPE_BATTERY))
		{
			capacity = mozart_get_battery_capacity();
			if(capacity == -1){
				return NULL;
			}
			printf("Now battery supply and battery_capacity is %d!!\n", capacity);
			if(capacity < 15)
			{
				mozart_play_key("charge_prompt");
				printf("Now battery_capacity is %d , please charge battery\n", capacity);
			}
		}
	}
	return NULL;
}

void register_battery_capacity(void)
{
	pthread_t recv_event;
	if(0 != pthread_create(&recv_event, NULL, battery_capacity_thread, NULL))
		printf("Can't create battery_capacity!\n");
	pthread_detach(recv_event);
}

void unregister_battery_capacity(void)
{

}
