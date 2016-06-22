CC = mipsel-linux-gcc
CXX = mipsel-linux-g++
STRIP = mipsel-linux-strip
INSTALL = install
PREFIX = /usr
LIBDIR = $(PREFIX)/lib

CFLAGS = -Wall -O2 -fPIC -I. -I$(MOLIB_DIR)/usr/include -I$(MOZART_DIR)/usr/include
LIBLDFLAGS = -shared

SRCS = fake_spimanager.c

OBJS = $(patsubst %.c, %.c.o, $(SRCS))
TARGET = libfake_spimanager.so

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBLDFLAGS)

%.c.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) $(TARGET) $(DESTDIR)$(LIBDIR)/

clean:
	-rm -rf $(OBJS) $(TARGET)

uninstall:
	-rm -rf $(DESTDIR)$(LIBDIR)/$(TARGET)

.PHONY:all clean install uninstall $(TARGET)
