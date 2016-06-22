#ifndef _DMMU_V1_2_H
#define _DMMU_V1_2_H

#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/io.h>
#include <linux/sched.h>

#define DMMU_NAME "dmmu"

struct dmmu_mem_info {
	int size;
	int page_count;

	unsigned long paddr;

	unsigned long vaddr;
	void *pages_phys_addr_table;
	unsigned int start_offset;
	unsigned int end_offset;
};

struct dmmu_global_data {
	spinlock_t list_lock;
	struct miscdevice misc_dev;
	struct list_head process_list;
	unsigned long res_page;			/* phys_addr | 0xFFF */
	unsigned long res_pte;			/* phys_addr | DMMU_PGD_VLD | DMMU_PGD_RES */
};

struct proc_page_tab_data {
	struct list_head list;
	pid_t pid;

	int * alloced_pgd_vaddr;
	int alloced_pgd_paddr;
	int ref_times;
};

struct dmmu_dma_flush_cache {
	unsigned int vaddr;
	unsigned int size;
	enum dma_data_direction mode;
};

#define KSEG0_LOW_LIMIT		0x80000000
#define KSEG1_HEIGH_LIMIT	0xC0000000
#define SECONDARY_SIZE		0x400000

/*Ioctl*/
#define DMMU_IOCTL_MAGIC 'd'
#define DMMU_GET_TLB_PGD_BASE_PHYS         _IOR(DMMU_IOCTL_MAGIC,  0x03, unsigned int)
#define DMMU_MATCH_USER_MEM                _IOW(DMMU_IOCTL_MAGIC,  0x04, unsigned int)
#define DMMU_MAP_USER_MEM                  _IOW(DMMU_IOCTL_MAGIC,  0x05, unsigned int)
#define DMMU_UNMAP_USER_MEM                _IOW(DMMU_IOCTL_MAGIC,  0x06, unsigned int)

#define DMMU_DMA_FLUSH_CACHE			   _IOW(DMMU_IOCTL_MAGIC,  0x20, unsigned int)
/*convert user vaddr to phy addr*/
#define DMMU_CONVERT_USERVADDR_TO_PADDR    _IOWR(DMMU_IOCTL_MAGIC, 0x21, unsigned int)

/*Debug*/
#define DMMU_DUMP_MEM_INFO                 _IOR(DMMU_IOCTL_MAGIC,  0x40, unsigned int)
#define DMMU_BREAKUP_PHYS_ADDR             _IOWR(DMMU_IOCTL_MAGIC, 0x41, unsigned int)

/*Interface*/
struct proc_page_tab_data *dmmu_open_handle(pid_t pid);
int get_pgd_base(struct proc_page_tab_data *table,unsigned int *pbase);
int dmmu_match_handle(struct dmmu_mem_info *mem);
int mem_map_new_tlb(struct proc_page_tab_data *table,struct dmmu_mem_info *mem);
int mem_unmap_new_tlb(struct proc_page_tab_data *table,struct dmmu_mem_info *mem);
int dmmu_release_handle(struct proc_page_tab_data *table);

struct proc_page_tab_data *dmmu_get_table(pid_t pid);


#endif
