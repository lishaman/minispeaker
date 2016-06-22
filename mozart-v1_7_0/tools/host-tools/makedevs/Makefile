CC=gcc
INSTALL=install

all:makedevs

makedevs:makedevs.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

install:
	$(INSTALL) -d $(DESTDIR)/usr/bin
	$(INSTALL) -m 775 makedevs $(DESTDIR)/usr/bin/

clean:
	-rm -f makedevs.o makedevs

distclean: clean

.PHONY:all makedevs clean
