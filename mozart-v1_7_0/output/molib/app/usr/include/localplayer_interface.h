/**
 * @file localplayer_interface.h
 * @brief API for localplayer
 * @author xllv <xinliang.lv@ingenic.com>
 * @version 1.0.0
 * @date 2015-01-14
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

#ifndef _LOCALPLAYER_INTERFACE_H_
#define _LOCALPLAYER_INTERFACE_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * @brief Startup local playback, and scanning the local music.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_startup(void);

	/**
	 * @brief Shutdown local playback, and release of resources.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_shutdown(void);

	/**
	 * @brief Start a new playback.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_start_playback(void);

	/**
	 * @brief Stop the current playback.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_stop_playback(void);

	/**
	 * @brief Execute pause or unpause command.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_play_pause(void);

	/**
	 * @brief Play the next music.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_next_music(void);

	/**
	 * @brief Play the previous music.
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
	extern int mozart_localplayer_prev_music(void);

	/**
	 * @brief scan sdcard mount dir(/mnt/sdcard). TODO
	 */
	extern void mozart_localplayer_scan(void);

	/*
	 * register scan callback function
	 */
	typedef void (*scan_callback)(void);
	typedef void (*scan_callback1)(char *musiclist);
	extern void mozart_localplayer_scan_callback(scan_callback scan_1music_callback, scan_callback1 scan_allmusic_callback);

        /**
         * @brief if scanning.
         *
         * @return return true if scanning, return false if not in scanning mode.
         */
        extern bool mozart_localplayer_is_scanning(void);

        /**
         * @brief get music list in /mnt/sdcard.
         *
         * @return music list(json string format), example:
         * [{"name":"shuzhuangtai.ape","url":"\/mnt\/sdcard\/system\/shuzhuangtai.ape"},
         * {"name":"sinian.ape","url":"\/mnt\/sdcard\/system\/sinian.ape"},
         * {"name":"wanwusheng.mp3","url":"\/mnt\/sdcard\/system\/wanwusheng.mp3"},
         * {"name":"zhou.mp3","url":"\/mnt\/sdcard\/system\/zhou.mp3"},
         * {"name":"for_young.wav","url":"\/mnt\/sdcard\/system\/for_young.wav"},
         * {"name":"aa.mp3","url":"\/mnt\/sdcard\/music\/download\/aa.mp3"},
         * {"name":"888 999.mp3","url":"\/mnt\/sdcard\/music\/download\/888 999.mp3"},
         * {"name":"8 88.mp3","url":"\/mnt\/sdcard\/music\/download\/8 88.mp3"},
         * {"name":"888 99.mp3","url":"\/mnt\/sdcard\/music\/download\/888 99.mp3"}]
         */
        extern char *mozart_localplayer_get_musiclist(void);

	/**
	 * @brief supported playmode.
	 */
	enum localplayer_playmode {
		/**
		 * @brief play from 1 to 2 to 3 to ..., and loop forever.
		 */
		PLAYMODE_ORDER = 1,
		/**
		 * @brief play a random music from musiclist, and play forever.
		 */
		PLAYMODE_RANDOM = 2,
		/**
		 * @brief useless
		 */
		PLAYMODE_MAX = 3,
	};
	/**
	 * @brief set localplayer playmode
	 *
	 * @param mode playmode
	 *
	 * @return On success returns 0, return -1 if an error occurred.
	 */
        extern int mozart_localplayer_playmode(enum localplayer_playmode mode);
#ifdef __cplusplus
}
#endif

#endif /* _LOCALPLAYER_INTERFACE_H_ */
