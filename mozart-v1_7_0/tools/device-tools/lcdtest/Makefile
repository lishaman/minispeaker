CC = mipsel-linux-gcc
STRIP = mipsel-linux-strip

CFLAGS = -Wall -I$(MOLIB_UPDATER_DIR)/usr/include
CFLAGS += -I$(MOLIB_APP_DIR)/usr/include
CFLAGS += -I$(MOZART_APP_DIR)/usr/include
CFLAGS += -I$(MOZART_UPDATER_DIR)/usr/include

LDFLAGS = -L$(MOLIB_UPDATER_DIR)/usr/lib
LDFLAGS += -L$(MOLIB_APP_DIR)/usr/lib
LDFLAGS += -L$(MOZART_APP_DIR)/usr/lib
LDFLAGS += -L$(MOZART_UPDATER_DIR)/usr/lib -lutils -lini

PREFIX = /usr
LIBDIR = $(PREFIX)/lib
BINDIR = $(PREFIX)/bin
TESTDIR = $(PREFIX)/test
INCDIR = $(PREFIX)/include
INITDIR = /etc/init.d/

PKG_CONFIG = pkg-config
INSTALL    = install

all:lcd_test

lcd_test:lcd_test.o
	$(CC) $? -o $@ $(LDFLAGS)

install:$(TARGETS)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
#	$(INSTALL) -m 755 lcd_test $(DESTDIR)$(BINDIR)/
#	$(INSTALL) -m 755 S99lcd.sh $(DESTDIR)$(INITDIR)

clean distclean:
	-rm -rf *.o lcd_test

uninstall:
#	rm -f $(DESTDIR)$(BINDIR)/lcd_test
#	rm -f $(DESTDIR)$(INITDIR)/S99lcd.sh

.PHONY:all clean install uninstall $(TARGETS)
