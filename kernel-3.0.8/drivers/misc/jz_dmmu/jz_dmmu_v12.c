/* drivers/misc/dmmu_v1_2.c
 *
 * DMMU: Device MMU, TLB TABLE for M200: H2D,VPU,VIDEO-DEC,VIDEO-ENC
 *
 * Copyright (C) 2013 Ingenic
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/mempolicy.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <linux/vmalloc.h>
#include <asm/rjzcache.h>

#include "jz_dmmu_v12.h"
#include <mach/jz_libdmmu.h>

#define PGD_SIZE	(1 << (32 - 10))
#undef _ALIGN_UP
#define _ALIGN_UP(addr,size)    (((addr)+((size)-1))&(~((size)-1)))
#undef _ALIGN_DOWN
#define _ALIGN_DOWN(addr,size)  ((addr)&(~((size)-1)))
#define PGD_ALIGN(addr) _ALIGN_UP((unsigned long)(addr), PGD_SIZE)

#define PGD_INDEX(addr) ((unsigned int)pgd_index((unsigned long)(addr)))
#define PTE_INDEX(addr)								\
	((unsigned int)(((unsigned long)(addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1)))
#define PTE_VADDR(pgd, addr)						\
	((unsigned int *)phys_to_virt(*((pgd) + PGD_INDEX(addr)) & PAGE_MASK))
#define PTE_OFFSET(pgd, addr)						\
	(PTE_VADDR(pgd, addr) + PTE_INDEX(addr))
#define UNCACHE(addr)	((typeof(addr))(((unsigned long)(addr)) | 0xa0000000))

#ifdef CONFIG_JZ_DMMU_V12
/* #define DMMU_SHARE_PTE */
#endif
/*
   Default break strategy :  valid(n) <--> valid(n+1)
   if set 0, break strategy : vaild(first) <--> valid(last)
*/
#define BREAK_STRATEGY 0
/* #define DEBUG_DMMU */
static struct dmmu_global_data jz_dmmu;

static unsigned int get_phy_addr(unsigned int vaddr,int map)
{
	unsigned int addr = vaddr & (PAGE_SIZE-1);
	pgd_t *pgdir;
#ifdef CONFIG_PGTABLE_4
	pud_t *pudir;
#endif
	pmd_t *pmdir;
	pte_t *pte;
	pgdir = pgd_offset(current->mm, vaddr);
	if(pgd_none(*pgdir) || pgd_bad(*pgdir))
		return 0;
#ifdef CONFIG_PGTABsE_4
	pudir = pud_offset(pgdir, vaddr);
	if (pud_none(*pudir) || pud_bad(*pudir))
		return 0;
	pmdir = pmd_offset(pudir, vaddr);
	if (pmd_none(*pmdir) || pmd_bad(*pmdir))
		return 0;
#else
	pmdir = pmd_offset((pud_t *)pgdir, vaddr);
	if(pmd_none(*pmdir) || pmd_bad(*pmdir))
		return 0;
#endif
	pte = pte_offset(pmdir,vaddr);
	if (pte_present(*pte)) {
		/* struct page *page = pte_page(*pte); */
		/* if (map) */
		/* 	SetPageUnevictable(page); */
		/* else */
		/* 	ClearPageUnevictable(page); */
		return addr | (pte_pfn(*pte) << PAGE_SHIFT);
	}
	return 0;
}

static int dmmu_virt_to_phys(void *vaddr)
{
	unsigned int paddr = 0;
	struct page *page;

	if((unsigned int)vaddr < KSEG0_LOW_LIMIT){
			paddr=get_phy_addr((unsigned int)vaddr,1);
	}else if((unsigned int)vaddr >= KSEG0_LOW_LIMIT && (unsigned int)vaddr < KSEG1_HEIGH_LIMIT){
			paddr=virt_to_phys(vaddr);
	}else{
			page = vmalloc_to_page(vaddr);
			paddr = page_to_phys(page);
	}
	return paddr;
}

static int pgd_num_cale(struct dmmu_mem_info *mem)
{
	unsigned int tmp = 0,pgd_num = 0;
	unsigned long vaddr;
	vaddr = mem->vaddr;

	tmp = ((vaddr+(SECONDARY_SIZE-1))&(~(SECONDARY_SIZE -1))) - vaddr;
	if(mem->size-tmp > 0){
		if(tmp !=0 )
			pgd_num++;
		tmp = ((mem->size-tmp)+(SECONDARY_SIZE - 1))&(~(SECONDARY_SIZE - 1));
		pgd_num = pgd_num + tmp/SECONDARY_SIZE;
	}else{
		pgd_num++;
	}
	return pgd_num;
}

struct proc_page_tab_data *dmmu_get_table(pid_t pid)
{
	struct proc_page_tab_data *table_tmp = NULL;
	spin_lock(&jz_dmmu.list_lock);
	list_for_each_entry(table_tmp, &jz_dmmu.process_list, list) {
		if(table_tmp->pid == pid){
			spin_unlock(&jz_dmmu.list_lock);
			return table_tmp;
		}
	}
	if(table_tmp == NULL)
		printk("+++ERROR:dmmu_get_table failed\n");
	spin_unlock(&jz_dmmu.list_lock);
	return NULL;
}

struct proc_page_tab_data *dmmu_open_handle(pid_t pid)
{
	struct proc_page_tab_data *table;
	struct proc_page_tab_data *table_tmp;
	unsigned long alloc_pgd_paddr;
	int *alloc_pgd_vaddr;
#ifndef DMMU_SHARE_PTE
	unsigned int pgd_index;
#endif

	/*printk("++dmmu_current pid=%d tgid=%d (%d)\n",
			current->pid, current->tgid,pid );*/

	spin_lock(&jz_dmmu.list_lock);
	list_for_each_entry(table_tmp, &jz_dmmu.process_list, list) {
		if(table_tmp->pid == pid){
			printk("++NOTICE: You need only open /dev/dmmu one time in the same process!++\n");
			/*printk("++table_tmp=%p\n",table_tmp);*/
			table_tmp->ref_times++;
			spin_unlock(&jz_dmmu.list_lock);
			return table_tmp;
		}
	}
	spin_unlock(&jz_dmmu.list_lock);

	table = kzalloc(sizeof(struct proc_page_tab_data), GFP_KERNEL);
	/*printk("++++table=%p\n",table);*/
	alloc_pgd_vaddr = (int *)__get_free_pages(GFP_KERNEL, 0);
	SetPageReserved(virt_to_page(alloc_pgd_vaddr));
	alloc_pgd_paddr = virt_to_phys((void *)alloc_pgd_vaddr);
#ifndef DMMU_SHARE_PTE
	for (pgd_index=0; pgd_index<PTRS_PER_PGD; pgd_index++)
		alloc_pgd_vaddr[pgd_index] = jz_dmmu.res_pte;
	dma_cache_sync(NULL, alloc_pgd_vaddr, PAGE_SIZE, DMA_TO_DEVICE);
#endif
	table->alloced_pgd_vaddr = alloc_pgd_vaddr;
	table->alloced_pgd_paddr = alloc_pgd_paddr;
	table->ref_times++;
	table->pid = pid;
	spin_lock(&jz_dmmu.list_lock);
	list_add_tail(&table->list, &jz_dmmu.process_list);
	spin_unlock(&jz_dmmu.list_lock);

	return table;
}

#ifndef DMMU_SHARE_PTE
int dmmu_free_pte(struct proc_page_tab_data *table, unsigned int pgd_index_start, unsigned int pgd_index_end)
{
	unsigned int *pgd_vaddr, *pte_vaddr;
	unsigned int pgd_index;

	/*pgd_vaddr = UNCACHE(table->alloced_pgd_vaddr);*/
	pgd_vaddr = (table->alloced_pgd_vaddr);
	for (pgd_index=pgd_index_start; pgd_index<=pgd_index_end; pgd_index++) {
		if (pgd_vaddr[pgd_index] == jz_dmmu.res_pte)
			continue;
		pte_vaddr = phys_to_virt(pgd_vaddr[pgd_index] & PAGE_MASK);
		ClearPageReserved(virt_to_page(pte_vaddr));
		free_page((unsigned long)pte_vaddr);
		pgd_vaddr[pgd_index] = jz_dmmu.res_pte;
	}

	return 0;
}
#endif

static int dmmu_open(struct inode *inode, struct file *file)
{
	struct proc_page_tab_data *table = NULL;
	table = dmmu_open_handle(current->tgid);
	if(table == NULL){
		printk("+++ERROR:dmmu open failed\n");
		return -1;
	}
	file->private_data = table;

	return 0;
}

int dmmu_release_handle(struct proc_page_tab_data *table)
{
	if(table == NULL){
		printk("+++%s warring: You may have released handle!\n",__func__);
		return 0;
	}

	if(--table->ref_times){
		printk("+++warring: You may open /dev/dmmu more than one time in the same process!++\n");
		return 0;
	}
#ifndef DMMU_SHARE_PTE
	dmmu_free_pte(table, 0, USER_PTRS_PER_PGD - 1);
#endif
	spin_lock(&jz_dmmu.list_lock);
	list_del(&table->list);
	spin_unlock(&jz_dmmu.list_lock);
	ClearPageReserved(virt_to_page(table->alloced_pgd_vaddr));
	free_page((unsigned long)table->alloced_pgd_vaddr);
	kfree(table);
	return 0;
}

static int dmmu_release(struct inode *inode, struct file *file)
{
	struct proc_page_tab_data *table = dmmu_get_table(current->tgid);
	dmmu_release_handle(table);
	return 0;
}

int get_pgd_base(struct proc_page_tab_data *table,unsigned int *pbase)
{
	*pbase = table->alloced_pgd_paddr;
	return 0;
}

static int dmmu_flush_secondary_page_table(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
	int i;
	int *pgd_base;
	int pgd_index;
	int pte_index,pte_index_end;
	unsigned int pgd_num = -1;
	unsigned long vaddr, vaddr_end;
	unsigned int * pte_addr,*pte_addr_v;

	pgd_base = (unsigned int *)(table->alloced_pgd_vaddr);
	vaddr = mem->vaddr;
	vaddr_end = mem->vaddr + mem->size;
	pte_index = (vaddr & 0x3ff000) >> 12;
	pte_index_end = (vaddr_end & 0x3ff000) >> 12;

	pgd_num = pgd_num_cale(mem);
	/*printk("flush secondary --MEM: vaddr=0x%p  size=0x%08x (%d)   pgd_num=%d\n ",
			mem->vaddr,mem->size,mem->size,pgd_num);*/
	for(i = 0; i < pgd_num; i++){
		pgd_index = vaddr >> 22;
		pte_addr = (unsigned int *)(*(pgd_base + pgd_index));
		/*printk("++++++++++++pgd index 0x%08x\n",*(pgd_base + pgd_index));*/
		pte_addr = (unsigned int *)((unsigned int )pte_addr&~0xFFF);
		pte_addr_v = phys_to_virt((unsigned int)pte_addr);
		pte_addr_v = (unsigned int *)((unsigned int )pte_addr_v);

		dma_cache_sync(NULL,pte_addr_v,PAGE_SIZE,DMA_TO_DEVICE);
		vaddr += SECONDARY_SIZE;
	}
	return 0;
}

#ifdef DMMU_SHARE_PTE
int mem_map_new_tlb(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
	int i;
	int pgd_index = 0;
	unsigned long vaddr;
	unsigned int pgd_num = -1;

	pgd_t *cpu_pgd_addr = current->mm->pgd;
	vaddr = mem->vaddr;

	pgd_num = pgd_num_cale(mem);

#ifdef DEBUG_DMMU
	printk("++++++++++++++++++++mem_map_new_tlb+++++++++++++++\n");
	printk("---MEM: vaddr=0x%p size=0x%08x (%d)   pgd_num=%d\n",
			mem->vaddr,mem->size,mem->size,pgd_num);
#endif

	for(i = 0; i < pgd_num; i++){
		pgd_index = (unsigned int)vaddr >> 22;
#ifdef DEBUG_DMMU
		printk("---pgd_index=%d  cpu pgd_index_content=0x%08x \n",pgd_index,(unsigned int)(cpu_pgd_addr + pgd_index)->pgd);
#endif
		*(unsigned int *)(table->alloced_pgd_vaddr + pgd_index) =
			dmmu_virt_to_phys((void *)(cpu_pgd_addr + pgd_index)->pgd) | DMMU_PGD_VLD;

		vaddr += SECONDARY_SIZE;
	}

        /* Flush data of cpu primary page table  to DDR */
#if 0
	dma_sync_single_for_device(NULL,table->alloced_pgd_paddr,PAGE_SIZE*256,DMA_TO_DEVICE);
#else
	dma_sync_single_for_device(NULL,table->alloced_pgd_paddr,PAGE_SIZE,DMA_TO_DEVICE);
	dmmu_flush_secondary_page_table(table,mem);
#endif
	return 0;
}


int mem_unmap_new_tlb(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
#if 0
	int i,pgd_index;
	unsigned char *vaddr;
	unsigned int pgd_num = -1;
	pgd_num = pgd_num_cale(mem);

	vaddr = mem->vaddr;
	for(i = 0; i < pgd_num; i++){
		pgd_index = (unsigned int)vaddr >> 22;
		*(unsigned int *)(table->alloced_pgd_vaddr + pgd_index) = 0;
		vaddr += SECONDARY_SIZE;
	}
#endif
	return 0;
}

#else

int mem_map_new_tlb(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
	unsigned int pgd_index, pte_index;
	unsigned int *pgd_vaddr, *pte_vaddr, *cpu_pte_vaddr;
	unsigned long vaddr_start, vaddr_end;
	pgd_t *cpu_pgd_addr = current->mm->pgd;

	/*pgd_vaddr = UNCACHE(table->alloced_pgd_vaddr);*/
	pgd_vaddr = (table->alloced_pgd_vaddr);
	for (pgd_index=PGD_INDEX(mem->vaddr);
			pgd_index<=PGD_INDEX(mem->vaddr + mem->size - 1);
			pgd_index++) {
		if (pgd_vaddr[pgd_index] == jz_dmmu.res_pte) {
			pte_vaddr = (unsigned int *)__get_free_pages(GFP_KERNEL, PTE_ORDER);
			SetPageReserved(virt_to_page(pte_vaddr));
			pgd_vaddr[pgd_index] = virt_to_phys(pte_vaddr) | DMMU_PGD_VLD;
			for (pte_index=0; pte_index<PTRS_PER_PTE; pte_index++)
				pte_vaddr[pte_index] = jz_dmmu.res_page;
			/*dma_cache_sync(NULL, pte_vaddr, PAGE_SIZE << PTE_ORDER, DMA_TO_DEVICE);*/
		}
		else {
			pte_vaddr = phys_to_virt(pgd_vaddr[pgd_index] & PAGE_MASK);
		}
		cpu_pte_vaddr =(unsigned int *)((cpu_pgd_addr[pgd_index].pgd) & PAGE_MASK);
		/*pte_vaddr = UNCACHE(pte_vaddr);*/
		pte_vaddr = (pte_vaddr);
		vaddr_start = max((unsigned long)pgd_index<<PGDIR_SHIFT, mem->vaddr);
		vaddr_end = min(((unsigned long)(pgd_index<<PGDIR_SHIFT) | (PGD_SIZE - 1)),
				mem->vaddr + mem->size - 1);
		for(pte_index=PTE_INDEX(vaddr_start); pte_index<=PTE_INDEX(vaddr_end); pte_index++) {
			unsigned int pte_tmp;
			pte_tmp = cpu_pte_vaddr[pte_index];
			if(pte_tmp & _PAGE_PRESENT) {
				pte_vaddr[pte_index] = pte_tmp | DMMU_PTE_VLD;
			} else {
				printk("BUG ON!! Stop here in [%s,%d]\n",__func__,__LINE__);
				printk("cpu_pte_vaddr[%d] = 0x%x, mem->vaddr  = 0x%x,\
						mem->size = %d, vaddr_start = 0x%x,\
						vaddr_end = 0x%x, pte_index = %d\n",
						pte_index, cpu_pte_vaddr[pte_index],
						(unsigned int)mem->vaddr, (unsigned int)mem->size,
						(unsigned int)vaddr_start, (unsigned int)vaddr_end,
						pte_index);
				BUG_ON(1);
			}

		}
		dma_cache_sync(NULL, pte_vaddr, PAGE_SIZE << PTE_ORDER, DMA_TO_DEVICE);
	}
	return 0;
}

#if 0
int dump_tlb_info(unsigned long tlb_base)
{
	unsigned int pgd_index, pte_index;
	unsigned int *pgd_vaddr, *pte_vaddr, *cpu_pte_vaddr;
	unsigned long vaddr_start, vaddr_end;

	pgd_vaddr = virt_to_phys(tlb_base);
	for (pgd_index=0; pgd_index<512; pgd_index++) {
		printk("pgd_index:%d, content:0x%x\n", pgd_index, pgd_vaddr[pgd_index]);
		if (!(pgd_vaddr[pgd_index] & 0x1)) {
			printk("pgd dir invalid!\n");
			continue;
		}
		pte_vaddr = phys_to_virt(pgd_vaddr[pgd_index] & PAGE_MASK);
		for (pte_index=0; pte_index<1024; pte_index++)
			printk("    pte_index:%d, content:0x%x\n", pte_vaddr[pte_index]);
	}
	return 0;
}
#endif

int _mem_unmap_new_tlb(struct proc_page_tab_data *table, unsigned long start, unsigned long end)
{
	unsigned int *pte_vaddr, *pgd_vaddr;
	unsigned int pte_index;

	/*pgd_vaddr = UNCACHE(table->alloced_pgd_vaddr);*/
	pgd_vaddr = (table->alloced_pgd_vaddr);
	pte_vaddr = PTE_VADDR(table->alloced_pgd_vaddr, start);
	for (pte_index=PTE_INDEX(start); pte_index<=PTE_INDEX(end); pte_index++)
		pte_vaddr[pte_index] = jz_dmmu.res_page;
	for (pte_index=0; pte_index<PTRS_PER_PTE; pte_index++) {
		if (pte_vaddr[pte_index] != jz_dmmu.res_page)
			return -1;
	}
	ClearPageReserved(virt_to_page(pte_vaddr));
	free_page((unsigned long)pte_vaddr);
	pgd_vaddr[PGD_INDEX(start)] = jz_dmmu.res_pte;
	return 0;
}

int mem_unmap_new_tlb(struct proc_page_tab_data *table, struct dmmu_mem_info *mem)
{
	unsigned int *pgd_vaddr;
	unsigned int pgd_index;
	unsigned long thr1, thr2, end;

	return 0;
	/*pgd_vaddr = UNCACHE(table->alloced_pgd_vaddr);*/
	pgd_vaddr = (table->alloced_pgd_vaddr);
	end = (unsigned long)mem->vaddr + mem->size - 1;
	thr1 = PGD_ALIGN(mem->vaddr);
	thr2 = end & PGDIR_MASK;
	if (thr2 < thr1) {
		pgd_index = PGD_INDEX(mem->vaddr);
		if ((pgd_vaddr[pgd_index] & DMMU_PGD_VLD)) {
			_mem_unmap_new_tlb(table, (unsigned long)mem->vaddr,
						(unsigned long)mem->vaddr + mem->size - 1);
		}
	}
	else {
		pgd_index = PGD_INDEX(mem->vaddr);
		if (pgd_vaddr[pgd_index] & DMMU_PGD_VLD)
			_mem_unmap_new_tlb(table, (unsigned long)mem->vaddr, thr1 - 1);

		dmmu_free_pte(table, PGD_INDEX(thr1), PGD_INDEX(thr2) - 1);

		pgd_index = PGD_INDEX(thr2);
		if (pgd_vaddr[pgd_index] & DMMU_PGD_VLD)
			_mem_unmap_new_tlb(table, thr2, end);
	}

	return 0;
}
#endif

int dmmu_match_handle(struct dmmu_mem_info *mem)
{
	volatile char *start	= (char *)(mem->vaddr & 0xFFFFFFFC);
	volatile char *end	= (char *)((mem->vaddr + mem->size - 1) & 0xFFFFFFFC);

	for (; start<end; start+=PAGE_SIZE)
		*start = *start;
	*end = *end;

	/* Do not remove this code; */
#if 0
	/**refer to kernel/mm/memory.c
	 *make_pages_present(...)*/

#if 1
	 make_pages_present((unsigned long )mem->vaddr,(unsigned long)(mem->vaddr+mem->size-1));
#else
	int ret, len, write;
	struct vm_area_struct * vma;
	unsigned long vm_page_prot, addr, end;

	/*printk("-----vaddr=0x%p  size=0x%08x\n",mem->vaddr,mem->size);*/
	addr = (unsigned long)mem->vaddr;
	end  = (unsigned long)(mem->vaddr + mem->size);
	vma = find_vma(current->mm, addr);
	if (!vma)
		return -ENOMEM;

	vm_page_prot = pgprot_val(vma->vm_page_prot);
	vma->vm_page_prot = __pgprot(vm_page_prot | _PAGE_VALID| _PAGE_ACCESSED | _PAGE_PRESENT);

	write = (vma->vm_flags & (VM_WRITE | VM_SHARED)) == VM_WRITE;
	BUG_ON(addr >= end);
	BUG_ON(end > vma->vm_end);
	len = DIV_ROUND_UP(end, PAGE_SIZE) - addr/PAGE_SIZE;
	ret = get_user_pages(current, current->mm, addr,
			len, write, 0, NULL, NULL);

	vma->vm_page_prot = __pgprot(vm_page_prot);

	if (ret < 0)
		return ret;
	return ret == len ? 0 : -EFAULT;

#endif
#endif
	return 0;
}

static int dmmu_dump_mem_info(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
	unsigned int pgd_index, pte_index;
	unsigned int *pgd_vaddr, *pte_vaddr;
	unsigned long start = mem->vaddr;
	unsigned long end = mem->vaddr + mem->size - 1;
	unsigned int *data;
	int count = 0;

	printk("\n======================dmmu dump==========================\n");
	pgd_vaddr = table->alloced_pgd_vaddr;
	printk("gpd base virt_addr:%p, phys_addr:%#x",
			pgd_vaddr, table->alloced_pgd_paddr);
#ifndef DMMU_SHARE_PTE
	printk("  use dmmu inner pte\n");
	printk("Reserved pte paddr:%#lx, Reserved page paddr:%#lx\n",
			jz_dmmu.res_pte & PAGE_MASK, jz_dmmu.res_page & PAGE_MASK);
#else
	printk("  use system pte\n");
#endif
	printk("\nPage tables of designated mem area:\n");
	printk("mem start:%#lx, end:%#lx, size:%#x\n", start, end, mem->size);

	for (; start <= end; start+=PAGE_SIZE) {
		pgd_index = PGD_INDEX(start);
#ifndef DMMU_SHARE_PTE
		if (pgd_vaddr[pgd_index] == jz_dmmu.res_pte) {
#else
		if (!(pgd_vaddr[pgd_index] & DMMU_PGD_VLD)) {
#endif
			printk("[%#lx~%#lx] pgd[%u]:%u not mmap!\n",
					start & PAGE_MASK, PAGE_ALIGN(start) - 1,
					pgd_index, pgd_vaddr[pgd_index]);
			continue;
		}

		pte_vaddr = phys_to_virt(pgd_vaddr[pgd_index] & PAGE_MASK);
		pte_index = PTE_INDEX(start);
		printk("[%#lx~%#lx] pgd[%u]:%#x, pte[%u]:%#x",
				start & PAGE_MASK, PAGE_ALIGN(start) - 1,
				pgd_index, pgd_vaddr[pgd_index],
				pte_index, pte_vaddr[pte_index]);
		data = ioremap(pte_vaddr[pte_index] & PAGE_MASK, sizeof(unsigned int));
		if (data) {
			printk(", data:%#x", *data);
			iounmap(data);
		}
		printk("\n");
	}

	printk("\ntotal pgd count:%d\n", PGD_INDEX(end) - PGD_INDEX(mem->vaddr) + 1);
	printk("total pte count:%d\n",
			(unsigned int)((end & PAGE_MASK) - (mem->vaddr & PAGE_MASK)) >> PAGE_SHIFT);

	printk("\nAll valid Page tables in dmmu:\n");
	for (pgd_index=0; pgd_index<USER_PTRS_PER_PGD; pgd_index++) {
#ifndef DMMU_SHARE_PTE
		if (pgd_vaddr[pgd_index] == jz_dmmu.res_pte) {
#else
		if (!(pgd_vaddr[pgd_index] & DMMU_PGD_VLD)) {
#endif
			continue;
		}
		printk("pgd[%d]:%#x\n", pgd_index, pgd_vaddr[pgd_index]);
		pte_vaddr = phys_to_virt(pgd_vaddr[pgd_index] & PAGE_MASK);
		for (pte_index=0; pte_index<PTRS_PER_PTE; pte_index++) {
			printk("    pte[%d]:%#x",
					pte_index, pte_vaddr[pte_index]);
			if (pte_vaddr[pte_index] != jz_dmmu.res_page) {
				printk("    %#x~%#x\n", (pgd_index<<PGDIR_SHIFT) | (pte_index<<PAGE_SHIFT),
						((pgd_index<<PGDIR_SHIFT) | ((pte_index+1)<<PAGE_SHIFT)) - 1);
			}
		}
		count++;
	}

	printk("\ntotal invalid pgd count:%d\n", count);
	printk("dmmu dump finish!\n\n");
	return 0;
}

#if BREAK_STRATEGY
static int dmmu_breakup_phys_addr(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
	int i,j,pgd_index,pte_index,page_num,pgd_num;
	void *vaddr;
	int size = 0;
	int *pgd_base;
	int *pte_base;
	int *page_before;
	int *page_after;

	int *page_b;
	int *page_a;

	int tmp_value;
	unsigned long tmp_vaddr;
	int pte_index_next;
	printk("------dmmu breakup mode 1\n");
	/* printk("--------break before\n"); */
	/* dump_mem_info(table,mem); */
	void *swap_page = (void *)__get_free_page(GFP_KERNEL);
	tmp_vaddr = ((unsigned int)mem->vaddr >> PAGE_SHIFT) << PAGE_SHIFT;
	page_num = ((mem->vaddr - tmp_vaddr) + mem->size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	pgd_num  = (page_num - 1) / PTRS_PER_PTE + 1;
	pgd_base = table->alloced_pgd_vaddr;
	vaddr = mem->vaddr;
	for(i = 0; i < pgd_num; i++){
		pgd_index = (unsigned int)vaddr >> 22;
		if(*(pgd_base + pgd_index) & DMMU_PGD_VLD){
			pte_base = (int *)((*(pgd_base + pgd_index) & (~0xf7f)) | 0x80000000) ;
			/*printk("----pte_base=%x  \n PTRS_PER_PTE=%d\n",pte_base,PTRS_PER_PTE);*/
			pte_index = ((unsigned int)vaddr & 0x3ff000) >> 12;
			for(j = pte_index; j < (PTRS_PER_PTE -1 ) && size <= mem->size; j+=2){
				pte_index = ((unsigned int)vaddr & 0x3ff000) >> 12;
				pte_index_next = ((unsigned int)(vaddr + PAGE_SIZE) & 0x3ff000) >> 12;
				page_b = pte_base + pte_index;
				page_a = pte_base + pte_index_next;
				if((*page_b & _PAGE_VALID) &&
				   (*page_a & _PAGE_VALID)){
					page_before = (int *)((*page_b & (~0xfff)) | 0x80000000);
					page_after  = (int *)((*page_a & (~0xfff)) | 0x80000000);

					memcpy(swap_page,(void *)page_before,PAGE_SIZE);
					memcpy((void *)page_before,(void *)page_after,PAGE_SIZE);
					memcpy((void *)page_after,swap_page,PAGE_SIZE);

					tmp_value = *page_b;
					*page_b = *page_a;
					*page_a = tmp_value;
				}
				vaddr += PAGE_SIZE*2;
				size  += PAGE_SIZE*2;
			}
			/*flush_secondary_page_table();*/
			dmmu_flush_secondary_page_table(table,mem);
		}
	}
	free_page((unsigned long) swap_page);

	/* printk("\n\n--------break after\n"); */
	/* dump_mem_info(table,mem); */
	return 0;
}
#else
static int dmmu_breakup_phys_addr(struct proc_page_tab_data *table,struct dmmu_mem_info *mem)
{
	int i,j,pgd_index,pte_index,page_num,pgd_num;
	unsigned long vaddr;
	int size = 0;
	int *pgd_base;
	int *pte_base;
	int *page_before;
	int *page_after;
	int *page_b;
	int *page_a;
	int *page_b1;
	int *page_a1;
	int *page_before1;
	int *page_after1;
	int tmp_value;
	unsigned long tmp_vaddr;
	int pte_index_next;
	/*printk("--------break before\n");
	dump_mem_info(table,mem); */
	void *swap_page = (void *)__get_free_page(GFP_KERNEL);
	printk("------dmmu breakup mode 2\n");
	tmp_vaddr = ((unsigned int)mem->vaddr >> PAGE_SHIFT) << PAGE_SHIFT;
	page_num = ((mem->vaddr - tmp_vaddr) + mem->size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	pgd_num  = (page_num - 1) / PTRS_PER_PTE + 1;
	pgd_base = table->alloced_pgd_vaddr;
	vaddr = mem->vaddr;
	/*int flag = 3;*/
	/*printk("------pgd_num=%d\n",pgd_num);*/
	for(i = 0; i < pgd_num; i++){
		pgd_index = (unsigned int)vaddr >> 22;
		if(*(pgd_base + pgd_index) & DMMU_PGD_VLD){
			pte_base = (int *)((*(pgd_base + pgd_index) & (~0xf7f)) | 0x80000000) ;
			pte_index_next = 1023;
			/*printk("----pte_base=%x  \n PTRS_PER_PTE=%d\n",pte_base,PTRS_PER_PTE);*/
			pte_index = ((unsigned int)vaddr & 0x3ff000) >> 12;
			for(j = pte_index; (j<=pte_index_next - 2) && size <= mem->size; j+=2){
				pte_index = ((unsigned int)vaddr & 0x3ff000) >> 12;
				pte_index = pte_index % 2 ? pte_index : pte_index + 1;

				page_b = pte_base + pte_index;
				page_a = pte_base + pte_index_next;
				page_b1 = pte_base + pte_index;
				page_a1 = pte_base + pte_index_next;

				if((*page_b & _PAGE_VALID) &&
				   (*page_a & _PAGE_VALID)){

					/*printk("j=%d  pte_index=%d  pte_index_next=%d\n",j,pte_index,pte_index_next);*/

					page_before = (int *)((*page_b & (~0xfff)) | 0x80000000);
					page_after  = (int *)((*page_a & (~0xfff)) | 0x80000000);
/*
					if(flag <= 2){
						printk("------before\n");
						printk("-i=%d index=%d page_b=0x%08x   b[%p]=0x%08x  b[%p]=0x%08x\n",\
								i,pte_index,*page_b,page_before,*page_before,(page_before +1),*(page_before + 1));
						printk("-i=%d index=%d page_a=0x%08x   a[%p]=0x%08x  a[%p]=0x%08x\n",\
								i,pte_index_next,*page_a,page_after,*page_after,(page_after +1),*(page_after + 1));
					}
*/
					memcpy(swap_page,(void *)page_before,PAGE_SIZE);
					memcpy((void *)page_before,(void *)page_after,PAGE_SIZE);
					memcpy((void *)page_after,swap_page,PAGE_SIZE);

					tmp_value = *page_b;
					*page_b = *page_a;
					*page_a = tmp_value;

					page_before1 = (int *)((*page_b1 & (~0xfff)) | 0x80000000);
					page_after1  = (int *)((*page_a1 & (~0xfff)) | 0x80000000);
/*
					if(flag <= 2){
						printk("------after\n");
						printk("-i=%d index=%d page_b=0x%08x   b[%p]=%x  b[%p]=%x\n",\
								i,pte_index,*page_b1,page_before1,*page_before1,(page_before1 +1),*(page_before1 + 1));
						printk("-i=%d index=%d page_a=0x%08x   a[%p]=%x  a[%p]=%x\n",\
								i,pte_index_next,*page_a1,page_after1,*page_after1,(page_after1 +1),*(page_after1 + 1));
						flag++ ;
					}
*/
				}

				vaddr += PAGE_SIZE*2;
				size  += PAGE_SIZE*2;
				pte_index_next -= 2;
			}

			pte_index = ((unsigned int)vaddr & 0x3ff000) >> 12;
			vaddr += (1023 - pte_index) * PAGE_SIZE;
			size  += (1023 - pte_index) * PAGE_SIZE;
			/*flush_secondary_page_table();*/
			dmmu_flush_secondary_page_table(table,mem);
		}
	}
	free_page((unsigned long) swap_page);

	/*printk("\n\n--------break after\n");
	dump_mem_info(table,mem);*/
	return 0;
}
#endif

static long dmmu_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int pbase;
	unsigned int p_addr;
	unsigned int v_addr;
	void __user *argp;
	struct dmmu_dma_flush_cache dma_f;
	struct dmmu_mem_info mem;
	struct proc_page_tab_data *table = dmmu_get_table(current->tgid);
	if(table == NULL){
		printk("+++%s error:You may release handle!\n",__func__);
		return -EFAULT;
	}

	argp = (void __user *)arg;
	switch (cmd){
	case DMMU_GET_TLB_PGD_BASE_PHYS:
		get_pgd_base(table,&pbase);
		if (copy_to_user(argp, &pbase, sizeof(unsigned int))) {
			return -EFAULT;
		}
		break;
	case DMMU_MATCH_USER_MEM:
		if (copy_from_user(&mem, argp, sizeof(struct dmmu_mem_info))) {
			return -EFAULT;
		}
		if (dmmu_match_handle(&mem)< 0)
			return -EFAULT;
		break;
	case DMMU_MAP_USER_MEM:
		if (copy_from_user(&mem, argp, sizeof(struct dmmu_mem_info))) {
			return -EFAULT;
		}
		mem_map_new_tlb(table,&mem);
		break;
	case DMMU_UNMAP_USER_MEM:
		if (copy_from_user(&mem, argp, sizeof(struct dmmu_mem_info))) {
			return -EFAULT;
		}
		mem_unmap_new_tlb(table,&mem);
		break;
	case DMMU_DUMP_MEM_INFO:
		if (copy_from_user(&mem, argp, sizeof(struct dmmu_mem_info))) {
			return -EFAULT;
		}
		dmmu_dump_mem_info(table, &mem);
		break;
	case DMMU_BREAKUP_PHYS_ADDR:
		if (copy_from_user(&mem, argp, sizeof(struct dmmu_mem_info))) {
			return -EFAULT;
		}
		dmmu_breakup_phys_addr(table,&mem);
		break;
	case DMMU_CONVERT_USERVADDR_TO_PADDR:
		if (copy_from_user(&v_addr, argp, sizeof(unsigned int))) {
			return -EFAULT;
		}
		p_addr = dmmu_virt_to_phys((void *)v_addr);

		if (copy_to_user(argp, &p_addr, sizeof(unsigned int))) {
			return -EFAULT;
		}
		break;
	case DMMU_DMA_FLUSH_CACHE:
		if (copy_from_user(&dma_f, argp, sizeof(struct dmmu_dma_flush_cache))) {
			return -EFAULT;
		}
		dma_cache_sync(NULL,
				(void *)dma_f.vaddr,
				dma_f.size,
				dma_f.mode);
		break;
	default:
		printk("********  UNKNOWN cmd  of DMMU **********\n  ");
		return -EFAULT;
	}
	return 0;
}

struct file_operations dmmu_fops = {
	.owner          =    THIS_MODULE,
	.open           =    dmmu_open,
	.release        =    dmmu_release,
	.unlocked_ioctl =    dmmu_ioctl,
};

static int __init dmmu_module_init(void)
{
	int err = 0;
#ifndef DMMU_SHARE_PTE
	unsigned int pte_index;
	unsigned int *res_pte_vaddr, *res_page_vaddr;
#endif

	INIT_LIST_HEAD(&jz_dmmu.process_list);
	spin_lock_init(&jz_dmmu.list_lock);

	jz_dmmu.misc_dev.name = DMMU_NAME;
	jz_dmmu.misc_dev.minor = MISC_DYNAMIC_MINOR;
	jz_dmmu.misc_dev.fops = &dmmu_fops;
#ifndef DMMU_SHARE_PTE
	res_page_vaddr = (void *)__get_free_pages(GFP_KERNEL, 0);
	SetPageReserved(virt_to_page(res_page_vaddr));
	jz_dmmu.res_page = virt_to_phys(res_page_vaddr) | 0xFFF;
	res_pte_vaddr = (unsigned int *)__get_free_pages(GFP_KERNEL, PTE_ORDER);
	SetPageReserved(virt_to_page(res_pte_vaddr));
	jz_dmmu.res_pte = virt_to_phys(res_pte_vaddr) | DMMU_PGD_VLD;
	for (pte_index=0; pte_index<PTRS_PER_PTE; pte_index++)
		res_pte_vaddr[pte_index] = jz_dmmu.res_page;
	dma_cache_sync(NULL, res_pte_vaddr, PAGE_SIZE << PTE_ORDER, DMA_TO_DEVICE);
#endif
	err = misc_register(&jz_dmmu.misc_dev);
	if (err < 0) {
		dev_err(NULL, "Unable to register dmmu driver!\n");
		return -1;
	}
	return 0;
}

static void __exit dmmu_module_exit(void)
{
#ifndef DMMU_SHARE_PTE
	struct proc_page_tab_data *table = NULL;
	unsigned int *res_pte_vaddr, *res_page_vaddr;

	spin_lock(&jz_dmmu.list_lock);
	list_for_each_entry(table, &jz_dmmu.process_list, list) {
		dmmu_free_pte(table, 0, USER_PTRS_PER_PGD - 1);
	}
	spin_unlock(&jz_dmmu.list_lock);

	res_page_vaddr = phys_to_virt(jz_dmmu.res_page & PAGE_MASK);
	ClearPageReserved(virt_to_page(res_page_vaddr));
	free_page((unsigned long)res_page_vaddr);
	res_pte_vaddr = phys_to_virt(jz_dmmu.res_pte & PAGE_MASK);
	ClearPageReserved(virt_to_page(res_pte_vaddr));
	free_page((unsigned long)res_pte_vaddr);
#endif
	misc_deregister(&jz_dmmu.misc_dev);
}

module_init(dmmu_module_init);
module_exit(dmmu_module_exit);
