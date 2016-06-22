/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

//#define DEBUG
//#define VERBOSE_DEBUG

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/crc32.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include "jz-mac.h"

#define DMA_PKT_MAX 256

struct dma_ring {
	int pkt_max;
	int is_rx;
	dma_addr_t dma_addr;
	struct dma_desc {
		u32 addr, size, next, data;
	} *desc;
#define DESC_EMPTY	BIT(31)
	u32 insert_flag;
	unsigned int head, tail;
	unsigned int pkt_num;
	spinlock_t lock;
};
#define ring_is_empty(ring)	((ring)->pkt_num == 0)
#define ring_is_full(ring)	((ring)->pkt_num == (ring)->pkt_max)
#define ring_dma_head(ring)						\
	((ring)->dma_addr +						\
	 ((ring)->head % DMA_PKT_MAX) * sizeof(struct dma_desc))
static struct dma_ring *ring_alloc(int is_rx, unsigned int flag)
{
	struct dma_ring *ring;
	unsigned int i, size;

	/* BUG if alloc failed */
	ring = kmalloc(sizeof(*ring), GFP_KERNEL);
	ring->pkt_max = DMA_PKT_MAX;
	ring->is_rx = is_rx;
	size = sizeof(struct dma_desc)*ring->pkt_max;
	ring->desc = dma_alloc_coherent(NULL, size, &ring->dma_addr, 0);

	for (i = 0; i < ring->pkt_max; i++) {
		size = flag;
		size |= is_rx ? 0 : DESC_EMPTY;
		ring->desc[i].size = size;
		ring->desc[i].data = 0;
		ring->desc[i].next = ring->dma_addr + 
			(i + 1) * sizeof(struct dma_desc);
	}
	ring->desc[i-1].next = ring->dma_addr;
	ring->insert_flag = flag;
	ring->pkt_num = 0;
	ring->head = 0;
	ring->tail = 0;
	spin_lock_init(&ring->lock);
	return ring;
}
static int __ring_insert(struct dma_ring *ring, dma_addr_t addr,
			 int len, u32 data)
{
	unsigned int tail, size;

	if (ring_is_full(ring))
		return -1;

	tail = ring->tail % DMA_PKT_MAX;
	ring->tail ++;
	ring->pkt_num ++;

	ring->desc[tail].addr = addr;
	ring->desc[tail].data = data;
	size = len | ring->insert_flag;
	size |= ring->is_rx? DESC_EMPTY: 0;
	wmb();
	ring->desc[tail].size = size;

	return 0;
}
static int ring_insert(struct dma_ring *ring, dma_addr_t addr,
		       int len, u32 data)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&ring->lock, flags);
	ret = __ring_insert(ring, addr, len, data);
	spin_unlock_irqrestore(&ring->lock, flags);

	return ret;
}
static struct dma_desc * __ring_deliver(struct dma_ring *ring)
{
	static struct dma_desc desc;
	unsigned int head;

	if (ring_is_empty(ring))
		return NULL;

	head = ring->head % DMA_PKT_MAX;
	desc.size = ring->desc[head].size;
	if ( (ring->is_rx && (desc.size & DESC_EMPTY)) ||
	     (!ring->is_rx && !(desc.size & DESC_EMPTY)) )
		return NULL;

	desc.size = desc.size & 0xfff;
	desc.addr = ring->desc[head].addr;
	desc.data = ring->desc[head].data;
	ring->desc[head].data = 0;
	ring->head ++;
	ring->pkt_num --;
	return &desc;
}
static struct dma_desc * ring_deliver(struct dma_ring *ring)
{
	struct dma_desc *desc;
	unsigned long flags;

	spin_lock_irqsave(&ring->lock, flags);
	desc = __ring_deliver(ring);
	spin_unlock_irqrestore(&ring->lock, flags);

	return desc;
}

/* the buffer only used for rx */
struct buffer {
#define BUF_PAGE_NUM	(DMA_PKT_MAX/2)
	short nr_page, min_page;
	unsigned long pages[BUF_PAGE_NUM];
	unsigned long page_get, page_put;
};
static struct buffer *jzmac_alloc_buffer(int min_buf_nr)
{
	int i;
	struct buffer *buf = kmalloc(sizeof(struct buffer), GFP_KERNEL);

	if (!buf)
		return NULL;

	buf->min_page = (min_buf_nr + 1)/2;
	for (i = 0; i < buf->min_page; i++) {
		unsigned long page = __get_free_page(GFP_KERNEL);
		if (page == 0)
			break;
		buf->pages[i] = page;
	}
	buf->nr_page = i;
	buf->page_get = 0;
	buf->page_put = 0;
	return buf;
}
static void *jzmac_get_buffer(struct buffer *buf, int can_alloc)
{
	unsigned long page;

	if (!buf->page_get) {
		if (!buf->nr_page && can_alloc) {
			page = __get_free_page(GFP_ATOMIC);
			buf->page_get = page;
		} else if (buf->nr_page) {
			buf->page_get = buf->pages[--buf->nr_page];
		}
	}
	if (!buf->page_get)
		return NULL;

	page = buf->page_get;
	if (buf->page_get & ~PAGE_MASK)
		buf->page_get = 0;
	else
		buf->page_get += PAGE_SIZE / 2;

	return (void*)page;
}
static void jzmac_put_buffer(struct buffer *buf, void *p)
{
	unsigned long page = (unsigned long)p;

	if (buf->page_put) {
		BUG_ON(buf->page_put + PAGE_SIZE/2 != page);
		BUG_ON(buf->nr_page >= BUF_PAGE_NUM);
		buf->pages[buf->nr_page++] = buf->page_put;
		buf->page_put = 0;
	} else {
		buf->page_put = page;
	}
}
static int jzmac_buffer_free_page(struct buffer *buf)
{
	/* once free one page */
	if (buf->nr_page > buf->min_page) {
		buf->nr_page --;
		free_page(buf->pages[buf->nr_page]);
		return 1;
	}
	return 0;
}


struct jzmac {
	int		mac_id;
	void __iomem	*iomem;
	int		irq;

	struct clk *clk;
	struct buffer *buffer;
	struct dma_ring *rx_ring;	
	struct dma_ring *tx_ring;	
	struct napi_struct napi;

	struct net_device *ndev;
	struct device *dev;
	struct mii_bus *mii_bus;
	struct phy_device *phydev;

	/* MII and PHY stuffs */
	int old_link;
	int old_speed;
	int old_duplex;
};

static inline 
unsigned long reg_read(struct jzmac *jzmac, int off)
{
	return readl(jzmac->iomem + off); 
}
static inline 
void reg_write(struct jzmac *jzmac, int off, unsigned long val)
{
	writel(val, jzmac->iomem + off); 
}
static inline 
unsigned long reg_clr(struct jzmac *jzmac, int off, 
		      unsigned long clear)
{
	unsigned long old;
	old = readl(jzmac->iomem + off); 
	clear = old & ~clear;
	writel(clear, jzmac->iomem + off);
	return old;
}
static inline 
unsigned long reg_set(struct jzmac *jzmac, int off, 
		      unsigned long set)
{
	unsigned long old;
	old = readl(jzmac->iomem + off); 
	set = old | set;
	writel(set, jzmac->iomem + off);
	return old;
}

static inline 
void jzmac_dma_enable_rx(struct jzmac *jzmac)
{
	reg_write(jzmac, MAC_DMA_RxCtrl, DMA_RxCtrl_EN);
}
static inline 
void jzmac_dma_enable_tx(struct jzmac *jzmac)
{
	reg_write(jzmac, MAC_DMA_TxCtrl, DMA_TxCtrl_EN);
}

static int jzmac_rx_ack(struct jzmac *jzmac, int max_ack)
{
	struct dma_desc *desc;
	int need, i, n;

	/* write 1 to clear stat bit */
	reg_set(jzmac, MAC_DMA_RxStat, DMA_RxStat_RxOF);
	for(n = 0; n < max_ack; n++) {
		struct sk_buff *skb;
		desc = __ring_deliver(jzmac->rx_ring);
		if (!desc) 
			break;
		/* reduce pkt count */
		reg_set(jzmac, MAC_DMA_RxStat, DMA_RxStat_RxPkt);

		skb = dev_alloc_skb(desc->size + NET_IP_ALIGN);
		if (!skb) {
			jzmac->ndev->stats.rx_dropped++;
			jzmac_put_buffer(jzmac->buffer, (void*)desc->data);
			continue;
		}

		skb_reserve(skb, NET_IP_ALIGN);
		skb_copy_to_linear_data(skb, (unsigned char *)desc->data,
					desc->size);
		skb_put(skb, desc->size);

		netdev_vdbg(jzmac->ndev,"jzmac rx ack skb %d\n", skb->len);
		skb->protocol = eth_type_trans(skb, jzmac->ndev);
		netif_rx(skb);

		dma_unmap_single(NULL, desc->addr, 
				 PAGE_SIZE/2, DMA_FROM_DEVICE);

		jzmac->ndev->stats.rx_packets ++;
		jzmac->ndev->stats.rx_bytes += desc->size;
		jzmac_put_buffer(jzmac->buffer, (void*)desc->data);
	}

	/* increament to 1.5x */
	need = n + n/2;
	if (need < 4)
		need = 4;
	if (need > DMA_PKT_MAX)
		need = DMA_PKT_MAX;
	for (i = 0; i < need - jzmac->rx_ring->pkt_num; i++) {
		dma_addr_t addr;
		void *d = jzmac_get_buffer(jzmac->buffer, 1);
		if (!d)
			break;
		addr = dma_map_single(NULL, d, 
				      PAGE_SIZE/2, DMA_FROM_DEVICE);
		__ring_insert(jzmac->rx_ring, addr, PAGE_SIZE/2, (u32)d);
	}
	jzmac_buffer_free_page(jzmac->buffer);

	if (!ring_is_empty(jzmac->rx_ring))
		jzmac_dma_enable_rx(jzmac);
	return n;
}

static void jzmac_tx_timeout(struct net_device *dev)
{
	netdev_err(dev,"jzmac tx timeout\n");
}
static int jzmac_tx_ack(struct jzmac *jzmac, int max_ack)
{
	struct dma_desc *desc;
	int n = 0;

	/* write 1 to clear stat bit */
	reg_set(jzmac, MAC_DMA_TxStat, DMA_TxStat_UdRun);
	while ((desc = ring_deliver(jzmac->tx_ring))) {
		struct sk_buff *skb = (void*)desc->data;

		/* reduce pkt count */
		reg_set(jzmac, MAC_DMA_TxStat, DMA_TxStat_TxPkt);

		dma_unmap_single(NULL, desc->addr, skb->len, DMA_TO_DEVICE);
		netdev_vdbg(jzmac->ndev,"jzmac tx ack skb %p\n", skb);

		dev_kfree_skb(skb);
		if (++n >= max_ack)
			break;
	}

	if (!ring_is_empty(jzmac->tx_ring))
		jzmac_dma_enable_tx(jzmac);
	if (n && netif_queue_stopped(jzmac->ndev))
		netif_wake_queue(jzmac->ndev);
	return n;
}
static int jzmac_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct jzmac *jzmac = netdev_priv(dev);
	dma_addr_t addr;

	netdev_vdbg(dev,"jzmac xmit skb %p %d\n", skb, skb->len);

	addr = dma_map_single(NULL, skb->data, skb->len, DMA_TO_DEVICE);
	ring_insert(jzmac->tx_ring, addr, skb->len, (u32)skb);

	if (ring_is_full(jzmac->tx_ring))
		netif_stop_queue(dev);

	jzmac_dma_enable_tx(jzmac);

	return NETDEV_TX_OK;
}

static irqreturn_t jzmac_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct jzmac *jzmac = netdev_priv(dev);
	unsigned int intr;

	intr = reg_read(jzmac, MAC_DMA_IntR);

	netdev_vdbg(dev,"jzmac intr %x\n", intr);

	if (intr & (DMA_IntR_RxErr | DMA_IntR_TxErr | 
		    DMA_IntR_RxOF | DMA_IntR_TxPkt)) {
		/* We never enable the interrupt
		 * DMA_IntR_RxOF and DMA_IntR_TxPkt
		 */
		netdev_err(dev, "interrupt err %x\n", intr);
	}
	else if (intr & (DMA_IntR_RxPkt)) {
		reg_clr(jzmac, MAC_DMA_IntM, DMA_IntR_RxPkt);
		if (likely(napi_schedule_prep(&jzmac->napi))) {
			__napi_schedule(&jzmac->napi);
		}
	}
	else if (intr & (DMA_IntR_TxUR)) {
		jzmac_tx_ack(jzmac, DMA_PKT_MAX);
	}

	return IRQ_NONE;
}

static int jzmac_poll(struct napi_struct *napi, int budget) 
{
	struct jzmac *jzmac = container_of(napi, struct jzmac, napi);
	int done;

	netdev_vdbg(jzmac->ndev,"jzmac poll %d\n", budget);
	
	done = jzmac_rx_ack(jzmac, budget);
	if (done < budget) {
		/* We don't like a DMA_IntR_TxUR interrupt here */
		reg_clr(jzmac, MAC_DMA_IntM, DMA_IntR_TxUR);
		done += jzmac_tx_ack(jzmac, budget - done);
		reg_set(jzmac, MAC_DMA_IntM, DMA_IntR_TxUR);
	}

	if (done < budget) {
		napi_complete(napi);
		reg_set(jzmac, MAC_DMA_IntM, DMA_IntR_RxPkt);
	}

	return done;
}

/* MAC control and configuration */
static void config_mac(struct jzmac *jzmac)
{
	u8 *maddr;

	/* reset mii */
	reg_set(jzmac, MAC_MII_MAC1, MII_MAC1_SFT_RST);
	mdelay(10);
	reg_clr(jzmac, MAC_MII_MAC1, MII_MAC1_SFT_RST);
	/* reset logic */
	reg_set(jzmac, MAC_MII_MAC1, 0xf00);
	mdelay(10);
	reg_clr(jzmac, MAC_MII_MAC1, 0xf00);
	/* reset rand gen */
	reg_set(jzmac, MAC_MII_MAC1, MII_MAC1_SIM_RST);
	mdelay(10);
	reg_clr(jzmac, MAC_MII_MAC1, MII_MAC1_SIM_RST);

#if defined(CONFIG_JZ4770_MAC_RMII)
	/* FIXME, why clear, not set */
	reg_clr(jzmac, MAC_MII_MAC1, MII_MAC1_RMII_MODE);
#endif

	/* set MAC Address */
	maddr = jzmac->ndev->dev_addr;
	reg_write(jzmac, MAC_MII_SA0, (maddr[0] << 8) | maddr[1]);
	reg_write(jzmac, MAC_MII_SA1, (maddr[2] << 8) | maddr[3]);
	reg_write(jzmac, MAC_MII_SA2, (maddr[4] << 8) | maddr[5]);

	// Enable tx & rx flow control, enable receive
	reg_set(jzmac, MAC_MII_MAC1, MII_MAC1_TX_PAUSE | MII_MAC1_RX_PAUSE);
	reg_set(jzmac, MAC_MII_MAC1, MII_MAC1_RX_EN);

	reg_set(jzmac, MAC_MII_MAC2, MII_MAC2_AUTO_PAD | MII_MAC2_PAD_EN |
		MII_MAC2_CRC_EN | MII_MAC2_LEN_CK | MII_MAC2_FULL_DPX);

	reg_write(jzmac, MAC_MII_IPGT, (0xc << 8) | 0x12);
}

static void config_fifo(struct jzmac *jzmac)
{
	unsigned int tmp;
	/* fifo reset all modules */
	reg_set(jzmac, MAC_FIFO_CFG_R0, 0x1f);
	mdelay(1);
	reg_clr(jzmac, MAC_FIFO_CFG_R0, 0x1f);

	/* FIXME, why do this? */
	reg_set(jzmac, MAC_FIFO_CFG_R0, 0x80000000);

	/* set min_frm_ram=1536 and xoffrtx=4 */
	reg_write(jzmac, MAC_FIFO_CFG_R1, (1536 <<16) + 4);

	/* set max_rx_watermark=1588 and min_rx_watermark=64 */
	reg_write(jzmac, MAC_FIFO_CFG_R2, (1588 <<16) + 64);

	/* for 2k FiFo tx */
	/* set max_tx_watermark=512 and min_tx_watermark=0x180*4 */
	reg_write(jzmac, MAC_FIFO_CFG_R3, (512 <<16) + 0x180*4);

	/* enable_pause_frm */
	reg_clr(jzmac, MAC_FIFO_CFG_R5, BIT(22));

	reg_write(jzmac, MAC_FIFO_CFG_R4, 0);
	reg_set(jzmac, MAC_FIFO_CFG_R5, 0xffff);

	/* drop cond: RSV_RUO | RSV_CRCE | RSV_RCV */
	tmp = RSV_RUO | RSV_CRCE | RSV_RCV;
	reg_set(jzmac, MAC_FIFO_CFG_R4,	tmp);
	reg_clr(jzmac, MAC_FIFO_CFG_R5, tmp);

	/* fifo_enable_all_module */
	reg_set(jzmac, MAC_FIFO_CFG_R0, 0x1f00);
	for (tmp = 5000; tmp > 0; tmp--) {
		if (((reg_read(jzmac, MAC_FIFO_CFG_R0) >> 16) & 0x1d) == 0x1d)
			break;
		udelay(100);
	}
	if (tmp == 0)
		netdev_err(jzmac->ndev, "start fifo timeout!\n");
}

static void config_sal(struct jzmac *jzmac)
{
	/* accept multicast */
	reg_set(jzmac, MAC_SAL_AFR, SAL_AFR_AMC);
	/* accept broadcast */
	reg_set(jzmac, MAC_SAL_AFR, SAL_AFR_ABC);

	reg_write(jzmac, MAC_SAL_HT1, 0);
	reg_write(jzmac, MAC_SAL_HT2, 0);
}

static void jzmac_mdio_poll(struct jzmac *jzmac);
static int jzmac_config_hw(struct jzmac *jzmac)
{
	/* Wait MII done */
	jzmac_mdio_poll(jzmac);

	reg_write(jzmac, MAC_DMA_TxCtrl, 0);
	reg_write(jzmac, MAC_DMA_RxCtrl, 0);

	reg_clr(jzmac, MAC_MII_MAC1, MII_MAC1_RX_EN);
	reg_write(jzmac, MAC_DMA_IntM, 0);

	config_mac(jzmac);
	config_fifo(jzmac);
	config_sal(jzmac);
	/* disable statistics(STAT module) */
	reg_clr(jzmac, MAC_DMA_IntM, DMA_IntM_STEN);

	reg_write(jzmac, MAC_DMA_RxStat, 0xf);
	while (reg_read(jzmac, MAC_DMA_RxStat))
		reg_write(jzmac, MAC_DMA_RxStat, DMA_RxStat_RxPkt);
	reg_write(jzmac, MAC_DMA_RxDesc, ring_dma_head(jzmac->rx_ring));

	reg_write(jzmac, MAC_DMA_TxStat, 0xf);
	while (reg_read(jzmac, MAC_DMA_TxStat))
		reg_write(jzmac, MAC_DMA_TxStat, DMA_TxStat_TxPkt);
	reg_write(jzmac, MAC_DMA_TxDesc, ring_dma_head(jzmac->tx_ring));

	reg_write(jzmac, MAC_DMA_IntM, DMA_IntR_RxPkt | 
		  DMA_IntR_TxUR | DMA_IntR_RxErr | DMA_IntR_TxErr);
	
	return 0;
}

static int jzmac_open(struct net_device *dev)
{
	struct jzmac *jzmac = netdev_priv(dev);
	int err;

	netdev_vdbg(dev,"jzmac open!\n");

	err = request_irq(dev->irq, jzmac_interrupt, 0,
			  dev->name, dev);
	if (err) {
		netdev_err(dev,"request IRQ failed\n");
		return err;
	}

	phy_start(jzmac->phydev);
	phy_write(jzmac->phydev, MII_BMCR, BMCR_RESET);

	jzmac_config_hw(jzmac);

	jzmac_rx_ack(jzmac, DMA_PKT_MAX);
	napi_enable(&jzmac->napi);

	jzmac_dma_enable_rx(jzmac);

	netif_start_queue(dev);

	return 0;
}

static int jzmac_close(struct net_device *dev)
{
	struct jzmac *jzmac = netdev_priv(dev);

	napi_disable(&jzmac->napi);
	netif_stop_queue(dev);
	free_irq(dev->irq, dev);
	return 0;
}


/*
 * Multicast filter and config multicast hash table
 */
#define MULTICAST_FILTER_LIMIT 64

static void jzmac_set_mclist(struct net_device *dev)
{
	struct jzmac *jzmac = netdev_priv(dev);

	netdev_vdbg(dev,"jzmac set multicast list!\n");

	if (dev->flags & IFF_PROMISC) {
		/* Accept any kinds of packets */
		reg_set(jzmac, MAC_SAL_AFR, SAL_AFR_PRO);

		/* FIXME: need save the previous value? */
		reg_write(jzmac, MAC_SAL_HT1, 0xffffffff);
		reg_write(jzmac, MAC_SAL_HT2, 0xffffffff);
		netdev_info(dev, "Enter promisc mode!\n");
	} else  if ((dev->flags & IFF_ALLMULTI) || 
		    (netdev_mc_count(dev) > MULTICAST_FILTER_LIMIT)) {
		/* Accept all multicast packets */
		reg_set(jzmac, MAC_SAL_AFR, SAL_AFR_PRO);
		reg_set(jzmac, MAC_SAL_AFR, SAL_AFR_ABC);

		/* don't drop RSV_MP */
		reg_set(jzmac, MAC_FIFO_CFG_R5,	RSV_MP);
		reg_clr(jzmac, MAC_FIFO_CFG_R4, RSV_MP);

		reg_write(jzmac, MAC_SAL_HT1, 0xffffffff);
		reg_write(jzmac, MAC_SAL_HT2, 0xffffffff);
		netdev_info(dev, "Enter allmulticast mode!\n");
	} else {
		struct netdev_hw_addr *ha;
		u32 mc_filter[2];	/* Multicast hash filter */

		mc_filter[1] = mc_filter[0] = 0;
		netdev_for_each_mc_addr(ha, dev)
			set_bit(ether_crc(ETH_ALEN, ha->addr)>>26,
				(long *)mc_filter);
		reg_write(jzmac, MAC_SAL_HT1, mc_filter[1]);
		reg_write(jzmac, MAC_SAL_HT2, mc_filter[0]);

		reg_set(jzmac, MAC_SAL_AFR, SAL_AFR_AMC);
		reg_clr(jzmac, MAC_SAL_AFR, SAL_AFR_PRO);
	}
}

static const struct net_device_ops jzmac_netdev_ops = {
	.ndo_open		= jzmac_open,
	.ndo_stop		= jzmac_close,
	.ndo_start_xmit		= jzmac_xmit,
	.ndo_tx_timeout		= jzmac_tx_timeout,
	.ndo_set_multicast_list	= jzmac_set_mclist,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_set_mac_address	= eth_mac_addr,
};


/*
 * MII operations
 */
/* Wait until the previous MDC/MDIO transaction has completed */
static void jzmac_mdio_poll(struct jzmac *jzmac)
{
	int timeout_cnt = 5000;

	while (reg_read(jzmac,MAC_MII_MIND) & MII_MIND_BUSY) {
		udelay(1);
		if (timeout_cnt-- < 0) {
			netdev_err(jzmac->ndev, "mdio poll timeout!\n");
			break;
		}
	}
}
/* Read an off-chip register in a PHY through the MDC/MDIO port */
static int jzmac_mdiobus_read(struct mii_bus *bus, int phy, int reg)
{
	struct jzmac *jzmac = bus->priv;

	jzmac_mdio_poll(jzmac);

	reg_write(jzmac, MAC_MII_MADR, (phy <<8) | reg);
	reg_write(jzmac, MAC_MII_MCMD, MII_MCMD_READ);

	jzmac_mdio_poll(jzmac);

	return reg_read(jzmac, MAC_MII_MRDD);
}
/* Write an off-chip register in a PHY through the MDC/MDIO port */
static int jzmac_mdiobus_write(struct mii_bus *bus, int phy, int reg,
			       u16 value)
{
	struct jzmac *jzmac = bus->priv;

	jzmac_mdio_poll(jzmac);

	reg_write(jzmac, MAC_MII_MADR, (phy <<8) | reg);
	reg_write(jzmac, MAC_MII_MCMD, 0);
	reg_write(jzmac, MAC_MII_MWTD, value);

	jzmac_mdio_poll(jzmac);

	return 0;
}
static int jzmac_mdiobus_reset(struct mii_bus *bus)
{
	return 0;
}

static void jzmac_adjust_link(struct net_device *dev)
{
	struct jzmac *jzmac = netdev_priv(dev);
	struct phy_device *phydev = jzmac->phydev;
	int new_state = 0;

	if (!phydev->link) 
		goto link_down;

	/* Now we make sure that we can be in full duplex mode.
	 * If not, we operate in half-duplex mode. */
	if (phydev->duplex != jzmac->old_duplex) {
		new_state = 1;
		if (DUPLEX_FULL == phydev->duplex) {
			reg_set(jzmac, MAC_MII_MAC2, MII_MAC2_FULL_DPX);
			reg_write(jzmac, MAC_MII_IPGT, 0x15);
		} else {
			reg_clr(jzmac, MAC_MII_MAC2, MII_MAC2_FULL_DPX);
			reg_write(jzmac, MAC_MII_IPGT, 0x12);
		}
		jzmac->old_duplex = phydev->duplex;
	}

	if (phydev->speed != jzmac->old_speed) {
		switch (phydev->speed) {
		case SPEED_10:
#if defined(CONFIG_JZ4770_MAC_RMII)
			reg_clr(jzmac, MAC_MII_SUPP, MII_SUPP_100M);
#endif
			break;
		case SPEED_100:
#if defined(CONFIG_JZ4770_MAC_RMII)
			reg_set(jzmac, MAC_MII_SUPP, MII_SUPP_100M);
#endif
			break;
		default:
			netdev_err(dev,"Unknown speed %d\n",phydev->speed);
			break;
		}
		new_state = 1;
		jzmac->old_speed = phydev->speed;
	}

link_down:
	if (jzmac->old_link != phydev->link) {
		new_state = 1;
		jzmac->old_link = phydev->link;
		if (jzmac->old_link) {
			jzmac->old_speed = 0;
			jzmac->old_duplex = -1;
			netif_carrier_off(dev);
		} else 
			netif_carrier_on(dev);
	}

	if (new_state)
		phy_print_status(phydev);

}

static int jzmac_mii_probe(struct net_device *dev)
{
	struct jzmac *jzmac = netdev_priv(dev);
	struct phy_device *phydev = NULL;
	int i;

	/* search for connect PHY device */
	for (i = 0; i < PHY_MAX_ADDR; i++) {
		if (jzmac->mii_bus->phy_map[i]) {
			phydev = jzmac->mii_bus->phy_map[i];
			break; /* found it */
		}
	}

	/* now we are supposed to have a proper phydev, to attach to... */
	if (!phydev) {
		dev_err(jzmac->dev,"Can't find any phy device at all\n");
		return -ENODEV;
	}

#if defined(CONFIG_JZ4770_MAC_RMII)
	phydev = phy_connect(dev, dev_name(&phydev->dev), &jzmac_adjust_link,
			     0, PHY_INTERFACE_MODE_RMII);
#else
	phydev = phy_connect(dev, dev_name(&phydev->dev), &jzmac_adjust_link,
			     0, PHY_INTERFACE_MODE_MII);
#endif

	if (IS_ERR(phydev)) {
		dev_err(jzmac->dev,"Could not attach to PHY\n");
		return PTR_ERR(phydev);
	}

	/* mask with MAC supported features */
	phydev->supported &= (SUPPORTED_10baseT_Half
			      | SUPPORTED_10baseT_Full
			      | SUPPORTED_100baseT_Half
			      | SUPPORTED_100baseT_Full
			      | SUPPORTED_Autoneg
			      | SUPPORTED_Pause | SUPPORTED_Asym_Pause
			      | SUPPORTED_MII
			      | SUPPORTED_TP);

	phydev->advertising = phydev->supported;

	jzmac->old_link = 0;
	jzmac->old_speed = 0;
	jzmac->old_duplex = -1;
	jzmac->phydev = phydev;

	dev_info(jzmac->dev, "attached PHY driver [%s] "
		 "(mii_bus:phy_addr=%s, irq=%d)\n",
		 phydev->drv->name, dev_name(&phydev->dev), phydev->irq);

	return 0;
}

static const unsigned char mdc_diver[8] = { 4, 4, 6, 8, 10, 14, 20, 28 };
static int __devinit jzmac_probe(struct platform_device *pdev)
{
	struct jzmac *jzmac;
	struct net_device *ndev;
	struct resource *base;
	int i, irq, err;

	base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!base) {
		dev_err(&pdev->dev, "failed to retrieve base register\n");
		err = -ENODEV;
		goto out;
	}
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "failed to retrieve IRQ\n");
		err = -ENODEV;
		goto out;
	}
	if (!request_mem_region(base->start, resource_size(base),
				pdev->name)) {
		dev_err(&pdev->dev, "failed to request memory region\n");
		err = -ENXIO;
		goto out;
	}

	ndev = alloc_etherdev(sizeof(struct jzmac));
	if (!ndev) {
		err = -ENOMEM;
		goto out_free_region;
	}

	platform_set_drvdata(pdev, ndev);
	ndev->netdev_ops = &jzmac_netdev_ops;
	ndev->base_addr = base->start;
	ndev->irq = irq;
	ndev->watchdog_timeo = 2 * HZ;

	jzmac = netdev_priv(ndev);
	jzmac->iomem = ioremap(base->start, resource_size(base));
	jzmac->irq = irq;
	jzmac->mac_id = pdev->id;
	jzmac->ndev = ndev;
	jzmac->dev = &pdev->dev;

	jzmac->clk = clk_get(&pdev->dev, "mac");
	if (IS_ERR(jzmac->clk)) {
		err = PTR_ERR(jzmac->clk);
		goto out_free_netdev;
	}
	clk_enable(jzmac->clk);

	/* set MAC clock to 2.5MHz */
	for (i = 0; i < ARRAY_SIZE(mdc_diver); i++)
		if (clk_get_rate(jzmac->clk) / mdc_diver[i] < 2500000)
			break;
	if (i == ARRAY_SIZE(mdc_diver)) {
		i --;
		dev_warn(&pdev->dev, "mac clock over!\n");
	}
	reg_clr(jzmac, MAC_MII_MCFG, 0x7 << 2);
	reg_set(jzmac, MAC_MII_MCFG, i << 2);

	jzmac->buffer = jzmac_alloc_buffer(8);
	jzmac->rx_ring = ring_alloc(1, 0);
	jzmac->tx_ring = ring_alloc(0, 0);

	random_ether_addr(ndev->dev_addr);

#ifdef JZMAC_PHY_RESET_PIN
	/* FIXME, do you need phy reset? */
#endif
	jzmac_config_hw(jzmac);

	jzmac->mii_bus = mdiobus_alloc();
	if (jzmac->mii_bus == NULL) {
		dev_err(&pdev->dev, "failed to allocate mdiobus structure\n");
		err = -ENOMEM;
		goto out_clk_put;
	}

	jzmac->mii_bus->priv = jzmac;
	jzmac->mii_bus->read = jzmac_mdiobus_read;
	jzmac->mii_bus->write = jzmac_mdiobus_write;
	jzmac->mii_bus->reset = jzmac_mdiobus_reset;
	jzmac->mii_bus->name = "jzmac_eth_mii";
	snprintf(jzmac->mii_bus->id, MII_BUS_ID_SIZE, "%d", jzmac->mac_id);
	jzmac->mii_bus->irq = kmalloc(sizeof(int)*PHY_MAX_ADDR, GFP_KERNEL);
	if (jzmac->mii_bus->irq == NULL) {
		err = -ENOMEM;
		goto out_free_mdiobus;
	}

	for (i = 0; i < PHY_MAX_ADDR; i++)
		jzmac->mii_bus->irq[i] = PHY_POLL;

	err = mdiobus_register(jzmac->mii_bus);
	if (err) {
		dev_err(&pdev->dev, "failed to register MDIO bus\n");
		goto out_free_mdiobus_irq;
	}

	if ((err = jzmac_mii_probe(ndev)) != 0)
		goto out_unregister_mdiobus;

	netif_napi_add(ndev, &jzmac->napi, jzmac_poll, 16);

	err = register_netdev(ndev);
	if (err) {
		dev_err(&pdev->dev, "failed to register netdev.\n");
		goto out_unregister_mdiobus;
	}

	return 0;

out_unregister_mdiobus:
	mdiobus_unregister(jzmac->mii_bus);
out_free_mdiobus_irq:
	kfree(jzmac->mii_bus->irq);
out_free_mdiobus:
	mdiobus_free(jzmac->mii_bus);
out_clk_put:
	clk_put(jzmac->clk);
out_free_netdev:
	free_netdev(ndev);
out_free_region:
	release_region(base->start, resource_size(base));
out:
	return err;
}

static int __devexit jzmac_device_remove(struct platform_device *device)
{
	struct net_device *dev = platform_get_drvdata(device);

	/* FIXME, if you need remove MAC */
	unregister_netdev(dev);
	free_netdev(dev);
	platform_set_drvdata(device, NULL);

	return 0;
}


static struct platform_driver jzmac_driver = {
	.driver = {
		.name		= "jzmac",
		.owner		= THIS_MODULE,
	},
	.probe		= jzmac_probe,
	.remove		= __devexit_p(jzmac_device_remove),
};

static int __init jzmac_init_module(void)
{
	int err;

	err = platform_driver_register(&jzmac_driver);
	if (err)
		printk(KERN_ERR "JzMAC registration failed\n");

	return err;
}

static void __exit jzmac_exit_module(void)
{
	platform_driver_unregister(&jzmac_driver);
}

module_init(jzmac_init_module);
module_exit(jzmac_exit_module);

MODULE_DESCRIPTION("Ingenic jz4770 on Chip Ethernet driver");
MODULE_AUTHOR("unknown <unknown@ingenic.cn>");
MODULE_LICENSE("GPL v2");
