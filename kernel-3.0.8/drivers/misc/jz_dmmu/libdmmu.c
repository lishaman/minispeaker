#include "jz_dmmu_v12.h"
#include <mach/jz_libdmmu.h>
int dmmu_init(void)
{
	/*printk("++ dmmu init current pid=%d tgid=%d )\n",
			current->pid, current->tgid);*/
	dmmu_open_handle(current->tgid);
	return 0;
}

int dmmu_get_page_table_base_phys(unsigned int *phys_addr)
{
	struct proc_page_tab_data *table = NULL;

	table = dmmu_get_table(current->tgid);
	/*printk("++ dmmu get table base current pid=%d tgid=%d  table=0x%08x\n",
			current->pid, current->tgid,table);*/
	get_pgd_base(table,phys_addr);
	/*printk("++++table_base phys_addr = 0x%08X", *phys_addr);*/

	return 0;
}

int dmmu_match_user_mem_tlb(void * vaddr, int size)
{
	struct dmmu_mem_info info;

	info.vaddr = (unsigned long)vaddr;
	info.size = size;

	return dmmu_match_handle(&info);
}

int dmmu_map_user_mem(void * vaddr, int size)
{
	struct proc_page_tab_data *table;
	struct dmmu_mem_info info;

	info.vaddr = (unsigned long)vaddr;
	info.size = size;
	table = dmmu_get_table(current->tgid);

	return mem_map_new_tlb(table,&info);
}

int dmmu_unmap_user_mem(void * vaddr, int size)
{
	struct proc_page_tab_data *table;
	struct dmmu_mem_info info;

	info.vaddr = (unsigned long)vaddr;
	info.size = size;
	table = dmmu_get_table(current->tgid);

	return mem_unmap_new_tlb(table,&info);
}

int dmmu_deinit(void)
{
	struct proc_page_tab_data *table;

	table = dmmu_get_table(current->tgid);

	return dmmu_release_handle(table);
}
