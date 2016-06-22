/* include/linux/em7180.h */
#ifndef LINUX_EM7180_MODULE_H
#define LINUX_EM7180_MODULE_H

//#define EM7180_DEBUG

/* Register for start */
#define REG_ALGORITHM_CONTROL	0x54
#define ALGORITHM_CONTROL_VALUE 0x02
#define REG_HOST_CONTROL	0x34
#define HOST_CONTROL_VALUE	0x01

/* Register for set parameters */
#define REG_ACCEL_RATE		0x56
#define REG_CURR_ACCEL		0x46
#define REG_RANGE		0x55
#define REG_CURR_RANGE		0x45
#define REG_THRESHOLD_VALUE	0x60
#define REG_CURR_THRESHOLD	0x4F

/* Register for read data */
#define REG_LEN_LOW		0x4B
#define REG_LEN_HIGH		0x4C
#define REG_MODE_REQ		0x5B
#define REG_CURR_MODE		0x4D
#define REG_BYTEREQ_LO		0x5C
#define REG_BYTEREQ_HI		0x5D
#define REG_BYTEREQRESP_LO	0x3A
#define REG_BYTEREQRESP_HI	0x3B
#define REG_BATCHBASE0		0x3C
#define REG_RESET_DATA		0x5F
#define REG_CURR_RESET		0x4E
#define REG_RESET		0x9B

/*  COPY_BUF_SIZE must be larger than the em7180 buffer  */
#define DATA_BUF_SIZE		(16 * 1024 * 1024)
#define COPY_BUF_SIZE		(50 * 1024)
#define BATCHBLOCKSIZE		8
#define BATCH_MODE_VALUE	1
#define UPDATE_MODE_VALUE	0
#define RESET_DATA_VALUE	1
#define PREP_RESET_VALUE	0
#define RESET_VALUE		1

#define THRESHOLD_CONSTANT	195

#define EM7180_NAME	"em7180"

#ifdef EM7180_DEBUG

#ifdef dbg
#undef dbg
#endif

#define dbg(fmt, args...) \
	printk("%s %d: " fmt "\n", __func__, __LINE__, ##args)
#else
#define dbg(fmt, args...)
#endif

extern int pni_sentral_download_firmware_to_ram(struct i2c_client *client);

struct accel_info_t {
	int threshold;
	int frequency;
	int ranges;
};

struct em7180_platform_data {
	int wakeup;
};

#endif/* LINUX_EM7180_MODULE_H */
