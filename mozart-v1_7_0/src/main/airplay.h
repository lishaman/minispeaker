#ifndef _AIRPLAY_H
#define _AIRPLAY_H

/**
 * @brief mozart_airplay_init Airplay initialization function
 *
 * @return The successful return 0
 */
extern int mozart_airplay_init(void);

/**
 * @brief mozart_airplay_service_start Start shairport service,such as mDNS、RTSP
 */
extern void mozart_airplay_start_service(void);

/**
 * @brief mozart_airplay_play_stop Stop the shairport music player (call switching DLNA playback)
 */
extern int mozart_airplay_stop_playback(void);

/**
 * @brief mozart_airplay_shutdown Close the exit shairport (call switching network)
 */
extern void mozart_airplay_shutdown(void);

#endif /* _AIRPLAY_H */
