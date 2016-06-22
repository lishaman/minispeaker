#ifndef __DSD2PCM_H_INCLUDED__
#define __DSD2PCM_H_INCLUDED__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <memory.h>
#include <malloc.h>
#include <assert.h>

#define DSDxFs1   (44100 * 1)
#define DSDxFs2   (44100 * 2)
#define DSDxFs4   (44100 * 4)
#define DSDxFs8   (44100 * 8)
#define DSDxFs64  (44100 * 64)
#define DSDxFs128 (44100 * 128)
#define DSDxFs256 (44100 * 256)
#define DSDxFs512 (44100 * 512)

#define	DSD64_44100		1
#define	DSD64_88200		2
#define	DSD64_176400	4
#define	DSD64_352800	8
#define	DSD128_44100	16
#define	DSD128_88200	64
#define	DSD128_176400	128
#define	DSD128_352800	256
#define	DSD256_44100	512
#define	DSD256_88200	1024
#define	DSD256_176400	2048
#define	DSD256_352800	4096
#define	DSD512_44100	8192
#define	DSD512_88200	16384
#define	DSD512_176400	32768
#define	DSD512_352800	65536

#define DSDFIR64LEN 		641//256//641
#define DSD_SILENCE_BYTE 		0x69//0x03//0x69
#define DSDPCM_MAX_CHANNELS 	6

#define CTABLES(fir_length) 	((fir_length + 7) / 8)
#define DSDFIR_LENGTH			CTABLES(DSDFIR64LEN)
#define DSDFIR_LENGTH_MOD		(DSDFIR_LENGTH - 1)

#define ASSERT(exp)	assert(exp)

#define FREE(p)	\
		if(p)	\
		{	\
			free(p);	\
			p = NULL;	\
		};

#define MALLOC(pHeap, size)	\
		pHeap = malloc(size);	\
		assert(pHeap);

#define MEMSET(pHeap, iValue, size)	memset(pHeap, iValue, size)

typedef enum { UNKNOWN=0, DSDIFF, DSF } dsd_type;
#define DSDPCM_MAX_CHANNELS 6
typedef struct {
	unsigned  sz;
	unsigned  chs;
	unsigned  dcm;
	unsigned  fi;

	uint8_t   *fir_buffer[DSDPCM_MAX_CHANNELS];
	int32_t   *fc[DSDFIR_LENGTH];
}dsdpcm_fir_d;

dsdpcm_fir_d *df;
extern void dsd2pcm_preinit(int ch);
extern int dsd2pcm_init(int);
extern void dsd2pcm_uninit(void);
extern unsigned dsd2pcm_fir_run(dsdpcm_fir_d *df, uint8_t *in, int32_t *out, const unsigned dsd_samples);

#endif	/*__DSD2PCM_H_INCLUDED__*/

