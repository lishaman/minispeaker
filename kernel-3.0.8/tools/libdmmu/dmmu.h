#ifndef _HW_LIB_DMMU__H
#define _HW_LIB_DMMU__H

#ifdef __cplusplus
extern "C" {
#endif
#define DMMU_DEV_NAME "/dev/dmmu"

struct dmmu_mem_info {
	int size;
	int page_count;

	unsigned int paddr;

	void *vaddr;
	void *pages_phys_addr_table;

	unsigned int start_offset;
	unsigned int end_offset;
};

enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};

struct dmmu_dma_flush_cache {
	unsigned int vaddr;
	unsigned int size;
	enum dma_data_direction mode;
};

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


/*user must use interface*/
int dmmu_init();
int dmmu_deinit();
int dmmu_get_page_table_base_phys(unsigned int * phys_addr);
int dmmu_match_user_mem_tlb(void * vaddr, int size);
int dmmu_map_user_mem(void * vaddr, int size);
int dmmu_unmap_user_mem(void * vaddr, int size);

/*user optional interface*/
int dmmu_conver_uservaddr_to_paddr(void * vaddr,unsigned int *paddr);
int dmmu_dma_flush_cache(void *vaddr, unsigned int size, int mode);
int dmmu_dump_mem_info(void * vaddr, int size);
int dmmu_breakup_user_mem(void * vaddr, int size);

#ifdef __cplusplus
}
#endif
#endif
