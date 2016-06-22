/**
 * @file power_interface.h
 * @brief Power Manager API
 * @author mlfeng <mingli.feng@ingenic.com>
 * @version 1.0.0
 * @date 2015-01-13
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
#ifndef _POWER_INTERFACE_H_
#define _POWER_INTERFACE_H_

/**
 * @brief   power supply type
 *
 */
enum power_supply_type {
	/**
	 * @brief battery
	 */
	POWER_SUPPLY_TYPE_BATTERY = 0,
	/**
	 * @brief not support 
	 */
	POWER_SUPPLY_TYPE_UPS,
	/**
	 * @brief ac 
	 */
	POWER_SUPPLY_TYPE_MAINS,
	/**
	 * @brief usb 
	 */
	POWER_SUPPLY_TYPE_USB,
	/**
	 * @brief not support 
	 */
	POWER_SUPPLY_TYPE_USB_DCP,
	/**
	 * @brief not support
	 */
	POWER_SUPPLY_TYPE_USB_CDP,
	/**
	 * @brief not support
	 */
	POWER_SUPPLY_TYPE_USB_ACA,
};

/**
 * @brief   battery info
 *
 */
enum battery_info {
	/**
	 * @brief Battery OCV voltage
	 */
	VOLTAGE_NOW = 0,
	/**
	 * @brief Battery OCV voltage max
	 */
	VOLTAGE_MAX,
	/**
	 * @brief Battery OCV voltage min
	 */
	VOLTAGE_MIN,
};

/**
 * @brief   battery status
 *
 */
enum battery_status {
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_STATUS_UNKNOWN = 0,
	/**
	 * @brief Battery charging
	 */
	POWER_SUPPLY_STATUS_CHARGING,
	/**
	 * @brief Battery discharging
	 */
	POWER_SUPPLY_STATUS_DISCHARGING,
	/**
	 * @brief Battery not charging 
	 */
	POWER_SUPPLY_STATUS_NOT_CHARGING,
	/**
	 * @brief Battery full 
	 */
	POWER_SUPPLY_STATUS_FULL,
};

/**
 * @brief   battery health status
 *
 */
enum battery_health {
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_HEALTH_UNKNOWN = 0,
	/**
	 * @brief Battery health good
	 */
	POWER_SUPPLY_HEALTH_GOOD,
	/**
	 * @brief Never return
	 */
	POWER_SUPPLY_HEALTH_OVERHEAT,
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_HEALTH_DEAD,
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_HEALTH_OVERVOLTAGE,
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_HEALTH_UNSPEC_FAILURE,
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_HEALTH_COLD,
};

/**
 * @brief   battery technology type
 *
 */
enum battery_technology {
	/**
	 * @brief Battery offline
	 */
	POWER_SUPPLY_TECHNOLOGY_UNKNOWN = 0,
	/**
	 * @brief Never return
	 */
	POWER_SUPPLY_TECHNOLOGY_NiMH,
	/**
	 * @brief Battery Li poly
	 */
	POWER_SUPPLY_TECHNOLOGY_LION,
	/**
	 * @brief Never return
	 */
	POWER_SUPPLY_TECHNOLOGY_LIPO,
	/**
	 * @brief Never return
	 */
	POWER_SUPPLY_TECHNOLOGY_LiFe,
	/**
	 * @brief Never return
	 */
	POWER_SUPPLY_TECHNOLOGY_NiCd,
	/**
	 * @brief Never return
	 */
	POWER_SUPPLY_TECHNOLOGY_LiMn,
};

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief  system suspend
 *
 * @return success:0
 */
extern int mozart_power_suspend(void);

/**
 * @brief  system resume
 */
extern void mozart_power_resume(void);

/**
 * @brief  system power off
 */
extern void mozart_power_off(void);

/**
 * @brief  get system battery/usb/ac online status
 *
 * @param  type [in] battery/usb/ac
 *
 * @return 1 online, 0 offline
 */
extern int mozart_power_supply_online(enum power_supply_type type);

/**
 * @brief  get system battery/usb/ac current value
 *
 * @param  type [in] battery/usb/ac
 *
 * @return current value
 */
extern int mozart_power_supply_current(enum power_supply_type type);

/**
 * @brief  get system battery/usb/ac voltage value
 *
 * @param  type [in] battery/usb/ac
 *
 * @return voltage value
 */
extern int mozart_power_supply_voltage(enum power_supply_type type);

/**
 * @brief  get battery voltage
 *
 * @param  info [in] battery current-ocv/man/min
 *
 * @return voltage value
 */
extern int mozart_get_battery_voltage(enum battery_info info);

/**
 * @brief  get battery capacity
 *
 * @return battery capacity(%)
 */
extern int mozart_get_battery_capacity(void);

/**
 * @brief  get battery status
 *
 * @return battery status
 */
extern enum battery_status mozart_get_battery_status(void);

/**
 * @brief  get battery health status
 *
 * @return battery health status
 */
extern enum battery_health mozart_get_battery_health(void);

/**
 * @brief  get battery technology type
 *
 * @return battery technology type
 */
extern enum battery_technology mozart_get_battery_technology(void);

#ifdef __cplusplus
}
#endif

#endif /* _POWER_INTERFACE_H_ */
