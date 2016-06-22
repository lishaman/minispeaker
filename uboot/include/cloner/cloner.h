#ifndef __CLONER_H__
#define __CLONER_H__
#include <errno.h>
#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include <rtc.h>
#include <part.h>
#include <spi.h>
#include <spi_flash.h>
#include <efuse.h>
#include <ingenic_soft_i2c.h>
#include <ingenic_soft_spi.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/compiler.h>
#include <linux/usb/composite.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <nand.h>
#include <ingenic_nand_mgr/nand_param.h>

#define ARGS_LEN (1024*1024)
#define BURNNER_DEBUG 0


#define SSI_IDX 0

struct spi spi;
#define VR_GET_CPU_INFO		0x00  /*bootrom stage request*/
#define VR_SET_DATA_ADDR	0x01
#define VR_SET_DATA_LEN		0x02
#define VR_FLUSH_CACHE		0x03
#define VR_PROG_STAGE1		0x04
#define VR_PROG_STAGE2		0x05
#define VR_GET_CHIP_ID		0x06
#define VR_GET_USER_ID		0x07
#define VR_GET_ACK		0x10	/*firmware stage request*/
#define VR_INIT			0x11
#define VR_WRITE		0x12
#define VR_READ			0x13
#define VR_UPDATE_CFG		0x14
#define VR_SYNC_TIME		0x15
#define VR_REBOOT		0x16
#define VR_POWEROFF		0x17	/*reboot and poweroff*/
#define VR_CHECK		0x18
#define VR_GET_CRC		0x19

#define MMC_ERASE_ALL	1
#define MMC_ERASE_PART	2
#define MMC_ERASE_CNT_MAX	10

#define SPI_NO_ERASE	0
#define SPI_ERASE_PART	1
#ifndef CONFIG_SF_DEFAULT_SPEED
# define CONFIG_SF_DEFAULT_SPEED    20000000
#endif
#ifndef CONFIG_SF_DEFAULT_MODE
# define CONFIG_SF_DEFAULT_MODE     SPI_MODE_3
#endif
#ifndef CONFIG_SF_DEFAULT_CS
# define CONFIG_SF_DEFAULT_CS       0
#endif
#ifndef CONFIG_SF_DEFAULT_BUS
# define CONFIG_SF_DEFAULT_BUS      0
#endif

/*
 *	cloner argument
 */
enum medium_type {
	MEMORY = 0,
	NAND,
	MMC,
	I2C,
	EFUSE,
	REGISTER,
	SPI_NOR,
	SFC_NOR,
	SPI_NAND,
	SFC_NAND,
	EXT_POL,
};

enum data_type {
	RAW = 0,
	OOB,
	IMAGE,
	MTD_RAW,
	MTD_UBI,
};

struct i2c_args {
	int clk;
	int data;
	int device;
	int value_count;
	int value[0];
};

struct mmc_erase_range {
	uint32_t start;
	uint32_t end;
};

struct spi_args {
	uint32_t clk;
	uint32_t data_in;
	uint32_t data_out;
	uint32_t enable;
	uint32_t rate;   // the ssi rate
	uint32_t sfc_rate;// the sfc rate
	uint32_t sfc_quad_mode;// the sfc rate
};


struct arguments {
	int efuse_gpio;
	int use_nand_mgr;
	int use_nand_mtd;
	int use_mmc;
	uint32_t use_spi_nor;
	uint32_t use_sfc_nor;
	uint32_t use_spi_nand;
	uint32_t use_sfc_nand;

	int nand_erase;
	int nand_erase_count;
	unsigned int offsets[32];

	int mmc_open_card;
	int mmc_erase;
	uint32_t mmc_erase_range_count;
	struct mmc_erase_range mmc_erase_range[MMC_ERASE_CNT_MAX];

	struct spi_args spi_args;
	uint32_t spi_erase_block_siz;
	uint32_t spi_erase;

	int transfer_data_chk;
	int write_back_chk;

	PartitionInfo PartInfo;
	MTDPartitionInfo MTDPartInfo;
	int nr_nand_args;
	nand_flash_param nand_params[0];
};

union cmd {
	struct update {
		uint32_t length;
	}update;

	struct write {
		uint64_t partation;
		uint32_t ops;
		uint32_t offset;
		uint32_t length;
		uint32_t crc;
	}write;

	struct read {
		uint64_t partation;
		uint32_t ops;
		uint32_t offset;
		uint32_t length;
	}read;

	struct check {
		uint64_t partation;
		uint32_t ops;
		uint32_t offset;
		uint32_t check;
	} check;

	struct rtc_time rtc;
};

#define MOUDLE_TYPE(ops) ((ops) >> 16)
#define MOUDLE_SUB_TYPE(ops) ((ops) & 0xffff)

struct cloner {
	struct usb_function usb_function;
	struct usb_composite_dev *cdev;		/*Copy of config->cdev*/
	struct usb_gadget *gadget;	/*Copy of cdev->gadget*/
	struct usb_ep *ep0;		/*Copy of gadget->ep0*/
	struct usb_request *ep0req;	/*Copy of cdev->req*/

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	struct usb_request *write_req;
	struct usb_request *args_req;
	struct usb_request *read_req;

	union cmd *cmd;
	int cmd_type;
	void *buf;
	uint32_t buf_size;
	int ack;
	int crc;
	struct arguments *args;
	int inited;

	/*used for mtd, ubi*/
	int full_size;
	void* spl_title;
	int spl_title_sz;
	int skip_spl_size;
	int full_size_remainder;
	uint32_t last_offset;
	uint32_t last_offset_avail;
};

static const char burntool_name[] = "INGENIC VENDOR BURNNER";

static struct usb_string burner_intf_string_defs[] = {
	[0].s = burntool_name,
	{}
};

static struct usb_gadget_strings  burn_intf_string = {
	.language = 0x0409, /* en-us */
	.strings = burner_intf_string_defs,
};

static struct usb_gadget_strings  *burn_intf_string_tab[] __attribute__((unused))= {
	&burn_intf_string,
	NULL,
};

static struct usb_interface_descriptor intf_desc = {
	.bLength =              sizeof(intf_desc),
	.bDescriptorType =      USB_DT_INTERFACE,
	.bNumEndpoints =        2,
};

static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN|0x1,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT|0x1,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN|0x1,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT|0x1,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
};

static struct usb_descriptor_header *fs_intf_descs[] __attribute__((unused)) = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &fs_bulk_out_desc,
	(struct usb_descriptor_header *) &fs_bulk_in_desc,
	NULL,
};

static struct usb_descriptor_header *hs_intf_descs[] __attribute__((unused)) = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &hs_bulk_out_desc,
	(struct usb_descriptor_header *) &hs_bulk_in_desc,
	NULL,
};

static inline struct cloner *func_to_cloner(struct usb_function *f)
{
	return container_of(f, struct cloner, usb_function);
}

static const uint32_t crc_table[] = {
	/* CRC polynomial 0xedb88320 */
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static inline uint32_t local_crc32(uint32_t crc,unsigned char *buffer, uint32_t size)
{
	uint32_t i;
	for (i = 0; i < size; i++) {
		crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
	}
	return crc ;
}

struct cloner_moudle {
	uint32_t medium;
	int ops;
	struct list_head node;
	int (*init)(void *args, void* mdata);
	int (*write)(struct cloner *cloner, int sub_ops, void* mdata);
	int (*read)(struct cloner *cloner, int sub_ops, void* mdata);
	int (*check)(struct cloner *cloner, int sub_ops, void* mdata);
	void *data;
};

int register_cloner_moudle(struct cloner_moudle *clmd);

typedef int (*cloner_regcall_t)(void);


#define CLONER_MOUDLE_INIT(fn)		\
	ll_entry_declare(cloner_regcall_t, _1##fn, cloner) = fn

#define CLONER_SUB_MOUDLE_INIT(fn)	\
	ll_entry_declare(cloner_regcall_t, _2##fn, cloner) = fn

/*----------------------------------------------------------------*/

int clmg_init(void *args);

int clmg_check(struct cloner *cloner);

int clmg_read(struct cloner *cloner);

int clmg_write(struct cloner *cloner);

static inline int cloner_moudle_init(void)
{
	cloner_regcall_t *call = ll_entry_start(cloner_regcall_t, cloner);
	int ret = 0 , i, count;

	for (i = 0, count = ll_entry_count(cloner_regcall_t, cloner);
			i < count;
			call++, i++) {
		ret = (*call)();
		if (ret) {
			printf("cloner ops init func error\n");
			break;
		}
	}
	return ret;
}
#endif
