#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stropts.h>
#include <string.h>

#define DRIVER_NAME "/dev/virtual_privilege"

#define SRC_ADDR (0x88000000)
#define DST_ADDR (0x8c000000)

#define MAX_NUM (64 * 1024 *1024)

/*
 * Macros to access the system control coprocessor
 */
#define __read_32bit_c0_register(source, sel)				\
({ int __res;								\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mfc0\t%0, " #source "\n\t"			\
			: "=r" (__res));				\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mfc0\t%0, " #source ", " #sel "\n\t"		\
			".set\tmips0\n\t"				\
			: "=r" (__res));				\
	__res;								\
})

#define __write_32bit_c0_register(register, sel, value)			\
do {									\
	if (sel == 0)							\
		__asm__ __volatile__(					\
			"mtc0\t%z0, " #register "\n\t"			\
			: : "Jr" ((unsigned int)(value)));		\
	else								\
		__asm__ __volatile__(					\
			".set\tmips32\n\t"				\
			"mtc0\t%z0, " #register ", " #sel "\n\t"	\
			".set\tmips0"					\
			: : "Jr" ((unsigned int)(value)));		\
} while (0)

#define get_pmon_csr()		__read_32bit_c0_register($16, 7)
#define set_pmon_csr(val)	__write_32bit_c0_register($16, 7, val)

#define get_pmon_high()		__read_32bit_c0_register($17, 4)
#define set_pmon_high(val)	__write_32bit_c0_register($17, 4, val)
#define get_pmon_lc()		__read_32bit_c0_register($17, 5)
#define set_pmon_lc(val)	__write_32bit_c0_register($17, 5, val)
#define get_pmon_rc()		__read_32bit_c0_register($17, 6)
#define set_pmon_rc(val)	__write_32bit_c0_register($17, 6, val)

#define pmon_clear_cnt() do {			\
		set_pmon_high(0);		\
		set_pmon_lc(0);			\
		set_pmon_rc(0);			\
	} while(0)

#define pmon_start() do {			\
		unsigned int csr;		\
		csr = get_pmon_csr();		\
		csr |= 0x100;			\
		set_pmon_csr(csr);		\
	} while(0)
#define pmon_stop() do {			\
		unsigned int csr;		\
		csr = get_pmon_csr();		\
		csr &= ~0x100;			\
		set_pmon_csr(csr);		\
	} while(0)

#define PMON_EVENT_CYCLE 0
#define PMON_EVENT_CACHE 1
#define PMON_EVENT_INST  2
#define PMON_EVENT_TLB   3

#define pmon_prepare(event) do {		\
		unsigned int csr;		\
		pmon_stop();			\
		pmon_clear_cnt();		\
		csr = get_pmon_csr();		\
		csr &= ~0xf000;			\
		csr |= (event)<<12;		\
		set_pmon_csr(csr);		\
	} while(0)

#define get_sum_rc() ( {			\
		unsigned long long sumrc;	\
		unsigned int high, rc;		\
		rc = get_pmon_rc();		\
		high = get_pmon_high();		\
		sumrc = (high & 0xffff);	\
		sumrc = sumrc << 32;		\
		sumrc += rc;			\
		sumrc;				\
	})

#define get_sum_lc() ({				\
		unsigned long long sumlc;	\
		unsigned int high, lc;		\
		lc = get_pmon_lc();		\
		high = get_pmon_high();		\
		sumlc = ((high >> 16) & 0xffff);	\
		sumlc = sumlc << 32;		\
		sumlc += lc;			\
		sumlc;				\
	})


static void print_one_cylc_handle_data(unsigned int data, unsigned int times)
{
	unsigned long long rc;

	rc = get_sum_rc();

	printf("pmon:high---rc %u\n", get_pmon_high() & 0xffff);
	printf("pmon:low---rc %u\n", get_pmon_rc());
	printf("pmon:sum---rc %llu\n", rc);
}

extern void memset_jz(void * dst,  int value, unsigned int size);
static void test_write(char * src_addr, unsigned int sum_data, unsigned int times)
{
	int i;

	for(i = 0; i < times; i++) {
		memset_jz(src_addr, 0xff, sum_data);
	}
}
extern void *memcpy_jz(void *dest, const void *src, unsigned int n);
static void test_memcpy(char * src_addr, char * dst_addr, unsigned int sum_data, unsigned int times)
{
	int i;

	for(i = 0; i < times; i++) {
		memcpy_jz(dst_addr, src_addr, sum_data);
	}
}
static inline void test_read(char * src_addr, unsigned int sum_data, unsigned int times)
{
	int i, n;
	char *tmp_addr;

	for(i = 0; i < times; i++) {
		tmp_addr = src_addr;
		for(n = 0; n < sum_data / 16; n += 64) {
			__asm__ __volatile__(
				"lw $0, 0(%0)\n\t"
				"lw $0, 4(%0)\n\t"
				"lw $0, 8(%0)\n\t"
				"lw $0, 12(%0)\n\t"
				"lw $0, 16(%0)\n\t"
				"lw $0, 20(%0)\n\t"
				"lw $0, 24(%0)\n\t"
				"lw $0, 28(%0)\n\t"
				"lw $0, 32(%0)\n\t"
				"lw $0, 36(%0)\n\t"
				"lw $0, 40(%0)\n\t"
				"lw $0, 44(%0)\n\t"
				"lw $0, 48(%0)\n\t"
				"lw $0, 52(%0)\n\t"
				"lw $0, 54(%0)\n\t"
				"lw $0, 58(%0)\n\t"
				"lw $0, 62(%0)\n\t"
				"lw $0, 64(%0)\n\t"
				:
				:"r"(tmp_addr)
				);
			tmp_addr += 64;
		}
	}
}

static void test_nouse_tlb(unsigned int times)
{
	char *src_addr, *dst_addr;
	unsigned int sum_data = MAX_NUM;

	src_addr = (char *)SRC_ADDR;
	dst_addr = (char *)DST_ADDR;

	printf("-------------------test write-----------------------\n");
	pmon_clear_cnt();
	pmon_start();
	test_write(src_addr, sum_data, times);
	pmon_stop();
	print_one_cylc_handle_data(sum_data, times);
	printf("-------------------test write end-------------------\n");

	printf("-------------------test memcpy-----------------------\n");
	pmon_clear_cnt();
	pmon_start();
	test_memcpy(src_addr, dst_addr, sum_data, times);
	pmon_stop();
	print_one_cylc_handle_data(sum_data, times);
	printf("-------------------test memcpy end-------------------\n");

	printf("-------------------test read-----------------------\n");
	pmon_clear_cnt();
	pmon_start();
	test_read(src_addr, sum_data, times);
	pmon_stop();
	print_one_cylc_handle_data(sum_data, times);
	printf("-------------------test read end-------------------\n");
}

static void test_use_tlb(unsigned int sum_data, unsigned int times)
{
	char *src_addr, *dst_addr;

	src_addr = (char *)malloc(sum_data);
	dst_addr = (char *)malloc(sum_data);
	if(!src_addr || !dst_addr) {
		printf("malloc fail!!!\n");
		return;
	}

	printf("-------------------test write-----------------------\n");
	pmon_clear_cnt();
	pmon_start();
	test_write(src_addr, sum_data, times);
	pmon_stop();
	print_one_cylc_handle_data(sum_data, times);
	printf("-------------------test write end-------------------\n");

	printf("-------------------test memcpy-----------------------\n");
	pmon_clear_cnt();
	pmon_start();
	test_memcpy(src_addr, dst_addr, sum_data,times);
	pmon_stop();
	print_one_cylc_handle_data(sum_data, times);
	printf("-------------------test memcpy end-------------------\n");

	printf("-------------------test read-----------------------\n");
	pmon_clear_cnt();
	pmon_start();
	test_read(src_addr, sum_data, times);
	pmon_stop();
	print_one_cylc_handle_data(sum_data, times);
	printf("-------------------test read end-------------------\n");

	free(src_addr);
	free(dst_addr);
}

int main(int argc, char * argv[])
{
	int fd;
	char opt, un ='K';
	unsigned int use_tlb = 0;
	unsigned int unit = 1024, times = 100;
	unsigned int sum_data = 4;

	while ((opt = getopt(argc, argv, "c:t:d:u:")) != -1) {
		switch(opt) {
		case 'c':
			if(!strcmp(optarg, "nouse_tlb"))
				use_tlb = 1;
			else
				use_tlb = 0;
			break;
		case 'd':
			sum_data = atoi(optarg);
			break;
		case 't':
			times = atoi(optarg);
			break;
		case 'u':
			un = optarg[0];
			switch(un) {
			case 'k':
			case 'K':
				{
					unit = 1024;
					break;
				}
			case 'm':
			case 'M':
				{
					unit = 1024 * 1024;
					break;
				}
			default:
				printf(" %c no support \n", un);
			}
			break;
		default:
			printf(" %c no support \n", opt);
		}
	}

	fd = open(DRIVER_NAME, O_RDWR);
	if(!fd) {
		printf("open %s error!\n", DRIVER_NAME);
		return -1;
	}

	pmon_prepare(PMON_EVENT_CYCLE);

	if(!use_tlb) {
		printf("USE_TLB:test data sum: %d%c, times: %d\n", sum_data, un, times);
		sum_data *= unit;
		test_use_tlb(sum_data, times);
	} else {
		printf("NOUSE_TLB:test data sum: 64M, times: %d\n", times);
		test_nouse_tlb(times);
	}
	close(fd);
	return 0;
}
