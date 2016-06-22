/*
 * jz4780_plat_dma.c
 *
 * For Jz4780 Plat
 */
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <asm/dma.h>
#include <mach/jzdma.h>

#include "ax88796c.h"
#include "ax88796c_plat.h"

enum jz_net_burst {
	TX_BURST	= 0x3, /* DMA Burst 16Bytes */
	RX_BURST	= 0x7, /* DMA Burst AUTO */
};

struct jz_ax_dma {
	struct dma_chan 	*chan;
	struct dma_slave_config	cfg;

	enum jzdma_type 	type;
	enum jz_net_burst	burst;
};

static struct jz_ax_dma tx_dma;
static struct jz_ax_dma rx_dma;

#define DCM_RDIL	(4 << 16)
static void jz_eth_dma_start(struct jz_ax_dma *dma_info, dma_addr_t mem_addr, int count)
{
	struct jzdma_channel *dmac_eth = to_jzdma_chan(dma_info->chan);
	u32 src_addr = 0;
	u32 dst_addr = 0;
	u32 dma_cmd = 0;
	u32 dma_dtc = 0;

	dma_cmd	= (dma_info->burst << DCM_TSZ_SHF) | DCM_TIE;
	dma_cmd	&= ~(DCM_SP_32 | DCM_DP_32);

	if (dma_info->cfg.direction == DMA_TO_DEVICE) {
		src_addr	= mem_addr;
		dst_addr	= dma_info->cfg.dst_addr;
		dma_cmd		|= DCM_SAI;
		dma_dtc		= (count + (16 - 1)) / 16; /* DMA burst length 16BYTE */
	} else {
		src_addr	= dma_info->cfg.src_addr;
		dst_addr	= mem_addr;
		dma_cmd		|= DCM_DAI | DCM_RDIL;
		dma_dtc		= count;
	}

	/* Prepare DMA channel */
	writel(src_addr, dmac_eth->iomem + CH_DSA);
	writel(dst_addr, dmac_eth->iomem + CH_DTA);
	writel(dma_dtc, dmac_eth->iomem + CH_DTC);
	writel(dma_info->type, dmac_eth->iomem + CH_DRT);
	writel(dma_cmd, dmac_eth->iomem + CH_DCM);

	/* Report DMA status */
	dmac_eth->status = STAT_RUNNING;
	/* Lanch DMA channel */
	writel(DCS_NDES | DCS_CTE, dmac_eth->iomem + CH_DCS);
}
#undef DCM_RDIL

void rx_dma_dump(void)
{
	struct jzdma_channel *rxdmac = to_jzdma_chan(rx_dma.chan);
	struct jzdma_channel *txdmac = to_jzdma_chan(tx_dma.chan);

	printk("Rx DMA\n");
	printk("DSA:\t\t%08x\n", readl(rxdmac->iomem + CH_DSA));
	printk("DTA:\t\t%08x\n", readl(rxdmac->iomem + CH_DTA));
	printk("DTC:\t\t%08x\n", readl(rxdmac->iomem + CH_DTC));
	printk("DRT:\t\t%08x\n", readl(rxdmac->iomem + CH_DRT));
	printk("DCM:\t\t%08x\n", readl(rxdmac->iomem + CH_DCM));
	printk("DCS:\t\t%08x\n\n", readl(rxdmac->iomem + CH_DCS));

	printk("Tx DMA\n");
	printk("DSA:\t\t%08x\n", readl(txdmac->iomem + CH_DSA));
	printk("DTA:\t\t%08x\n", readl(txdmac->iomem + CH_DTA));
	printk("DTC:\t\t%08x\n", readl(txdmac->iomem + CH_DTC));
	printk("DRT:\t\t%08x\n", readl(txdmac->iomem + CH_DRT));
	printk("DCM:\t\t%08x\n", readl(txdmac->iomem + CH_DCM));
	printk("DCS:\t\t%08x\n\n", readl(txdmac->iomem + CH_DCS));
}

void dma_start (dma_addr_t dst, int len, u8 tx)
{
	if (tx)
		jz_eth_dma_start(&tx_dma, dst, (len << 1));
	else
		jz_eth_dma_start(&rx_dma, dst, (len << 1));
}

static bool ax88796c_dma_filter(struct dma_chan *chan, void *filter_param)
{
	/* DMA physics id = 32 - chan->chan_id */
	return chan->chan_id >= 16;
}

int ax88796c_plat_dma_init (unsigned long base_addr,
			    void (*tx_dma_complete)(void *param),
			    void (*rx_dma_complete)(void *param),
			    void *priv)
{
	struct jzdma_channel *dmac;
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (tx_dma_complete) {
		tx_dma.chan			= dma_request_channel(mask, ax88796c_dma_filter, NULL);
		if (!tx_dma.chan)
			return -ENXIO;

		tx_dma.cfg.direction		= DMA_TO_DEVICE;
		tx_dma.cfg.src_addr		= (dma_addr_t)NULL;
		tx_dma.cfg.dst_addr		= (dma_addr_t)CPHYSADDR(base_addr + DATA_PORT_ADDR);
		tx_dma.burst			= TX_BURST;
		tx_dma.type			= JZDMA_REQ_AUTO;

		dmac = to_jzdma_chan(tx_dma.chan);
		dmac->tx_desc.callback		= tx_dma_complete;
		dmac->tx_desc.callback_param	= priv;
		dmac->tx_desc.cookie		= 0;
	} else
		tx_dma.chan			= NULL;

	if (rx_dma_complete) {
		rx_dma.chan			= dma_request_channel(mask, ax88796c_dma_filter, NULL);
		if (!rx_dma.chan)
			return -ENXIO;

		rx_dma.cfg.direction		= DMA_FROM_DEVICE;
		rx_dma.cfg.src_addr		= (dma_addr_t)CPHYSADDR(base_addr + DATA_PORT_ADDR);
		rx_dma.cfg.dst_addr		= (dma_addr_t)NULL;
		rx_dma.burst			= RX_BURST;
		rx_dma.type			= JZDMA_REQ_AUTO;

		dmac = to_jzdma_chan(rx_dma.chan);
		dmac->tx_desc.callback		= rx_dma_complete;
		dmac->tx_desc.callback_param	= priv;
		dmac->tx_desc.cookie		= 0;
	} else
		rx_dma.chan			= NULL;

#if 0
	printk("tx dma chan id:\t\t%d\n", tx_dma.chan->chan_id);
	printk("rx dma chan id:\t\t%d\n", rx_dma.chan->chan_id);
#endif

	return 0;
}

void ax88796c_plat_dma_release (void)
{
	if (tx_dma.chan)
		dma_release_channel(tx_dma.chan);
	if (rx_dma.chan)
		dma_release_channel(rx_dma.chan);
}

static u32 jz_sram_smcr_get(int clock)
{
	int cycle = 1000000000 / (clock / 1000); /* Unit: ps */
	u32 taw, tbp, tah, tas;

	taw = (TRDL_V33 * 1000 + cycle - 1) / cycle - 1;
	tbp = (TDV_V33 * 1000 + cycle -1) / cycle - 1;
	tah = (TDS * 1000 + cycle - 1) / cycle / 2;
	tas = (TDS * 1000 + cycle - 1) / cycle - tah - 1;

#if 0
	printk("%s: NEMC taw = %d, tbp = %d, tah = %d, tas = %d\n", AX88796C_DRV_NAME, taw, tbp, tah, tas);
#endif

	return (7 << 24) | (taw << 20) | (tbp << 16) | (tah << 12) | (tas << 8);
}

void ax88796c_plat_init (struct ax88796c_device *ax_local, int bus_width)
{
	int bank = ((0x1C000000 & 0xfffffff) - ((u32)ax_local->membase & 0xfffffff)) / 0x1000000;
	void __iomem *base = ax_local->confbase + 0x14 + (bank - 1) * 0x4;
	struct clk *ax_clk;
	int clock;
	u32 smcr_val;

	ax_clk = clk_get(NULL, "nemc");
	clock = clk_get_rate(ax_clk);

	/* Calculat SMCR value with AHB2 clock */
	smcr_val = jz_sram_smcr_get(clock);
	/* Write SMCRn Register */
	writel(smcr_val, base);

#if 0
	printk("%s: Clock: %dMHz\n", AX88796C_DRV_NAME, clock / 1000000);
	printk("%s: SMCR: %08x, bank: %d\n", AX88796C_DRV_NAME, readl(base), bank);
#endif
}
