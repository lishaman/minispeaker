#ifndef WEBRTC_AEC_H_
#define WEBRTC_AEC_H_

#ifdef __cplusplus
extern "C" {
#endif
unsigned int webrtc_aec_enable(void);
unsigned int webrtc_aec_get_buffer_length(void);
void *webrtc_aec_calculate(void *buf_record, void *buf_play, void *buf_result, unsigned int size);
unsigned int ingenic_apm_init();
unsigned int ingenic_apm_destroy();
#ifdef __cplusplus
}
#endif
#endif

