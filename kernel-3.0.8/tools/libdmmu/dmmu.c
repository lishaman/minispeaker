 #include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "dmmu.h"

#define LOGD(fmt,args...) ((void)0);
#define LOGE(fmt, args...) printf(fmt, ##args)

static int dmmu_fd = -1;
int dmmu_init()
{
	if (dmmu_fd < 0) {
		dmmu_fd = open(DMMU_DEV_NAME, O_RDWR);
		if (dmmu_fd < 0) {
			LOGE("DMMU: can't open device\n");
			return -1;
		}
	}
	return 0;
}

int dmmu_get_page_table_base_phys(unsigned int *phys_addr)
{
	int ret = 0;
	if (phys_addr == NULL) {
		LOGE("phys_addr is NULL!\n");
		return -1;
	}

	if (dmmu_fd < 0) {
		LOGE("dmmu_fd < 0");
		return -1;
	}

	ret = ioctl(dmmu_fd, DMMU_GET_TLB_PGD_BASE_PHYS, phys_addr);
	if (ret < 0) {
		LOGE("dmmu_get_page_table_base_phys_addr ioctl(DMMU_GET_TLB_PGD_BASE_PHYS) failed, ret=%d\n", ret);
		return -1;
	}
	LOGD("++++table_base phys_addr = 0x%08x\n", (int *)(*phys_addr & 0xefffffff));
	return 0;
}

int dmmu_match_user_mem_tlb(void * vaddr, int size)
{
	int ret = -1;
	struct dmmu_mem_info info;

	if (vaddr==NULL)
		return 1;

	info.vaddr = vaddr;
	info.size = size;
	/*printf("++++++++++++++vaddr=0x%08x   size=0x%08x (%d)\n",vaddr,size,size);*/
	ret = ioctl(dmmu_fd, DMMU_MATCH_USER_MEM, &info);
	if (ret < 0) {
		LOGE("dmmu_match_user_mem_tlb ioctl(DMMU_MATCH_USER_MEM) failed, ret=%d\n", ret);
		return -1;
	}

    return 0;
}

int dmmu_map_user_mem(void * vaddr, int size)
{
	if (dmmu_fd < 0) {
		LOGE("dmmu_fd < 0");
		return -1;
	}

	struct dmmu_mem_info info;
	info.vaddr = vaddr;
	info.size = size;
	info.paddr = 0;
	info.pages_phys_addr_table = NULL;

	int ret = 0;
	ret = ioctl(dmmu_fd, DMMU_MAP_USER_MEM, &info);
	if (ret < 0) {
		LOGE("dmmu_map_user_memory ioctl(DMMU_MAP_USER_MEM) failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

int dmmu_unmap_user_mem(void * vaddr, int size)
{
	if (dmmu_fd < 0) {
		LOGE("dmmu_fd < 0");
		return -1;
	}

	struct dmmu_mem_info info;
	info.vaddr = vaddr;
	info.size = size;

	int ret = 0;
	ret = ioctl(dmmu_fd, DMMU_UNMAP_USER_MEM, &info);
	if (ret < 0) {
		LOGE("dmmu_unmap_user_memory ioctl(DMMU_UNMAP_USER_MEM) failed, ret=%d\n", ret);
		return -1;
	}
	return 0;
}

int dmmu_deinit()
{
	if (dmmu_fd < 0) {
		LOGE("dmmu_fd < 0");
		return -1;
	}

	close(dmmu_fd);
	dmmu_fd = -1;

	return 0;
}

int dmmu_dma_flush_cache(void *vaddr, unsigned int size, int mode)
{
	int ret = 0;
	struct dmmu_dma_flush_cache dma_f;
	dma_f.vaddr = (unsigned int)vaddr;
	dma_f.size = size;
	dma_f.mode = mode;
	ret = ioctl(dmmu_fd, DMMU_DMA_FLUSH_CACHE, &dma_f);
	if (ret < 0) {
		LOGE("dmmu_flush_cache is error ret=%d\n", ret);
		return -1;
	}

	return 0;
}

int dmmu_conver_uservaddr_to_paddr(void * vaddr, unsigned int *paddr)
{
	unsigned int v_addr = (unsigned int)vaddr;
	int ret = 0;
	if (vaddr == NULL) {
		LOGE("user vaddr is NULL!\n");
		return -1;
	}

	if (dmmu_fd < 0) {
		LOGE("dmmu_fd < 0");
		return -1;
	}

	ret = ioctl(dmmu_fd, DMMU_CONVERT_USERVADDR_TO_PADDR, &v_addr);
	if (ret < 0) {
		LOGE("dmmu_conver_uservaddr_to_padd ioctl(DMMU_CONVERT_USERVADDR_TO_PADDR) failed, ret=%d\n", ret);
		return -1;
	}

	*paddr = v_addr;
	return 0;
}

int dmmu_dump_mem_info(void * vaddr, int size)
{
	int ret = -1;
	struct dmmu_mem_info info;
	info.vaddr = vaddr;
	info.size = size;

	ret = ioctl(dmmu_fd, DMMU_DUMP_MEM_INFO, &info);
	if (ret < 0) {
		LOGD("-----------dump mem info failed\n");
		return -1;
	}
	return 0;
}

int dmmu_breakup_user_mem(void * vaddr, int size)
{
	if (dmmu_fd < 0) {
		LOGE("dmmu_fd < 0");
		return -1;
	}

	int ret = 0;
	struct dmmu_mem_info info;
	info.vaddr = vaddr;
	info.size = size;
	info.paddr = 0;
	info.pages_phys_addr_table = NULL;

	ret = ioctl(dmmu_fd, DMMU_BREAKUP_PHYS_ADDR, &info);
	if (ret < 0) {
		LOGE("dmmu_map_user_memory ioctl(DMMU_MAP_USER_MEM) failed, ret=%d\n", ret);
		return -1;
	}
	return 0;
}
