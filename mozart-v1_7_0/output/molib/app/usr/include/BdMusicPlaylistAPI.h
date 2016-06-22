#ifndef __BD_MUSIC_PLAYLIST_API_H_
#define __BD_MUSIC_PLAYLIST_API_H_
#include "BdOnlineVRAPI.h"
#ifdef  __cplusplus
extern "C" {
#endif

typedef enum VR_PLAYLIST_TASK_STATE
{
    VR_PLAYLIST_TASK_OK = 0,                    /** REQUEST COMPLETE */
    VR_PLAYLIST_TASK_WAITING,                   /* (LIBRARY INTERNAL USE) */
    VR_PLAYLIST_TASK_DETAIL_LOAD_ERROR,         /** FAILED TO LOAD DETAILS OF REQUESTED SONG (GENERAL ERROR, YOU MAY TRY TO MOVE TO NEXT ONE) */
    VR_PLAYLIST_TASK_PAGE_LOAD_ERROR,           /** FAILED TO LOAD MORE OF SONG LIST (POSSIBLY DUE TO NETWORK PROBLEM) */
    VR_PLAYLIST_TASK_CANCELED,                  /* CANCELED DUE TO LIBRARY SHUTDOWN */
    VR_PLAYLIST_TASK_UNKNOWN,                   /* UNKNOWN ERROR */
    VR_PLAYLIST_TASK_PLAYLIST_SERVER_NOT_OPEN,  /* LIBRARY IS NOT INITIALIZED */
    VR_PLAYLIST_TASK_LIST_EMPTY,                /** vr_get_curr_song CALLED, BUT THE PLAYLIST IS EMPTY */
    VR_PLAYLIST_NO_USER_LOGIN,			/** Need login */
    VR_PLAYLIST_TASK_MALLOC_ERROR,              /* INTERNAL ERROR (THE SYSTEM IS RUNNING LOW ON MEMORY)*/
    // Reached list start/end
    VR_PLAYLIST_END_REACHED,                    /** vr_get_next_song CALLED, BUT ALREADY AT THE END OF LIST */
    VR_PLAYLIST_START_REACHED,                  /** vr_get_prev_song CALLED, BUT ALREADY AT THE BEGINNING OF LIST */
    VR_PLAYLIST_PLAY_URL_ERR_NETWORK,           /** unable to get play url due to network issues */
    VR_PLAYLIST_PLAY_URL_ERR_LEGAL,             /** unable to get play url due to legal resons (song has been taken offline) */
}VR_PLAYLIST_TASK_STATE;

int vr_get_total_songs(VR_PLAYLIST_TASK_STATE *err);
voice_cmd_music_t* vr_get_next_song(VR_PLAYLIST_TASK_STATE *err);
voice_cmd_music_t* vr_get_prev_song(VR_PLAYLIST_TASK_STATE *err);
voice_cmd_music_t* vr_get_curr_song(VR_PLAYLIST_TASK_STATE *err);
VR_PLAYLIST_TASK_STATE vr_song_list_clear();

/*
  * void vr_prefetch_next_song_details();
  * Tell the playlist to prefetch the details of the next song.
  * Calling this may reduce the time taken by vr_get_next_song()
  * Should be called when playback of the current song has been started.
  */
void vr_prefetch_next_song_details();


#ifdef  __cplusplus
}
#endif
#endif
