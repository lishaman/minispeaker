LCD ?= 0
CC = mipsel-linux-gcc
STRIP = mipsel-linux-strip
INSTALL = install
DESTDIR =
PREFIX = /usr
SBINDIR = $(PREFIX)/sbin
INITDIR = /etc/init.d/

CFLAGS = -Wall -O2 -I. -I$(MOLIB_DIR)/usr/include -I$(MOZART_DIR)/usr/include
CFLAGS += -I$(MOLIB_DIR)/usr/include/upnp
CFLAGS += -I$(MOLIB_UPDATER_DIR)/usr/include

LDFLAGS = -lpthread -L$(MOLIB_DIR)/usr/lib -L$(MOLIB_UPDATER_DIR)/usr/lib -L$(MOZART_DIR)/usr/lib
LDFLAGS += -Wl,-rpath-link,$(MOLIB_DIR)/usr/lib:$(MOLIB_UPDATER_DIR)/usr/lib
LDFLAGS += -lwifi -lini -lutils -ltips -lplayer -ljson-c

ifeq ($(LCD),1)
LDFLAGS += -llcd -llcdshow -lpng -lz
else
$(warning lcd func CLOSED!!!!)
endif

all:update wr_flag

update:update.o
	$(CC) -o $@ $^ $(LDFLAGS)

wr_flag:wr_flag.o
	$(CC) -o $@ $^ $(LDFLAGS)

install:
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -d $(DESTDIR)$(INITDIR)
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) update $(DESTDIR)$(SBINDIR)/
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) wr_flag $(DESTDIR)$(SBINDIR)/
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) mtd_debug $(DESTDIR)$(SBINDIR)/
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) flash_erase $(DESTDIR)$(SBINDIR)/
	$(INSTALL) -m 755 S99updater.sh $(DESTDIR)$(INITDIR)/
	$(INSTALL) -m 755 unzip.sh $(DESTDIR)$(SBINDIR)/
	$(INSTALL) -m 755 mkzip.sh $(DESTDIR)$(SBINDIR)/
	$(INSTALL) -m 755 recovery.sh $(DESTDIR)$(SBINDIR)/

clean:
	-rm -rf *.o update wr_flag

uninstall:

.PHONY:all clean install uninstall $(TARGETS)
