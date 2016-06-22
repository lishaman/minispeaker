LCD ?= 0
CC = mipsel-linux-gcc
STRIP = mipsel-linux-strip

CFLAGS = -g -Wall -I$(MOLIB_UPDATER_DIR)/usr/include
CFLAGS += -I$(MOLIB_APP_DIR)/usr/include
CFLAGS += -I$(MOLIB_APP_DIR)/usr/include/upnp
CFLAGS += -I$(MOLIB_UPDATER_DIR)/usr/include/json-c/
CFLAGS += -I$(MOZART_APP_DIR)/usr/include
CFLAGS += -I$(MOZART_UPDATER_DIR)/usr/include
CFLAGS += -rdynamic -fasynchronous-unwind-tables

LDFLAGS = -L$(MOLIB_UPDATER_DIR)/usr/lib
LDFLAGS += -L$(MOLIB_APP_DIR)/usr/lib
LDFLAGS += -L$(MOZART_APP_DIR)/usr/lib
LDFLAGS += -L$(MOZART_UPDATER_DIR)/usr/lib
LDFLAGS += -Wl,-rpath-link,$(MOLIB_DIR)/usr/lib:$(MOLIB_UPDATER_DIR)/usr/lib:$(MOZART_DIR)/usr/lib

DESTDIR = $(MOZART_APP_DIR)
DESTDIR1 = $(MOZART_UPDATER_DIR)
PREFIX = /usr
LIBDIR = $(PREFIX)/lib
BINDIR = $(PREFIX)/bin
TESTDIR = $(PREFIX)/test
INCDIR = $(PREFIX)/include

PKG_CONFIG = pkg-config
INSTALL    = install

# test program for libsharememory.
TARGETS = getshm setshm
# test program for libini.
TARGETS += inirw
# test program for network_manager.
TARGETS += get_mode switch_ap switch_sta switch_airkiss get_wifi_list stop_wifi startup_ether stop_ether
# test program for volume
TARGETS += volume
# test program for libtips
TARGETS += playtone
TARGETS += playkey
TARGETS += welcome
# test program for libota
TARGETS += otact
TARGETS += ota_state
TARGETS += ota_test
TARGETS += ota_test_nv
TARGETS += ota_test_version
# test program for libpower
TARGETS += power
# test program for libplayer.
TARGETS += player_test
# test program for librender.
TARGETS += render_test
# test program for librecord.
TARGETS += record_test
# test program for event_manager.
TARGETS += event_test_recv event_test_send

all:$(TARGETS)

getshm:libsharememory/getshm.o
	$(CC) $? -o $@ $(LDFLAGS) -lsharememory
setshm:libsharememory/setshm.o
	$(CC) $? -o $@ $(LDFLAGS) -lsharememory

inirw:inirw.o
	$(CC) $? -o $@ $(LDFLAGS) -lini
inirw.o:inirw.c
	$(CC) $< -c -o $@ $(CFLAGS)
inirw.c:
	ln -s $(TOPDIR)/tools/host-tools/inirw/inirw.c $(CURDIR)/inirw.c

get_mode:network_manager/get_mode.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
switch_airkiss:network_manager/switch_airkiss.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
switch_ap:network_manager/switch_ap.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
switch_sta:network_manager/switch_sta.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
get_wifi_list:network_manager/get_wifi_list.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
stop_wifi:network_manager/stop_wifi.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
startup_ether:network_manager/startup_ether.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c
stop_ether:network_manager/stop_ether.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lutils -ltips -lplayer -lwifi -ljson-c

volume:libvolume/volume.o
	$(CC) $? -o $@ $(LDFLAGS) -lini -lvolume -lasound

playtone:libtips/play_tone.o
	$(CC) $? -o $@ $(LDFLAGS) -ltips -lz -lplayer -lini -lutils

playkey:libtips/play_key.o
	$(CC) $? -o $@ $(LDFLAGS) -ltips -lz -lplayer -lini -lutils

ifeq ($(LCD),1)
welcome:libtips/welcome.o
	$(CC) $? -o $@ $(LDFLAGS) -ltips -lz -lplayer -lini -llcd -llcdshow -lpng -lz -lutils
else
welcome:libtips/welcome.o
	$(CC) $? -o $@ $(LDFLAGS) -ltips -lz -lplayer -lini -lutils
endif
otact:libota/ota_active.o
	$(CC) $? -o $@ $(LDFLAGS) -lota -lini

ota_state:libota/ota_state.o
	$(CC) $? -o $@ $(LDFLAGS) -lota

ota_test:libota/ota_test.o
	$(CC) $? -o $@ $(LDFLAGS) -lota

ota_test_nv:libota/test_nv.o
	$(CC) $? -o $@ $(LDFLAGS) -lota

ota_test_version:libota/test_version.o
	$(CC) $? -o $@ $(LDFLAGS) -lota

power:libpower/power.o
	$(CC) $? -o $@ $(LDFLAGS) -lpower

player_test:player/test.o
	$(CC) $? -o $@ $(LDFLAGS) -lplayer -lini -lutils

render_test:librender/test.o
	$(CC) $? -o $@ $(LDFLAGS) -lrender -lplayer -lini -lutils -llinklist -ltips -lsharememory -lasound -lpthread -lupnp -lixml -lvolume -lthreadutil -levent

record_test:librecord/test.o
	$(CC) $? -o $@ $(LDFLAGS) -lasound -lini -lrecord

event_test_recv:event_manager/test_recv.o
	$(CC) $? -o $@ $(LDFLAGS) -levent

event_test_send:event_manager/test_send.o
	$(CC) $? -o $@ $(LDFLAGS) -levent

install:$(TARGETS)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -d $(DESTDIR1)$(BINDIR)
	$(INSTALL) -d $(DESTDIR)$(TESTDIR)
	$(INSTALL) -m 755 getshm $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 setshm $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 inirw $(DESTDIR1)$(BINDIR)/
	$(INSTALL) -m 755 volume $(DESTDIR1)$(BINDIR)/
	$(INSTALL) -m 755 welcome $(DESTDIR1)$(BINDIR)/
	$(INSTALL) -m 755 switch_sta $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 switch_ap $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 switch_airkiss $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 get_mode $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 get_wifi_list $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 stop_wifi $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 startup_ether $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 stop_ether $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 playtone $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 playkey $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 otact $(DESTDIR1)$(BINDIR)/
	$(INSTALL) -m 755 ota_state $(DESTDIR1)$(BINDIR)/
	$(INSTALL) -m 755 ota_test $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 ota_test_nv $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 ota_test_version $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 power $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 player_test $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 render_test $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 record_test $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 event_test_send $(DESTDIR)$(TESTDIR)/
	$(INSTALL) -m 755 event_test_recv $(DESTDIR)$(TESTDIR)/

clean distclean:
	-rm -rf inirw.c *.o */*.o $(TARGETS)

uninstall:
	rm -rf $(DESTDIR1)/$(BINDIR)/{inirw,volume,ota_state,welcome}
	rm -rf $(DESTDIR)/$(TESTDIR)/{playtone,playtone}
	rm -rf $(DESTDIR)/$(TESTDIR)/{getshm,setshm,switch_airkiss,switch_ap,switch_sta,get_mode,get_wifi_list,stop_wifi,startup_ether,stop_ether}
	rm -rf $(DESTDIR)/$(TESTDIR)/{power,player_test,otact,ota_test,ota_test_nv,ota_test_version,render_test,record_test}
	rm -rf $(DESTDIR)/$(TESTDIR)/{event_test_recv,event_test_send}

.PHONY:all clean install uninstall $(TARGETS)
