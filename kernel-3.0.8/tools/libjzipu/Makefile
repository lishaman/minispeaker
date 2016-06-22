CC=mipsel-linux-gcc
#CC=mipsel-linux-android-gcc
CCFLAGS += -Wall -g
target = iputest
objects = ipu_test.o saveBmp.o android_jz_ipu.o dmmu.o

$(target):$(objects)
	@$(CC) $(CCFLAGS) -o $@ $^
	@rm $(objects)
	@echo "generate $@"

ipu_test.o:ipu_test/ipu_test.c ipu_test/saveBmp.h android_jz_ipu.h ../libdmmu/dmmu.h
	@$(CC) $(CCFLAGS) -c $<
saveBmp.o:ipu_test/saveBmp.c ipu_test/saveBmp.h
	@$(CC) $(CCFLAGS) -c $<
dmmu.o:../libdmmu/dmmu.c ../libdmmu/dmmu.h
	@$(CC) $(CCFLAGS) -c $<
android_jz_ipu.o:android_jz_ipu.c android_jz_ipu.h Log.h
	@$(CC) $(CCFLAGS) -c $<

.PHONY : clean
clean:
	rm $(target)
