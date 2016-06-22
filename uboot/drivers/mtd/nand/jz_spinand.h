#ifndef __JZ_SPINAND_H__
#define __JZ_SPINAND_H__

/*#include <linux/mtd/partitions.h>*/

#define X_ENV_LENGTH        1024
#define X_COMMAND_LENGTH    128


#define MTD_MODE		0x0
#define UBI_MANAGER		0x1

struct jz_spinand_partition {
	char *name;         /* identifier string */
	uint32_t size;          /* partition size */
	uint32_t offset;        /* offset within the master MTD space */
	u_int32_t mask_flags;       /* master MTD flags to mask out for this partition */
	u_int32_t manager_mode;		/* manager_mode mtd or ubi */
};


#endif //__JZ_SPINAND_H__
