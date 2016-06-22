/*
 * dummy_plat_dma.c
 *
 * For Dummy Plat
 */
#include "ax88796c.h"
#include "ax88796c_plat.h"

#if !defined(CONFIG_ARCH_S3C2410) && !defined(CONFIG_SOC_4780)
void ax88796c_plat_init (void __iomem *confbase, int bus_width) { }

void dma_start (dma_addr_t dst, int len, u8 tx) { }

void ax88796c_plat_dma_release (void) { }

int ax88796c_plat_dma_init (unsigned long base_addr,
				void (*tx_dma_complete)(void *data),
				void (*rx_dma_complete)(void *data),
				void *priv)
{
	return 0;
}
#endif
