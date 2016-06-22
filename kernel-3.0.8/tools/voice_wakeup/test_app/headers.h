#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#undef sndkit_dbg
extern int debug_level;
#define sndkit_dbg(x...)			\
	({					\
		if (debug_level > 0)		\
			fprintf(stdout, x);	\
	})
