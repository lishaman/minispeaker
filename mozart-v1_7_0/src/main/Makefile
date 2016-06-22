DIR_CUR := $(shell pwd)

# $1: string
# $2: start pos
# $3: length
define substring
$(shell echo `expr substr $(1) $(2) $(3)`)
endef

BT ?= 0
VR ?= 0
LAPSULE ?= 0
DMR ?= 0
WEBRTC ?= 0
DMS ?= 0
AIRPLAY ?= 0
LOCALPLAYER ?= 0


CC = mipsel-linux-gcc
CXX = mipsel-linux-g++
STRIP = mipsel-linux-strip
INSTALL = install
DESTDIR =
PREFIX = /usr
INITDIR = /etc/init.d/
SBINDIR = $(PREFIX)/sbin

SRCS = mozart.c
SRCS += app.c battery_capacity.c key_function.c linein.c ximalaya.c tts.c

# common opts.
CFLAGS = -fPIC -Wall -g -O2 -I. -I$(MOLIB_DIR)/usr/include -I$(MOLIB_UPDATER_DIR)/usr/include -I$(MOZART_DIR)/usr/include
CFLAGS += -rdynamic -fasynchronous-unwind-tables

LDFLAGS = -g -lpthread -lstdc++ -L$(MOLIB_DIR)/usr/lib -L$(MOLIB_UPDATER_DIR)/usr/lib -L$(MOZART_DIR)/usr/lib
LDFLAGS += -Wl,-rpath-link,$(MOLIB_DIR)/usr/lib:$(MOLIB_UPDATER_DIR)/usr/lib:$(MOZART_DIR)/usr/lib

# basic depends.
LDFLAGS += -levent -lpower -lwifi -lvolume -ltips -lsharememory -lini -lutils -lpthread -lplayer -ljson-c -lximalaya -lm -lresample

# alarm
LDFLAGS += -lalarm -lalarm_interface

ifeq ("$(VR)","0")
$(warning vr func are CLOSED!!!!)
else
ifeq ("$(VR)","vr_baidu")
LDFLAGS += -lmovr
SRCS += vr.c
else
ifeq ("$(VR)","vr_speech")
LDFLAGS += -lvr_speech
SRCS += vr.c
else
ifeq ("$(VR)","vr_iflytek")
LDFLAGS += -lvr_iflytek
SRCS += vr.c
else
ifeq ("$(VR)","vr_unisound")
LDFLAGS +=
else
ifeq ("$(VR)","vr_atalk")
LDFLAGS += -lvr_atalk
SRCS += vr.c
else
$(error "$(VR): NOT supported vr!!!!")
endif
endif
endif
endif
endif
endif

ifeq ($(LCD),1)
LDFLAGS += -llcd -llcdshow -lpng -lz
else
$(warning lcd func CLOSED!!!!)
endif

ifeq ($(LAPSULE),1)
SRCS += lapsule_control.c
else
$(warning lapsule func CLOSED!!!!)
endif

ifeq ($(ATALK),1)
LDFLAGS += -lwifi -ltips -lutils -lplayer -lvolume -lsharememory -ljson-c
SRCS += atalk/atalk_control.c
else
$(warning atalk func CLOSED!!!!)
endif

ifeq ($(DMR),1)
CFLAGS += -I$(MOLIB_DIR)/usr/include/upnp
LDFLAGS += -lrender
else
$(warning dmr func CLOSED!!!!)
endif

ifeq ($(WEBRTC),1)
LDFLAGS += -lwebrtc_audio_processing -lwebrtc
else
$(warning webrtc func CLOSED!!!!)
endif

ifeq ($(DMS),1)
SRCS += dms.c
else
$(warning dms func CLOSED!!!!)
endif

ifeq ($(AIRPLAY),1)
SRCS += airplay.c
else
$(warning airplay func CLOSED!!!!)
endif


ifeq ($(LOCALPLAYER),1)
LDFLAGS += -llocalplayer
else
$(warning localplayer func CLOSED!!!!)
endif

ifeq ($(BT),0)
$(warning bt func are CLOSED!!!!)
else
ifeq ("$(call substring,$(BT),1,3)", "rtk") # realtek's bt module.
SRCS += bt.c
else
ifeq ("$(call substring,$(BT),1,3)", "bcm") # broadcom's bt module.
SRCS += bt.c
LDFLAGS += -lbsa -lbt -lappcommon
else
$(error "$(BT): NOT supported bt module!!!!")
endif
endif
endif

OBJS = $(patsubst %.c, %.c.o, $(SRCS))
TARGET = mozart

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.c.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -d $(DESTDIR)$(INITDIR)
	if [ -e $(TARGET) ]; then \
		$(INSTALL) -m 755 -s --strip-program=$(STRIP) $(TARGET) $(DESTDIR)$(SBINDIR)/; \
		$(INSTALL) -m 755 S10mozart.sh $(DESTDIR)$(INITDIR)/; \
	fi

clean:
	-rm -rf $(OBJS) $(TARGET)

uninstall:
	if [ -e $(DESTDIR)$(SBINDIR)/$(TARGET)]; then \
		rm -rf $(DESTDIR)$(SBINDIR)/$(TARGET); \
		rm -rf $(DESTDIR)$(INITDIR)/S10mozart.sh; \
	fi

.PHONY:all clean install uninstall $(TARGET)
