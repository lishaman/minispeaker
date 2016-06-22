CC = mipsel-linux-gcc
STRIP = mipsel-linux-strip
INSTALL = install
DESTDIR =
PREFIX = /usr
SBINDIR = $(PREFIX)/sbin

CFLAGS = -Wall -O2 -I. -I$(MOLIB_DIR)/usr/include -I$(MOZART_DIR)/usr/include
CFLAGS += -I$(MOLIB_DIR)/usr/include/upnp

LDFLAGS = -lpthread -L$(MOLIB_DIR)/usr/lib -L$(MOZART_DIR)/usr/lib
LDFLAGS += -levent -lupdater -ltips
LDFLAGS += -lssl -lcrypto -lsharememory
LDFLAGS += -llinklist -lini

SRCS = updater.c broadcast.c

OBJS = $(patsubst %.c, %.c.o, $(SRCS))
TARGET = updater

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.c.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) $(TARGET) $(DESTDIR)$(SBINDIR)/

clean:
	-rm -rf $(OBJS) $(TARGET)

uninstall:
	-rm -rf $(DESTDIR)$(SBINDIR)/$(TARGET)

.PHONY:all clean install uninstall $(TARGET)
