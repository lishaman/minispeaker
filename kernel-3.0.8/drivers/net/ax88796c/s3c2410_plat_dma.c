/*
 * S3C2410 platform
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#include <asm/arch/regs-mem.h>
#include <asm/arch/regs-irq.h>
#include <asm/arch/regs-gpio.h>
#else
#include <mach/regs-gpio.h>
#include <mach/regs-mem.h>
#include <plat/regs-dma.h>
#endif

#include "ax88796c.h"
#include "ax88796c_plat.h"

struct dma_channel {

	void __iomem	*membase;
	void __iomem	*tx_csr;
	void __iomem	*rx_csr;
	int		tx_irq;
	int		rx_irq;
	unsigned	tx_inuse:1;
	unsigned	rx_inuse:1;

	dma_addr_t	dma_addr;
	void		*priv;

	void (*tx_dma_complete)(void *data);
	void (*rx_dma_complete)(void *data);

} dma_chans;

#define S2440_TX_DMA_IRQ		IRQ_DMA0
#define S2440_RX_DMA_IRQ		IRQ_DMA3

#define TX_DMA_CSR_OFFSET		0x00 /* Channel 0 */
#define RX_DMA_CSR_OFFSET		0xC0 /* Channel 3 */

#define TSZ_BURST			(1 << 28)
#define WHOLE_SERVICE			(1 << 27)
#define DSZ_DSZ_WORD			(1 << 20)
#define DCON_INT			(1 << 29)
#define DMA_STOP_CLR			(1 << 2) * 0
#define CHANNEL_ON			(1 << 1)
#define DMA_SW_REQ			(1 << 0)

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_plat_config_bank1
 * Purpose: S3C2440A memory bank configuration
 * ----------------------------------------------------------------------------
 */
static void ax88796c_plat_config_bank1 (int bus_type)
{
	/* Configure SAMSUNG S3C2440A controller for AX88796C operation */

	unsigned long tmp;

	/* lowlevel active */
#if (AX88796B_PIN_COMPATIBLE)
	tmp = __raw_readl (S3C2410_EXTINT1) & 0xFFFF8FFF;
	__raw_writel (tmp, S3C2410_EXTINT1);
#else
	tmp = (__raw_readl (S3C2410_EXTINT2) & 0xFFFF8FFF);
	__raw_writel (tmp, S3C2410_EXTINT2);
#endif
	tmp = __raw_readl (S3C2410_BWSCON);

	if(bus_type)	/* 16-bit mode */
		__raw_writel ((tmp & ~0x30) | S3C2410_BWSCON_DW1_16,
				S3C2410_BWSCON);
	else		/* 8-bit mode */
		__raw_writel ((tmp & ~0x30) | S3C2410_BWSCON_DW1_8,
				S3C2410_BWSCON);

	/* __raw_writel ((S3C2410_BANKCON_Tcah2 | S3C2410_BANKCON_Tacc8),
				S3C2410_BANKCON1); */
	__raw_writel ((S3C2410_BANKCON_Tcah1 | S3C2410_BANKCON_Tacc6),
				S3C2410_BANKCON1);
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_plat_rx_dma_interrupt
 * Purpose: SMDK2440 Rx DMA completion interrupt function
 * ----------------------------------------------------------------------------
 */
static irqreturn_t 
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
ax88796c_plat_rx_dma_interrupt(int irq, void *dev_id)
#else
ax88796c_plat_rx_dma_interrupt(int irq, void *dev_id, struct pt_regs * regs)
#endif
{
	struct dma_channel *dma_cb = &dma_chans;

	dma_cb->rx_dma_complete (dma_cb->priv);

	return IRQ_HANDLED;
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_plat_tx_dma_interrupt
 * Purpose: SMDK2440 tx DMA completion interrupt function
 * ----------------------------------------------------------------------------
 */
static irqreturn_t
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,28)
ax88796c_plat_tx_dma_interrupt(int irq, void *dev_id)
#else
ax88796c_plat_tx_dma_interrupt(int irq, void *dev_id, struct pt_regs * regs)
#endif
{
	struct dma_channel *dma_cb = &dma_chans;

	dma_cb->tx_dma_complete (dma_cb->priv);

	return IRQ_HANDLED;
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796c_plat_dma_start
 * Purpose:  Start DMA transmission by DMA on 2440
 * ----------------------------------------------------------------------------
 */
void dma_start (dma_addr_t dst, int len, u8 tx)
{
	struct dma_channel *dma_cb = &dma_chans;
	void __iomem	*csr;

	if (tx)
	{
		csr = dma_cb->tx_csr;
		__raw_writel (dst, (csr + S3C2410_DMA_DISRC));
		__raw_writel (((len + 3) >> 2) | TSZ_BURST | WHOLE_SERVICE |
				 DSZ_DSZ_WORD | DCON_INT,
				(csr + S3C2410_DMA_DCON));
		__raw_writel ((DMA_STOP_CLR | CHANNEL_ON | DMA_SW_REQ),
				(csr + S3C2410_DMA_DMASKTRIG));
	}
	else
	{
		csr = dma_cb->rx_csr;
		__raw_writel (dst ,(csr + S3C2410_DMA_DIDST));
		__raw_writel (((len + 3 ) >> 2) | TSZ_BURST | WHOLE_SERVICE |
				 DSZ_DSZ_WORD | DCON_INT,
				(csr + S3C2410_DMA_DCON));
		__raw_writel ((DMA_STOP_CLR | CHANNEL_ON | DMA_SW_REQ ),
				(csr + S3C2410_DMA_DMASKTRIG));
	}		
}

void ax88796c_plat_dma_release (void)
{

	struct dma_channel *dma_cb = &dma_chans;

	if (dma_cb->tx_inuse || dma_cb->rx_inuse) {

		if (dma_cb->tx_inuse)
			free_irq (dma_cb->tx_irq, dma_cb);
	
		if (dma_cb->rx_inuse)
			free_irq (dma_cb->rx_irq, dma_cb);
	
		iounmap (dma_cb->membase);
		release_mem_region (S3C24XX_PA_DMA, 0x200);
	}
}

int ax88796c_plat_dma_init (unsigned long base_addr,
				void (*tx_dma_complete)(void *data),
				void (*rx_dma_complete)(void *data),
				void *priv)
{
	int ret = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
	unsigned long irq_flag = SA_SHIRQ;
#else
	unsigned long irq_flag = IRQF_SHARED;
#endif

	if (tx_dma_complete || rx_dma_complete) {

		void __iomem *membase;
		struct dma_channel *dma_cb = &dma_chans;
	
		memset (dma_cb, 0, sizeof (*dma_cb));
	
		if (check_mem_region (S3C24XX_PA_DMA, 0x200)) 
			return -ENODEV;
	
		if (!request_mem_region (S3C24XX_PA_DMA, 0x200, "AX88796C-DMA"))
			return -EBUSY;
	
		membase = ioremap_nocache (S3C24XX_PA_DMA, 0x200);
		if (membase == NULL) {
			release_mem_region (S3C24XX_PA_DMA, 0x200);
			printk(KERN_ERR "dma failed to remap register block\n");
			return -EBUSY;
		}
		dma_cb->membase = membase;
		dma_cb->dma_addr = base_addr + DATA_PORT_ADDR;
		dma_cb->priv = priv;

		if (tx_dma_complete) {
	
			dma_cb->tx_csr = membase + TX_DMA_CSR_OFFSET;
			dma_cb->tx_irq = S2440_TX_DMA_IRQ;
			dma_cb->tx_dma_complete = tx_dma_complete;
		
			ret = request_irq (dma_cb->tx_irq,
					ax88796c_plat_tx_dma_interrupt,
					irq_flag, "AX88796C-TXDMA", dma_cb);
			if (ret) {
				printk ("AX88796C unable to get Tx DMA IRQ"
					" %d (errno=%d)\n",
					dma_cb->tx_irq, ret);
				iounmap (membase);
				release_mem_region (S3C24XX_PA_DMA, 0x200);
				return ret;
			}

			dma_cb->tx_inuse = 1;

			__raw_writel (0, (dma_cb->tx_csr + S3C2410_DMA_DISRCC));
			__raw_writel (0, (dma_cb->tx_csr + S3C2410_DMA_DISRC));
#if (AX88796B_PIN_COMPATIBLE)
			__raw_writel (0, (dma_cb->tx_csr + S3C2410_DMA_DIDSTC));
#else
			__raw_writel (1, (dma_cb->tx_csr + S3C2410_DMA_DIDSTC));
#endif
			__raw_writel (dma_cb->dma_addr ,
				(dma_cb->tx_csr + S3C2410_DMA_DIDST));

			printk("Tx DMA mode is enabled\n");

		}
	
		if (rx_dma_complete) {

			dma_cb->rx_csr = membase + RX_DMA_CSR_OFFSET;
			dma_cb->rx_irq = S2440_RX_DMA_IRQ;
			dma_cb->rx_dma_complete = rx_dma_complete;
		
			ret = request_irq (dma_cb->rx_irq,
					ax88796c_plat_rx_dma_interrupt,
					irq_flag, "AX88796C-RXDMA", dma_cb);
			if (ret) {
				if (dma_cb->tx_inuse)
					free_irq (dma_cb->tx_irq, dma_cb);
				iounmap (membase);
				release_mem_region (S3C24XX_PA_DMA, 0x200);
				printk ("AX88796C: unable to get Rx DMA IRQ"
					" %d (errno=%d)\n",
					dma_cb->rx_irq, ret);
				return ret;
			}
			dma_cb->rx_inuse = 1;
#if (AX88796B_PIN_COMPATIBLE)
			__raw_writel (1, (dma_cb->rx_csr + S3C2410_DMA_DISRCC));
#else
			__raw_writel (0, (dma_cb->rx_csr + S3C2410_DMA_DISRCC));
#endif
			__raw_writel (dma_cb->dma_addr,
				(dma_cb->rx_csr + S3C2410_DMA_DISRC));
			__raw_writel (0, (dma_cb->rx_csr + S3C2410_DMA_DIDSTC));
			__raw_writel (0 ,(dma_cb->rx_csr + S3C2410_DMA_DIDST));

			printk("Rx DMA mode is enabled\n");
		}

	}
	return ret;
}

void ax88796c_plat_init (void __iomem *confbase, int bus_type)
{
	ax88796c_plat_config_bank1 (bus_type);
}
