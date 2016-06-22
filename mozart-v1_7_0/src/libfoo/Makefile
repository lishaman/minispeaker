CC = mipsel-linux-gcc
CXX= mipsel-linux-g++
STRIP = mipsel-linux-strip

CFLAGS= -fPIC -Wall -O2 -I. -I$(MOLIB_DIR)/usr/include
LDFLAGS= -shared -lpthread -L$(MOLIB_DIR)/usr/lib

DESTDIR =
PREFIX = /usr
BINDIR = $(PREFIX)/bin
SBINDIR = $(PREFIX)/sbin
LIBDIR = $(PREFIX)/lib
INCDIR = $(PREFIX)/include

INSTALL    = install

SRCS = foo.c
OBJS = $(patsubst %.c, %.c.o, $(SRCS))
TARGET=libfoo.so

all:$(TARGET)

libfoo.so: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.c.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:$(TARGET)
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -d $(DESTDIR)$(INCDIR)
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) $(TARGET) $(DESTDIR)$(LIBDIR)/
	$(INSTALL) -m 664  foo.h $(DESTDIR)$(INCDIR)

clean:
	-rm -f $(OBJS) $(TARGET)

uninstall:
	-rm -f $(DESTDIR)$(LIBDIR)/libfoo.so
	-rm -f $(DESTDIR)$(INCDIR)/foo.h

.PHONY:all clean install uninstall $(TARGET)
