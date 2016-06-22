/**
 * @file movr_interface.h
 * @brief API for voice recognition
 * @author xllv <xinliang.lv@ingenic.com>
 * @version 1.0.0
 * @date 2015-01-28
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

#ifndef __VR_HEAD__
#define __VR_HEAD__

#ifdef  __cplusplus
extern "C" {
#endif

	/**
	 * @brief To print DEBUG information.
	 *
	 * @param x The format string.
	 * @param y A variable length argument.
	 */
#define DEBUG(x,y...)	(printf("DEBUG [ %s : %s : %d] "x"\n",__FILE__, __func__, __LINE__, ##y))

	/**
	 * @brief To print ERROR information.
	 *
	 * @param x The format string.
	 * @param y A variable length argument.
	 */
#define ERROR(x,y...)	(printf("ERROR [ %s : %s : %d] "x"\n", __FILE__, __func__, __LINE__, ##y))

	/**
	 * @brief Voice recognition startup.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_vr_startup(void);

	/**
	 * @brief Voice recognition shutdown.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_vr_shutdown(void);

	/**
	 * @brief Voice recognition stop play.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_vr_stop(void);

	/**
	 * @brief Start Online Voice Recognition.
	 * Which is called when user pressed button down                                                   
	 * When OfflineVR triggers the OnlineVR, there is no need to call this.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_vr_online_vr_start(void);

	/**
	 * @brief Cancel ongoing OnlineVR
	 * For instance when user short press button to cancel the OnlineVR.
	 *
	 * @return 
	 */
	extern int mozart_vr_online_vr_cancel(void);

	/**
	 * @brief Notify the OnlineVR, that user has finished talking.
	 * User pressed button up, to finish to recording actively
	 *
	 * @return 
	 */
	extern int mozart_vr_online_vr_finish(void);

	/**
	 * @brief Play the next music in voice recognition result.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_vr_next_music(void);

	/**
	 * @brief Play the previous music in voice recognition result.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_vr_prev_music(void);

	/**
	 * @brief Gets the running state of voice recognition.
	 *
	 * @return 1 indicate voice recognition is running, 0 indicate voice
	 * recognition is stopped.
	 */
	extern int mozart_vr_get_status(void);

#ifdef  __cplusplus
}
#endif
#endif				/* __VR_HEAD__ */
