WARNINGS=-W -Wall -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return \
	 -Wcast-align -Wcast-qual -Wnested-externs -Wshadow -Wbad-function-cast \
	 -Wwrite-strings

CFLAGS=-O3 -I.. -I../tools $(WARNINGS)
CFLAGS+=-ffast-math -fomit-frame-pointer -static -pthread
#CFLAGS+=-funroll-loops
#CFLAGS+=-march=prescott
#CFLAGS+= -mtune=native
# TIP: try adding -openmp or -fopenmp  to enable OPENMP directives and use of multiple cores
# #CFLAGS+=-fopenmp
# CFLAGS+= $(CFLAGADD)

ifeq "$(DATATYPE)" ""
	DATATYPE=double
endif

ifeq  "$(DATATYPE)" "int16_t"
	TYPEFLAGS=-DFIXED_POINT=16
endif

ifeq  "$(DATATYPE)" "int32_t"
	TYPEFLAGS=-DFIXED_POINT=32
endif

TYPEFLAGS=-Dkiss_fft_scalar=$(DATATYPE)

SRCFILES=sndkit.c wav.c

all: clean sndkit

sndkit: $(SRCFILES)
	mipsel-linux-gcc -o $@ $(CFLAGS) $(TYPEFLAGS) -DREAL_FASTFIR -lm -lpthread $+ -DFAST_FILT_UTIL


#	rm -rf sndkit
#	mipsel-linux-gcc -Wall -o sndkit sndkit.c wav.c -lpthread

clean:
	rm -rf sndkit
