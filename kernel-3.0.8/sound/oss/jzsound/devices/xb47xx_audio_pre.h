
#if 1
#define AUDIO_WRITE(n) (*(volatile unsigned int*)(n))
#define audio_write(val,addr) AUDIO_WRITE(addr) = (val)
#define I2SCDR1_PRE 	0xb0000070
#define I2SDIV_PRE	0xb0020030
#define PCMCDR_PRE 	0xb0000084
#define PCMCDR1_PRE 	0xb00000E0
#define PCMDIV_PRE 	0xb0071014
#endif
