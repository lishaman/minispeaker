#ifndef __UPDATER_H__
#define __UPDATER_H__

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @brief current update status.
 */
typedef enum update_status {
	/**
	 * @brief do nothing now.
	 */
	UPDATE_DO_NOTHING = 0,
	/**
	 * @brief detecting if new version released.
	 */
	UPDATE_DETECTING = 1,
	/**
	 * @brief new version found.
	 */
	UPDATE_FOUND,
	/**
	 * @brief NO new version found.
	 */
	UPDATE_NOTFOUND,
	/**
	 * @brief downloading and verfing and writing update package to flash.
	 */
	UPDATE_UPDATING,
	/**
	 * @brief update successfully.
	 */
	UPDATE_UPDATED_SUCCESS,
	/**
	 * @brief updated done, but failed.
	 */
	UPDATE_UPDATED_FAIL,
} update_status;

/**
 * @brief string format of all update_status, index is update_status.
 */
extern char *update_status_str[];

extern update_status get_status(void);

#ifdef  __cplusplus
}
#endif

#endif //__UPDATER_H__
