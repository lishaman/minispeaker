#ifndef __ASM_MACH_GENERIC_KMALLOC_H
#define __ASM_MACH_GENERIC_KMALLOC_H


#ifndef CONFIG_DMA_COHERENT
/*
 * Total overkill for most systems but need as a safe default.
 * Set this one if any device in the system might do non-coherent DMA.
 */
#define ARCH_DMA_MINALIGN	128
#endif

#ifdef CONFIG_FINER_KMALLOC
#define FINER_KMALLOC_MIN_SIZE	32
#endif	/* CONFIG_FINER_KMALLOC */

#endif /* __ASM_MACH_GENERIC_KMALLOC_H */
