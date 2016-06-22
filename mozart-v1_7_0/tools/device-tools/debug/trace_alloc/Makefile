CC = mipsel-linux-gcc
STRIP = mipsel-linux-strip

PREFIX = /usr
LIBDIR = $(PREFIX)/lib
DEBUGDIR = $(PREFIX)/debug

INSTALL = install
TARGETS = libtrace_alloc.mips.so
#TARGETS += libtrace_alloc.mips.test.so trace_alloc_test.mips

all:$(TARGETS)

# LD_PRELOAD="/usr/fs/usr/debug/libtrace_alloc.mips.so" mozart -sb
libtrace_alloc.mips.so: trace_alloc.c
	$(CC) $^ -o $@ -shared -Wall -ldl -fPIC -lpthread -O2 -DMIPS -DLOG_FILE

# mips
# LD_LIBRARY_PATH="/usr/fs/usr/debug/" /usr/fs/usr/debug/trace_alloc_test.mips
libtrace_alloc.mips.test.so: trace_alloc.c
	$(CC) $^ -o $@ -shared -Wall -ldl -fPIC -O2 -DMIPS -DLOG_FILE -DTEST
trace_alloc_test.mips: main.c libtrace_alloc.mips.test.so
	$(CC) $< -o $@ -O2 -L. -Wall -ltrace_alloc.mips.test -lpthread -DMIPS  -DTEST

install:$(TARGETS)
	$(INSTALL) -d $(DESTDIR)$(DEBUGDIR)
	$(INSTALL) -m 755 -s --strip-program=$(STRIP) $(TARGETS) $(DESTDIR)$(DEBUGDIR)/

clean:
	-rm -rf libtrace_alloc.mips.so libtrace_alloc.mips.test.so trace_alloc_test.mips

uninstall:
	-rm -rf $(DESTDIR)$(DEBUGDIR)/libtrace_alloc.mips.so
	-rm -rf $(DESTDIR)$(DEBUGDIR)/libtrace_alloc.mips.test.so
	-rm -rf $(DESTDIR)$(DEBUGDIR)/trace_alloc_test.mips

distclean: clean uninstall

.PHONY:all install clean uninstall libtrace_alloc.mips.so trace_alloc_test.mips

# x86
x86:
	@rm -f trace_alloc_test_log
	gcc -o libtrace_alloc.so -shared trace_alloc.c -Wall -ldl -fPIC -O2 -m32 -DLOG_FILE -DTEST
	gcc -o trace_alloc_test.x86 main.c -O2 -L. -m32 -Wall -ltrace_alloc -lpthread -DTEST
	LD_LIBRARY_PATH="." ./trace_alloc_test.x86
	@ls trace_alloc_test_log > /dev/null
	gcc -o libtrace_alloc.so -shared trace_alloc.c -Wall -ldl -fPIC -O2 -m32 -DTEST
	LD_LIBRARY_PATH="." ./trace_alloc_test.x86
