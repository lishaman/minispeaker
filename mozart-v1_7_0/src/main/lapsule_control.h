#ifndef __LAPSULE_CONTROL_H__
#define __LAPSULE_CONTROL_H__

#define LAPSULE_PROVIDER "lapsule"

extern int net_flag;
extern int start_lapsule_app(void);
extern void stop_lapsule_app(void);
extern int lapsule_do_wakeup(void);
extern int lapsule_do_prev_song(void);
extern int lapsule_do_next_song(void);
extern int lapsule_do_toggle(void);
extern int lapsule_do_stop_play(void);
extern int lapsule_do_set_vol(int vol);
extern int lapsule_do_play(void);
extern int lapsule_do_pause(void);
extern int lapsule_do_search_song(char *song);
extern int lapsule_do_music_list(int keyIndex);
extern int lapsule_do_linein_on(void);
extern int lapsule_do_linein_off(void);

#endif /* __LAPSULE_CONTROL_H__ */
