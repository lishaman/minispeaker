DESTDIR ?=
PREFIX = /usr
WWW_BIN = /var/www/

INSTALL = install
PWD_DIR = $(shell pwd)

all:

install:
	$(INSTALL) -d $(DESTDIR)$(WWW_BIN)

	cp $(PWD_DIR)/*.html $(DESTDIR)$(WWW_BIN)/

uninstall:
	-rm -f $(DESTDIR)$(WWW_BIN)/*.html
	-rm -rf $(DESTDIR)$(WWW_BIN)/

.PHONY:all clean install uninstall
