SUBDIR=www cgi

all install clean uninstall: $(SUBDIR)
	$(foreach subdir,$(SUBDIR),make -C $(subdir) $@;)

.PHONY:all clean install uninstall $(SUBDIR)
