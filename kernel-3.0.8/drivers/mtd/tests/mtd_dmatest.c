/*
 * mtd_dmatest.c
 *
 *  Created on: Apr 26, 2013
 *      Author: Fighter Sun <wanmyqawdr@126.com>
 */

#include <asm/div64.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/dmaengine.h>
#include <linux/vmalloc.h>

#include <asm/cache.h>

#include <mach/jzdma.h>

#define BUF_SIZE (8 * 1024)

static uint8_t *dst_buf;
static uint8_t *src_buf;

struct dma_pipe{
	struct dma_chan *chan;

	enum jzdma_type type;
} dma_test_pipe;

struct device dev;

static bool dma_filter(struct dma_chan *chan, void *filter_param)
{
	struct dma_pipe *dma = (struct dma_pipe *)filter_param;

	return (int)chan->private == (int)dma->type;
}

#define GET_PHYADDR(a) ({	\
	unsigned int v;	\
	if (unlikely((int)(a) & 0x40000000)) {	\
		v = page_to_phys(	\
				vmalloc_to_page((const void *)(a))) |	\
				((int)(a) & ~PAGE_MASK);	\
	} else											\
		v = ((_ACAST32_((int)(a))) & 0x1fffffff);	\
	v;		        								\
})

static int __init test_init(void)
{
	/*
	 * buf
	 */
	dst_buf = vmalloc(BUF_SIZE);
	src_buf = kzalloc(BUF_SIZE, GFP_KERNEL);

	/* cache inv */
	dma_sync_single_for_device(&dev, CPHYSADDR(src_buf),
			BUF_SIZE, DMA_TO_DEVICE);

	BUG_ON(!dst_buf || !src_buf);

	printk("we are started from here. \n");
	printk("src_buf address: 0x%p\n", src_buf);
	printk("dst_buf address: 0x%p\n", dst_buf);

	memset(dst_buf, 0, BUF_SIZE);

	/*
	 * uncached address
	 */
	src_buf = (uint8_t *)CKSEG1ADDR(src_buf);

	/*
	 * dma channel
	 */
	{	dma_cap_mask_t mask;

		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);

		dma_test_pipe.type = JZDMA_REQ_AUTO;
		dma_test_pipe.chan = dma_request_channel(mask,
				dma_filter, &dma_test_pipe);
		if (!dma_test_pipe.chan) {
			pr_err("dma_test: failed to request dma channel.\n");
			goto err_kfree;
		}
	}


	/*
	 * test loop
	 */
	{
		uint64_t rnd_seed = 0;
		static struct rnd_state rnd_state;

		while (1) {
			/*
			 * start from seed
			 */
			prandom_seed_state(&rnd_state, rnd_seed);

			/*
			 * fill src_buf with random data
			 */
			prandom_bytes_state(&rnd_state, src_buf, BUF_SIZE);


			/*
			 * DMA : src_buf -> dst_buf
			 */

			{
				struct jzdma_channel *dmac;
				unsigned long timeo;

				dmac = to_jzdma_chan(dma_test_pipe.chan);

				/*
				 * step1. configure dma channel by direct IO access
				 */
				writel(GET_PHYADDR(dst_buf), dmac->iomem + CH_DTA);

				writel(CPHYSADDR(src_buf), dmac->iomem + CH_DSA);

				writel(DCM_DAI | DCM_SAI | (7 << 8), dmac->iomem + CH_DCM);

				writel(BUF_SIZE, dmac->iomem + CH_DTC);
				writel(dmac->type, dmac->iomem + CH_DRT);


				/* step2. cache inv */
				dma_sync_single_for_device(&dev, GET_PHYADDR(dst_buf), BUF_SIZE, DMA_TO_DEVICE);

				jzdma_dump(dma_test_pipe.chan);
				/*
				 * step3. start no descriptor dma transfer
				 */
				writel(BIT(31) | BIT(0), dmac->iomem + CH_DCS);

				/*
				 * step4. wait for dma transfer done
				 */
				timeo = jiffies + msecs_to_jiffies(1000);
				do {
					if (readl(dmac->iomem + CH_DCS) & DCS_TT)
						break;

					cond_resched();
				} while (time_before(jiffies, timeo));

				if (!(readl(dmac->iomem + CH_DCS) & DCS_TT)) {
					writel(0, dmac->iomem + CH_DCS);

					WARN(1, "Timeout when DMA read\n");
					goto err_release_dma;
				}

				/*
				 * step5. stop dma transfer
				 */
				writel(0, dmac->iomem + CH_DCS);
			}

			/*
			 * verify
			 */
			{
				int i;
				for (i = 0; i < BUF_SIZE; i++) {
					if (src_buf[i] != dst_buf[i]) {
						pr_err("dma_test: a mismatch found:"
								" (src_buf[%d] = 0x%x) != (dst_buf[%d] = 0x%x)"
								" at times: %llu\n", i, src_buf[i],
								i, dst_buf[i], rnd_seed + 1);

						pr_info("dma_tets: double check from uncached address\n");
						for (i = 0; i < BUF_SIZE; i++) {
							uint8_t *uncached_dst_buf = (uint8_t *)CKSEG1ADDR(dst_buf);
							if (src_buf[i] != uncached_dst_buf[i]) {
								pr_err("dma_test: a mismatch found:"
										" (src_buf[%d] = 0x%x) != (uncached_dst_buf[%d] = 0x%x)"
										" at times: %llu\n", i, src_buf[i],
										i, uncached_dst_buf[i], rnd_seed + 1);

								goto err_release_dma;
							}
						}

						goto err_release_dma;
					}
				}
			}

			/*
			 * next random seed
			 */
			rnd_seed++;
		}
	}

err_release_dma:
	dma_release_channel(dma_test_pipe.chan);

err_kfree:
	kfree((void *)CKSEG0ADDR(src_buf));
	kfree(dst_buf);

	return -EINVAL;
}

module_init(test_init);
MODULE_LICENSE("GPL");
