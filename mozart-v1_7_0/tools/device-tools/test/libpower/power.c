#include <stdio.h>
#include <unistd.h>

#include "power_interface.h"

int main(void)
{
	int online = 0;
	int current = 0;
	int voltage = 0;
	int ocv_voltage = 0;
	int capacity = 0;
	enum battery_status status = POWER_SUPPLY_STATUS_UNKNOWN;
	enum battery_health health = POWER_SUPPLY_HEALTH_UNKNOWN;
	enum battery_technology technology = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;

	online = mozart_power_supply_online(0);
	printf("===>battery online = %d\n", online);
	online = mozart_power_supply_online(3);
	printf("===>usb online = %d\n", online);
	online = mozart_power_supply_online(2);
	printf("===>ac online = %d\n", online);

	current = mozart_power_supply_current(0);
	printf("===>battery current = %d\n", current);
	current = mozart_power_supply_current(3);
	printf("===>usb current = %d\n", current);
	current = mozart_power_supply_current(2);
	printf("===>ac current = %d\n", current);


	voltage = mozart_power_supply_voltage(0);
	printf("===>battery voltage = %d\n", voltage);
	voltage = mozart_power_supply_voltage(3);
	printf("===>usb voltage = %d\n", voltage);
	voltage = mozart_power_supply_voltage(2);
	printf("===>ac voltage = %d\n", voltage);

	ocv_voltage = mozart_get_battery_voltage(0);
	printf("===>battery now = %d\n", ocv_voltage);
	ocv_voltage = mozart_get_battery_voltage(1);
	printf("===>battery max = %d\n", ocv_voltage);
	ocv_voltage = mozart_get_battery_voltage(2);
	printf("===>battery min = %d\n", ocv_voltage);

	capacity = mozart_get_battery_capacity();
	printf("========++>battery capacity = %d\n", capacity);

	status = mozart_get_battery_status();
	printf("========+>status = %d\n", status);

	health = mozart_get_battery_health();
	printf("========+>health = %d\n", health);

	technology = mozart_get_battery_technology();
	printf("========+>technology = %d\n", technology);

	return 0;
}
