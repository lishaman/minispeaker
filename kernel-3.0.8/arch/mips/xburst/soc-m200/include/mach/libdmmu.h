#ifndef _LIBDMMU_
#define _LIBDMMU_

unsigned long dmmu_map(unsigned long vaddr,unsigned long len);
int dmmu_unmap(unsigned long vaddr, int len);
int dmmu_unmap_all(void);
void dmmu_dump_vaddr(unsigned long vaddr);

#endif
