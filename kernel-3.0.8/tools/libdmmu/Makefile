CC=mipsel-linux-gcc
WARNINGS = -Wall -Wextra
CFLAGS = $(WARNINGS) -g -shared -fPIC
target = libdmmu.so
objects = dmmu.o

$(target):$(objects)
	$(CC) $(CFLAGS) -o $@ $^
	@rm $(objects)
	@echo "generate $@"

clean :
	@rm $(target)
