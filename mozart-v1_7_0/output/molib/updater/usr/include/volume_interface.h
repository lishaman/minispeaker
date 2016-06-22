/**
 * @file volume_interface.h
 * @brief volume get and set interface header.
 * @author ylguo <yonglei.guo@ingenic.com>
 * @version 0.1
 * @date 2015-01-23
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

#ifndef _VOLUME_INTERFACE_H_
#define _VOLUME_INTERFACE_H_

#ifdef __cplusplus
extern "C"{
#endif


/**
 * @brief volume domain
 */
typedef enum volume_domain {
	/**
	 * @brief music volume
	 */
	MUSIC_VOLUME,
	/**
	 * @brief bt volume
	 */
	BT_CALL_VOLUME,
	BT_MUSIC_VOLUME,
	VOLUME_MAX
} volume_domain;


/**
 * @brief the readable string of volume_domain
 */
extern char *volume_domain_str[VOLUME_MAX];

/**
 * @brief get system volume.
 *
 * @return volume value.
 */
extern int mozart_volume_get(void);


/**
 * @brief set system volume.
 *
 * @param volume [in] volume to set.
 * @param domain [in] who's volume you wish to set.
 *
 * @return return 0 on Success, else return -1.
 */
extern int mozart_volume_set(int volume, volume_domain domain);

/**
 * @brief set system volume mute
 *
 * @return 0 on success otherwise negative error code.
 */
int mozart_volume_mute();

/**
 * @brief set system volume unmute
 *
 * @return 0 on success otherwise negative error code.
 */
int mozart_volume_unmute();

#ifdef __cplusplus
}
#endif

#endif // __VOLUME_INTERFACE_H_
