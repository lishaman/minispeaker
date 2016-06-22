#include "alsa/asoundlib.h"

const char *snd_asoundlib_version(void)
{return NULL;}
int snd_lib_error_set_handler(snd_lib_error_handler_t handler)
{return 0;}
int snd_mixer_attach(snd_mixer_t *mixer, const char *name)
{return 0;}
int snd_mixer_close(snd_mixer_t *mixer)
{return 0;}
snd_mixer_elem_t *snd_mixer_find_selem(snd_mixer_t *mixer, const snd_mixer_selem_id_t *id)
{return NULL;}
int snd_mixer_load(snd_mixer_t *mixer)
{return 0;}
int snd_mixer_open(snd_mixer_t **mixer, int mode)
{return 0;}
int snd_mixer_selem_get_playback_volume(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value)
{return 0;}
int snd_mixer_selem_get_playback_volume_range(snd_mixer_elem_t *elem, long *min, long *max)
{return 0;}
int snd_mixer_selem_has_playback_switch(snd_mixer_elem_t *elem)
{return 0;}
int snd_mixer_selem_has_playback_switch_joined(snd_mixer_elem_t *elem)
{return 0;}
unsigned int snd_mixer_selem_id_get_index(const snd_mixer_selem_id_t *obj)
{return 0;}
const char *snd_mixer_selem_id_get_name(const snd_mixer_selem_id_t *obj)
{return 0;}
void snd_mixer_selem_id_set_index(snd_mixer_selem_id_t *obj, unsigned int val)
{return;}
void snd_mixer_selem_id_set_name(snd_mixer_selem_id_t *obj, const char *val)
{return;}
size_t snd_mixer_selem_id_sizeof(void)
{return 0;}
int snd_mixer_selem_register(snd_mixer_t *mixer, struct snd_mixer_selem_regopt *options, snd_mixer_class_t **classp)
{return 0;}
int snd_mixer_selem_set_playback_switch(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, int value)
{return 0;}
int snd_mixer_selem_set_playback_volume(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long value)
{return 0;}
int snd_pcm_close(snd_pcm_t *pcm)
{return 0;}
int snd_pcm_delay(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp)
{return 0;}
int snd_pcm_drain(snd_pcm_t *pcm)
{return 0;}
int snd_pcm_drop(snd_pcm_t *pcm)
{return 0;}
const char *snd_pcm_format_description(const snd_pcm_format_t format)
{return NULL;}
snd_pcm_sframes_t snd_pcm_forward(snd_pcm_t *pcm, snd_pcm_uframes_t frames)
{return 0;}
int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{return 0;}
int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{return 0;}
int snd_pcm_hw_params_can_pause(const snd_pcm_hw_params_t *params)
{return 0;}
int snd_pcm_hw_params_get_buffer_size(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val)
{return 0;}
int snd_pcm_hw_params_get_period_size(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir)
{return 0;}
int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
{return 0;}
int snd_pcm_hw_params_set_buffer_time_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{return 0;}
int snd_pcm_hw_params_set_channels_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val)
{return 0;}
int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{return 0;}
int snd_pcm_hw_params_set_periods_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{return 0;}
int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{return 0;}
int snd_pcm_hw_params_set_rate_resample(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{return 0;}
size_t snd_pcm_hw_params_sizeof(void)
{return 0;}
int snd_pcm_hw_params_test_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{return 0;}
int snd_pcm_nonblock(snd_pcm_t *pcm, int nonblock)
{return 0;}
int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
{return 0;}
int snd_pcm_pause(snd_pcm_t *pcm, int enable)
{return 0;}
int snd_pcm_prepare(snd_pcm_t *pcm)
{return 0;}
int snd_pcm_resume(snd_pcm_t *pcm)
{return 0;}
snd_pcm_state_t snd_pcm_state(snd_pcm_t *pcm)
{return 0;}
int snd_pcm_status(snd_pcm_t *pcm, snd_pcm_status_t *status)
{return 0;}
snd_pcm_uframes_t snd_pcm_status_get_avail(const snd_pcm_status_t *obj)
{return 0;}
size_t snd_pcm_status_sizeof(void)
{return 0;}
int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{return 0;}
int snd_pcm_sw_params_current(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{return 0;}
int snd_pcm_sw_params_get_boundary(const snd_pcm_sw_params_t *params, snd_pcm_uframes_t *val)
{return 0;}
int snd_pcm_sw_params_set_silence_size(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val)
{return 0;}
int snd_pcm_sw_params_set_start_threshold(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val)
{return 0;}
int snd_pcm_sw_params_set_stop_threshold(snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val)
{return 0;}
size_t snd_pcm_sw_params_sizeof(void)
{return 0;}
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{return 0;}
const char *snd_strerror(int errnum)
{return NULL;}
int snd_pcm_format_physical_width(snd_pcm_format_t format)
{return 0;}
int snd_pcm_hw_params_get_buffer_time_max(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{return 0;}
int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{return 0;}
int snd_pcm_hw_params_set_period_time_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{return 0;}
snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{return 0;}
int snd_pcm_wait(snd_pcm_t *pcm, int timeout)
{return 0;}
int snd_mixer_selem_set_playback_volume_range(snd_mixer_elem_t *elem, long min, long max)
{return 0;}
int snd_mixer_handle_events(snd_mixer_t *mixer)
{return 0;}
int snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode)
{return 0;}
int snd_ctl_close(snd_ctl_t *ctl)
{return 0;}
int snd_ctl_elem_value_malloc(snd_ctl_elem_value_t **ptr)
{return 0;}
void snd_ctl_elem_value_set_integer(snd_ctl_elem_value_t *obj, unsigned int idx, long val)
{}
void snd_ctl_elem_value_set_interface(snd_ctl_elem_value_t *obj, snd_ctl_elem_iface_t val)
{}
void snd_ctl_elem_value_set_name(snd_ctl_elem_value_t *obj, const char *val)
{}
void snd_ctl_elem_value_set_numid(snd_ctl_elem_value_t *obj, unsigned int val)
{}
int snd_ctl_elem_write(snd_ctl_t *ctl, snd_ctl_elem_value_t *value)
{return 0;}
