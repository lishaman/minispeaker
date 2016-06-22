/**
 * @file ota_interface.h
 * @brief OTA interface API
 * @author jgao <jian.gao@ingenic.com>
 * @version 1.0.0
 * @date 2015-08-19
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
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
#ifndef _OTA_INTERFACE_H_
#define _OTA_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief ota_init
 *
 * @return success:0
 */

extern int mozart_ota_seturl(char* url,int urlsize, int fd);

extern char* mozart_ota_geturl(int fd);

extern int mozart_ota_callback_checkkey(char* path);

extern int mozart_ota_get_state(void);

extern unsigned int mozart_ota_get_version(int fd);

extern int mozart_ota_set_version(unsigned int current_version,int fd);

extern void mozart_ota_start_update();

extern void mozart_ota_start_recovery();

extern int mozart_ota_init(void);

extern void mozart_ota_close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* _OTA_INTERFACE_H_ */
