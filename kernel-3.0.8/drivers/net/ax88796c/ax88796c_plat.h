#ifndef _AX796C_PLAT_H_
#define _AX796C_PLAT_H_

/* Exported DMA operations */
int ax88796c_plat_dma_init (unsigned long base_addr,
			    void (*tx_dma_complete)(void *data),
			    void (*rx_dma_complete)(void *data),
			    void *priv);
void ax88796c_plat_dma_release (void);
void dma_start (dma_addr_t dst, int len, u8 tx);

/* Plat Initialise */
void ax88796c_plat_init (struct ax88796c_device *ax_local, int bus_width);
#endif	/* _AX796C_PLAT_H_ */
