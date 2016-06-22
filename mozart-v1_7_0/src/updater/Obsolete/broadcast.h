#ifndef __BROADCAST_H__
#define __BROADCAST_H__

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct version_t {
	/**
	 * @brief current update process
	 */
	int status;
	/**
	 * @brief machine version(current version)
	 */
	int machine_version;
	/**
	 * @brief lasted version
	 */
	int lastest_version;
} version_t;

extern int start_version_broadcast_thread(void);
extern void stop_version_broadcast(void);

#ifdef  __cplusplus
}
#endif

#endif //__BROADCAST_H__
