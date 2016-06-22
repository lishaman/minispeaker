
obj-$(CONFIG_SOUND_JZ_DMIC_V12) += fate.o

$(obj)/fate.o:$(obj)/lib.a

$(obj)/lib.a:$(obj)/libivw3_6.hex
	cp $(obj)/libivw3_6.hex $(obj)/lib.a
