#ifndef __PLAY_INTERFACE_H__
#define __PLAY_INTERFACE_H__

#include <stdbool.h>
#include <pthread.h>

#ifdef  __cplusplus
extern "C" {
#endif
	/**
	 * @brief The player status.
	 */
	typedef enum {
		/**
		 * @brief init status or error.
		 */
		PLAYER_UNKNOWN,
		/**
		 * @brief recv url, prepare to play.
		 */
		PLAYER_TRANSPORT,
		/**
		 * @brief Playing state
		 */
		PLAYER_PLAYING,
		/**
		 * @brief Pause state
		 */
		PLAYER_PAUSED,
		/**
		 * @brief Stop state
		 */
		PLAYER_STOPPED,
	} player_status_t;

	/**
	 * @brief string array of player_status.
	 *
	 * char *player_status_str[] = {
	 *     [PLAYER_UNKNOWN] = "PLAYER_UNKNOWN",
	 *     [PLAYER_TRANSPORT] = "PLAYER_TRANSPORT",
	 *     [PLAYER_PLAYING] = "PLAYER_PLAYING",
	 *     [PLAYER_PAUSED] = "PLAYER_PAUSED",
	 *     [PLAYER_STOPPED] = "PLAYER_STOPPED",
	 * };
	 */
	extern char *player_status_str[];

	/**
	 * @brief Express player current state.
	 */
	typedef struct {
		/**
		 * @brief uuid of current controller.
		 */
		char uuid[32];
		/**
		 * @brief Current play status.
		 */
		player_status_t status;
		/**
		 * @brief Current play position.
		 */
		int position;
		/**
		 * @brief The duration of the audio.
		 */
		int duration;
	} player_snapshot_t;

	/**
	 * @brief callback on player status changed.
	 */
	struct player_handler;
	typedef int (*player_callback)(player_snapshot_t *snapshot, struct player_handler *handler, void *param);

	/**
	 * @brief handler of player.
	 */
	typedef struct player_handler {
		/**
		 * @brief user uuid.
		 */
		char uuid[32];
		/**
		 * @brief internal data, guess it.
		 */
		int fd_sync;
		/**
		 * @brief internal data, guess it.
		 */
		int fd_async;
		/**
		 * @brief user callback.
		 */
		player_callback callback;
		/**
		 * @brief internal data, guess it.
		 */
		pthread_t handler_thread;
		/**
		 * @brief internal data, guess it.
		 */
		bool _service_stop;
		/**
		 * @brief user private data.
		 */
		void *param;
	} player_handler_t;

	/**
	 * @brief apply a handler from player
	 *
	 * @param uuid [in] uuid of your module, will store in player_handler_t->uuid, limit to 32Bytes.
	 * @param callback [in] callback on player status change.
	 * @param param [in] private data, will be passed as the last argument of callback.
	 *
	 * @return return player handler on success, return NULL otherwise.
	 */
	extern player_handler_t *mozart_player_handler_get(char *uuid, player_callback callback, void *param);

	/**
	 * @brief destroy a player handler.
	 *
	 * @param handler [in] player handler to be destroyed, return directly if handler is NULL.
	 */
	extern void mozart_player_handler_put(player_handler_t *handler);

	/**
	 * @brief play a url. you will recieve PLAYER_TRANSPORT and/or PLAYER_PLAYING status on callback.
	 * Generally, You should stop the already playing music before play new song.
	 *
	 * @param handler [in] player handler
	 * @param url [in] target url
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_playurl(player_handler_t *handler, char *url);

	/**
	 * @brief play a url(ONLY cache to cache-min size, then pause)
	 * mozart_player_cacheurl() is the same as invoke mozart_player_pause() following mozart_player_playurl().
	 * Generally, You should stop the already playing music before play new song.
	 *
	 * @param handler [in] player handler
	 * @param url [in] target url
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_cacheurl(player_handler_t *handler, char *url);

	/**
	 * @brief pause the music. you will recieve PLAYER_PAUSED status on callback.
	 *
	 * @param handler [in] player handler
	 * @param url [in] target url
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_pause(player_handler_t *handler);

	/**
	 * @brief resume the music. you will recieve PLAYER_PLAYING status again on callback.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_resume(player_handler_t *handler);

	/**
	 * @brief seek the music, if no status change, will not invoke callback.
	 *
	 * @param handler [in] player handler
	 * @param position [in] target position(Unit: seconds)
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_seek(player_handler_t *handler, int position);

	/**
	 * @brief stop the music, you will recieve PLAYER_STOPPED status on callback.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_stop(player_handler_t *handler);

	/**
	 * @brief set player volume
	 *
	 * @param handler [in] player handler
	 * @param volume [in] target volume(range: [0-100])
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_volset(player_handler_t *handler, int volume);
	/**
	 * @brief get player volume
	 *
	 * @param handler [in] player handler
	 *
	 * @return current player volume
	 */
	extern int mozart_player_volget(player_handler_t *handler);

	/**
	 * @brief get player snapshot.
	 *
	 * @param handler [in] player handler
	 *
	 * @return player snapshot on success(MUST be freed with free()), return NULL otherwise
	 */
	extern player_snapshot_t *mozart_player_getsnapshot(player_handler_t *handler);

	/**
	 * @brief get uuid of current player controller, MAY NOT be you.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return uuid on success(MUST be freed with free()), return NULL otherwise
	 */
	extern char *mozart_player_getuuid(player_handler_t *handler);

	/**
	 * @brief get url of current playing.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return url on success(MUST be freed with free()), return NULL otherwise
	 */
	extern char *mozart_player_geturl(player_handler_t *handler);

	/**
	 * @brief get status of current playing.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return status on success(MUST be freed with free()), return PLAYER_UNKNOWN otherwise
	 */
	extern player_status_t mozart_player_getstatus(player_handler_t *handler);

	/**
	 * @brief get postion of current playing.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_getpos(player_handler_t *handler);

	/**
	 * @brief get duration of current playing.
	 *
	 * @param handler [in] player handler
	 *
	 * @return return 0 on success(MUST be freed with free()), return -1 otherwise
	 */
	extern int mozart_player_getduration(player_handler_t *handler);

	/**
	 * @brief a easy mp3player supported by libmad, ONLY used for libtips now.
	 *
	 * @param mp3_file [in] mp3 file path, support http:// protocol, NOT support file:// protocol.
	 * @param fade_msec [in] fade in/out time(Unit: milliseconds), ONLY used in alsa mode, pass 0 on oss mode.
	 * @param fade_volume [in] fake in/out target volume, ONLY used in alsa mode, pass 0 in oss mode.
	 * @param is_sync [in] need wait this mp3 play over?
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_playmp3(char *mp3_file, int fade_msec, int fade_volume, int is_sync);

	/**
	 * @brief stop all mp3 played by mozart_player_playmp3().
	 *
	 * @param is_sync [in] need wait stop?
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_stopmp3(int is_sync);


	/**
	 * @brief stop music played by player, DO NOT care if player controller is you.
	 *
	 * @param handler [in] player handler.
	 * @param is_sync [in] need wait stop?
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_stopall(player_handler_t *handler, bool is_sync);

	/**
	 * @brief pause music(if playing) played by player, DO NOT care if player controller is you.
	 *
	 * @param handler [in] player handler.
	 * @param is_sync [in] need wait stop?
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_pauseall(player_handler_t *handler, bool is_sync);

	/**
	 * @brief resume music(if paused) played by player, DO NOT care if player controller is you.
	 *
	 * @param handler [in] player handler.
	 * @param is_sync [in] need wait stop?
	 *
	 * @return return 0 on success, return -1 otherwise
	 */
	extern int mozart_player_resumeall(player_handler_t *handler, bool is_sync);
#ifdef  __cplusplus
}
#endif
#endif	/* __PLAY_INTERFACE_H__ */
