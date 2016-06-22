CROSS   = mipsel-linux-
CC      = $(CROSS)gcc

all: rtc-test rtc-poweron-test rtc-wakeup-test

rtc-test : rtc-test.c
	$(CC) -o $@ rtc-test.c

rtc-poweron-test : rtc-poweron-test.c
	$(CC) -o $@ rtc-poweron-test.c

rtc-wakeup-test : rtc-wakeup-test.c
	$(CC) -o $@ rtc-wakeup-test.c

clean:
	rm rtc-test rtc-poweron-test rtc-wakeup-test
