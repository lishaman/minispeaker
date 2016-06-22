CC = gcc
CFLAGS = -m32 -fPIC -Wall -I. -I$(MOLIB_UPDATER_DIR)/usr/include
LDFLAGS = -m32 -lpthread -L$(MOLIB_UPDATER_DIR)/usr/lib -lini_x86

DESTDIR =
PREFIX = /usr
LIBDIR = $(PREFIX)/lib
BINDIR = $(PREFIX)/bin
INCDIR = $(PREFIX)/include
TESTDIR = $(PREFIX)/test

INSTALL    = install

SRCS = inirw.c
OBJS = $(patsubst %.c, %.c.o, $(SRCS))

all:inirw

%.c.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

inirw: $(OBJS)
	$(CC) $? $(LDFLAGS) -o inirw

install:$(TARGETS)
	$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 755 inirw $(DESTDIR)$(BINDIR)

clean:
	-rm -rf inirw *.o

distclean: clean

uninstall:
	rm -rf $(DESTDIR)$(BINDIR)/inirw

.PHONY:all clean inirw install uninstall inirw
