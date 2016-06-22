/**
 * xb_snd_dmic.c
 *
 * jbbi <jbbi@ingenic.cn>
 *
 * 24 APR 2012
 *
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/string.h>
#include <linux/sound.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <linux/switch.h>
#include <linux/dma-mapping.h>
#include <linux/soundcard.h>
#include <linux/suspend.h>
#include <linux/wait.h>
#include <mach/jzdma.h>
#include <mach/jzsnd.h>
#include <soc/irq.h>
#include <soc/base.h>
#include "xb47xx_dmic.h"
#include "codecs/jz4780_codec.h"
#include <linux/delay.h>
/**
 * global variable
 **/
void volatile __iomem *volatile dmic_iomem;
static volatile int jz_switch_state = 0;
static struct dsp_endpoints dmic_endpoints;
static volatile bool dmic_is_incall_state = false;
static LIST_HEAD(codecs_head);
bool dmic_is_incall(void);

static int dmic_global_init(struct platform_device *pdev);

struct dsp_node *dmic_node = NULL;

int dmic_prepare_ok;
unsigned long long dmic_timer[20];

//#define DMIC_IRQ
static struct jz_dmic {
	int dmic_irq;
	wait_queue_head_t dmic_wait;
	spinlock_t dmic_irq_lock;
	atomic_t trigger_done;
} *cur_dmic;

struct codec_info_dmic {
	struct list_head list;
	char *name;
	unsigned long record_rate;
	unsigned long replay_rate;
	int record_codec_channel;
	int replay_codec_channel;
	int record_format;
	int replay_format;
	enum codec_mode codec_mode;
	unsigned long codec_clk;
	int (*codec_ctl)(unsigned int cmd, unsigned long arg);
	struct dsp_endpoints *dsp_endpoints;
};

static int user_record_volume = 50;
/*##################################################################*\
 | dump
\*##################################################################*/

 void dump_dmic_reg(void)
{
	int i;
	unsigned long reg_addr[] = {
		DMICCR0, DMICGCR, DMICIMR, DMICINTCR, DMICTRICR, DMICTHRH,
		DMICTHRL, DMICTRIMMAX, DMICTRINMAX, DMICDR ,DMICFTHR, DMICFSR, DMICCGDIS
	};

	for (i = 0;i < 13; i++) {
		printk("dmic reg0x%x = 0x%x.\n",
		(unsigned int)reg_addr[i], dmic_read_reg(reg_addr[i]));
	}
	printk("intc = 0x%x.\n",*((volatile unsigned int *)(0xb0001000)));
}

/*######DMICTRIMMAX############################################################*\
 |* suspDMICTRINMAXand func
\*##################################################################*/
static int dmic_suspend(struct platform_device *, pm_message_t state);
static int dmic_resume(struct platform_device *);
static void dmic_shutdown(struct platform_device *);

/*##################################################################*\
  |* codec control
\*##################################################################*/
/**
 * register the codec
 **/

int dmic_register_codec(char *name, void *codec_ctl,unsigned long codec_clk,enum codec_mode mode)
{
	struct codec_info_dmic *tmp = kmalloc(sizeof(struct codec_info_dmic), GFP_KERNEL);
	if ((name != NULL) && (codec_ctl != NULL)) {
		tmp->name = name;
		tmp->codec_ctl = codec_ctl;
		tmp->codec_clk = codec_clk;
		tmp->codec_mode = mode;
		tmp->dsp_endpoints = &dmic_endpoints;
		list_add_tail(&tmp->list, &codecs_head);
		return 0;
	} else {
		return -1;
	}
}
EXPORT_SYMBOL(dmic_register_codec);

bool dmic_is_incall(void)
{
	return dmic_is_incall_state;
}

/*##################################################################*\
|* dev_ioctl
\*##################################################################*/
static int dmic_set_fmt(unsigned long *format,int mode)
{

	int ret = 0;
	struct dsp_pipe *dp = NULL;

    /*
	 * The value of format reference to soundcard.
	 * AFMT_MU_LAW      0x00000001
	 * AFMT_A_LAW       0x00000002
	 * AFMT_IMA_ADPCM   0x00000004
	 * AFMT_U8			0x00000008
	 * AFMT_S16_LE      0x00000010
	 * AFMT_S16_BE      0x00000020
	 * AFMT_S8			0x00000040
	 */
	debug_print("dmic set format = %d",*format);
	switch (*format) {
	case AFMT_S16_LE:
		break;
	case AFMT_S16_BE:
		break;
	default :
		printk("DMIC: dmic only support 16bit width, 0x%x.\n",(unsigned int)*format);
		return -EINVAL;
	}

	if (mode & CODEC_RMODE) {
		dp = dmic_endpoints.in_endpoint;
		dp->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		ret |= NEED_RECONF_TRIGGER;
		ret |= NEED_RECONF_DMA;
	}
	if (mode & CODEC_WMODE) {
		printk("DMIC: do not support replay!\n");
		ret = -1;
	}

	return ret;
}

/*##################################################################*\
|* filter opt
\*##################################################################*/
static void dmic_set_filter(int mode , uint32_t channels)
{
	struct dsp_pipe *dp = NULL;

	if (mode & CODEC_RMODE)
		dp = dmic_endpoints.in_endpoint;
	else
		return;

	if (channels == 1) {
		dp->filter = convert_16bits_stereo2mono;
	} else if (channels == 3){
		dp->filter = convert_32bits_2_20bits_tri_mode;
	} else if (channels == 2){
		dp->filter = NULL;
	} else {
		dp->filter = NULL;
	}
}

#ifdef DMIC_IRQ

static void jz_dmic_trigger_done_reset() {
	atomic_set(&cur_dmic->trigger_done, 0);
}
#endif


int dmic_set_channel(int* channel,int mode)
{
	int ret = 0;
	debug_print("dmic set channels = %d",*channel);
	if (mode & CODEC_RMODE) {
		switch(*channel){
		case 1:
			__dmic_select_chnum(0);
			break;
		case 2:
			__dmic_select_chnum(1);
			break;
		case 3:
			__dmic_select_chnum(2);
			break;
		case 4:
			__dmic_select_chnum(3);
			break;
		default:
			printk("Dmic don't support more than 4 channels.\n");
			ret = -1;
		}
		return ret;
	}
	if (mode & CODEC_WMODE) {
		return -1;
	}
	return ret;
}



int dmic_set_rate(unsigned long *rate,int mode)
{
	int ret = 0;

	if (mode & CODEC_WMODE) {
		/*************************************************\
		|* WARING:dmic have only one mode,               *|
		|* So we should not to care write mode.          *|
		\*************************************************/
		printk("dmic do not support replay!\n");
		return -EPERM;
	}
	if (mode & CODEC_RMODE) {
		if (*rate == 8000){
			debug_print("Dmic sample rate %ld", *rate);
			__dmic_select_8k_mode();
		}
		else if (*rate == 16000){
			debug_print("Dmic sample rate %ld", *rate);
			__dmic_select_16k_mode();
		}
		else if (*rate == 48000){
			debug_print("Dmic sample rate %ld", *rate);
			__dmic_select_48k_mode();
		}
		else{
			printk("DMIC: unsupport sample rate: %ld, fix to 48000.\n", *rate);
			__dmic_select_48k_mode();
			*rate = 48000;
			ret = -1;
		}
	}

	return ret;
}

void dmic_set_trigger(int mode)
{

	if (mode & CODEC_WMODE) {
		printk("dmic do not support replay!\n");
	}
	if (mode &CODEC_RMODE) {
		__dmic_set_request(8);
	}

	return;
}

int dmic_enable(int mode)
{
	unsigned long record_format = 16;
	/*int record_channel = DEFAULT_RECORD_CHANNEL;*/
	if (!(mode & CODEC_RMODE))
		return -1;

	__dmic_disable_all_int();
	__dmic_reset();
	while(__dmic_get_reset());

	dmic_set_fmt(&record_format,mode);

	dmic_is_incall_state = false;
	return 0;
}

int dmic_disable(int mode)
{
	__dmic_disable();
	return 0;
}

int dmic_dma_enable(int mode)		//CHECK
{
	if (mode & CODEC_RMODE) {
		__dmic_set_request(8);
		__dmic_enable_rdms();
		__dmic_clear_tur();
		__dmic_enable();
	}
	if (mode & CODEC_WMODE) {
		printk("DMIC: dmic do not support replay\n");
		return -1;
	}

	return 0;
}

int dmic_dma_disable(int mode)		//CHECK seq dma and func
{
	if (mode & CODEC_RMODE) {
		__dmic_disable_rdms();
	}
	else
		return -1;

	return 0;
}

int dmic_get_fmt_cap(unsigned long *fmt_cap,int mode)
{
	unsigned long dmic_fmt_cap = 0;
	if (mode & CODEC_WMODE) {
		return -1;
	}
	if (mode & CODEC_RMODE) {
		dmic_fmt_cap |= AFMT_S16_LE|AFMT_S16_BE;
	}

	if (*fmt_cap == 0)
		*fmt_cap = dmic_fmt_cap;
	else
		*fmt_cap &= dmic_fmt_cap;

	return 0;
}


int dmic_get_fmt(unsigned long *fmt, int mode)
{
	if (mode & CODEC_WMODE)
		return -1;
	if (mode & CODEC_RMODE)
		*fmt = AFMT_S16_LE;

	return 0;
}

 void dmic_dma_need_reconfig(int mode)
{
	struct dsp_pipe	*dp = NULL;

	if (mode & CODEC_RMODE) {
		dp = dmic_endpoints.in_endpoint;
		dp->need_reconfig_dma = true;
	}
	if (mode & CODEC_WMODE) {
		printk("DMIC: dmic do not support replay\n");
	}

	return;
}


int dmic_set_device(unsigned long device)
{
	return 0;
}

static int dmic_set_record_volume(int *val)
{
	/* The dmic record gain range we used is from 0dB ~ +48dB */
        if (*val >100 || *val < 0) {
                *val = user_record_volume;
                printk("Dmic record volume out of range [ 0 ~ 100 ].\n");
        }

	__dmic_set_gm(*val/6);

        user_record_volume = *val;
        return *val;
}

/********************************************************\
 * dev_ioctl
\********************************************************/
long dmic_ioctl(unsigned int cmd, unsigned long arg)
{
	long ret = 0;


	switch (cmd) {
	case SND_DSP_ENABLE_REPLAY:
		/* enable dmic replay */
		/* set dmic default record format, channels, rate */
		/* set default replay route */
		printk("dmic do not support replay!\n");
		ret = -1;
		break;

	case SND_DSP_DISABLE_REPLAY:
		printk("dmic do not support replay!\n");
		ret = -1;
		/* disable dmic replay */
		break;

	case SND_DSP_ENABLE_RECORD:
		/* enable dmic record */
		/* set dmic default record format, channels, rate */
		/* set default record route */
		ret = dmic_enable(CODEC_RMODE);
		break;

	case SND_DSP_DISABLE_RECORD:
		/* disable dmic record */
		ret = dmic_disable(CODEC_RMODE);
		break;

	case SND_DSP_ENABLE_DMA_RX:
		ret = dmic_dma_enable(CODEC_RMODE);
		break;

	case SND_DSP_DISABLE_DMA_RX:
		ret = dmic_dma_disable(CODEC_RMODE);
		break;

	case SND_DSP_ENABLE_DMA_TX:
		printk("dmic not support replay!\n");
		ret = -1;
		break;

	case SND_DSP_DISABLE_DMA_TX:
		ret = -1;
		printk("dmic not support replay!\n");
		break;

	case SND_DSP_SET_REPLAY_RATE:
		ret = -1;
		printk("dmic not support replay!\n");
		break;

	case SND_DSP_SET_RECORD_RATE:
		ret = dmic_set_rate((unsigned long *)arg,CODEC_RMODE);
		break;

	case SND_DSP_GET_REPLAY_RATE:
		ret = -1;
		printk("dmic not support replay!\n");
		break;

	case SND_DSP_GET_RECORD_RATE:
		ret = 0;
		break;


	case SND_DSP_SET_REPLAY_CHANNELS:
		/* set replay channels */
		printk("dmic not support replay!\n");
		ret = -1;
		break;

	case SND_DSP_SET_RECORD_CHANNELS:
		dmic_set_channel((int *)arg, CODEC_RMODE);
		/* set record channels */
		break;

	case SND_DSP_GET_REPLAY_CHANNELS:
		printk("dmic not support record!\n");
		ret = -1;
		break;

	case SND_DSP_GET_RECORD_CHANNELS:
		ret = 0;
		break;

	case SND_DSP_GET_REPLAY_FMT_CAP:
		ret = -1;
		printk("dmic not support replay!\n");
		/* return the support replay formats */
		break;

	case SND_DSP_GET_REPLAY_FMT:
		/* get current replay format */
		ret = -1;
		printk("dmic not support record!\n");
		break;

	case SND_DSP_SET_REPLAY_FMT:
		/* set replay format */
		printk("dmic not support replay!\n");
		ret = -1;
		break;

	case SND_DSP_GET_RECORD_FMT_CAP:
		/* return the support record formats */
		ret = 0;
		ret = dmic_get_fmt_cap((unsigned long *)arg,CODEC_RMODE);
		break;

	case SND_DSP_GET_RECORD_FMT:
		ret = 0;
		ret = dmic_get_fmt((unsigned long *)arg,CODEC_RMODE);
		/* get current record format */

		break;

	case SND_DSP_SET_RECORD_FMT:
		/* set record format */
		ret = dmic_set_fmt((unsigned long *)arg,CODEC_RMODE);
		if (ret < 0)
			break;
	//	[> if need reconfig the trigger, reconfig it <]
		if (ret & NEED_RECONF_TRIGGER)
			dmic_set_trigger(CODEC_RMODE);
	//	[> if need reconfig the dma_slave.max_tsz, reconfig it and
	//	   set the dp->need_reconfig_dma as true <]
		if (ret & NEED_RECONF_DMA)
			dmic_dma_need_reconfig(CODEC_RMODE);
		ret = 0;

		break;

	case SND_MIXER_DUMP_REG:
		dump_dmic_reg();
		break;
	case SND_MIXER_DUMP_GPIO:
		break;

	case SND_DSP_SET_STANDBY:
		break;

	case SND_DSP_SET_DEVICE:
		ret = dmic_set_device(arg);
		break;
	case SND_DSP_GET_HP_DETECT:
		ret = 0;
		break;
	case SND_DSP_SET_RECORD_VOL:
		ret = dmic_set_record_volume((int *)arg);
		break;
	case SND_DSP_SET_REPLAY_VOL:
		printk("dmic not support replay!\n");
		ret = -1;
		break;
	case SND_DSP_SET_MIC_VOL:
		__dmic_set_gm(arg);
		break;
	case SND_DSP_CLR_ROUTE:
		break;
	case SND_DSP_DEBUG:
		ret = 0;
		break;
	default:
		printk("SOUND_ERROR: %s(line:%d) unknown command!",
				__func__, __LINE__);
		ret = -EINVAL;
	}

	return ret;
}

#ifdef DMIC_IRQ
void jz_dmic_trigger_done() {
	atomic_set(&cur_dmic->trigger_done, 1);
	wake_up(&cur_dmic->dmic_wait);
}
#endif

/*##################################################################*\
|* functions
\*##################################################################*/
//#ifdef DMIC_IRQ
irqreturn_t dmic_irq_handler(int irq, void *dev_id)
{
	irqreturn_t ret = IRQ_HANDLED;

	__dmic_disable_all_int();
	if (((*(volatile unsigned int *)0xb002100c) & (1 << 1))){
		printk("over run happen\n");
		*(volatile unsigned int *)0xb002100c = 0x3f;
	}
	return ret;
}
//#endif

int dmic_init_pipe(struct dsp_pipe **dp , enum dma_data_direction direction,unsigned long iobase)
{
	if (*dp != NULL || dp == NULL)
		return 0;
	*dp = kmalloc(sizeof(struct dsp_pipe), GFP_KERNEL);
	if (*dp == NULL) {
		return -ENOMEM;
	}

	(*dp)->dma_config.direction = direction;
	(*dp)->dma_config.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;//unpack_dis = 1
	(*dp)->dma_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	(*dp)->dma_type = JZDMA_REQ_I2S1;
	(*dp)->fragsize = FRAGSIZE_S;
	(*dp)->fragcnt = FRAGCNT_B;

	if (direction == DMA_FROM_DEVICE) {
		(*dp)->dma_config.src_maxburst = 32;
		(*dp)->dma_config.dst_maxburst = 32;
		(*dp)->dma_config.src_addr = iobase + DMICDR;
		(*dp)->dma_config.dst_addr = 0;
	} else	if (direction == DMA_TO_DEVICE) {
		(*dp)->dma_config.src_maxburst = 32;
		(*dp)->dma_config.dst_maxburst = 32;
		(*dp)->dma_config.dst_addr = iobase + AICDR;
		(*dp)->dma_config.src_addr = 0;
	} else
		return -1;

	return 0;
}

int dmic_global_init(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *dmic_resource = NULL;
	struct dsp_pipe *dmic_pipe_out = NULL;
	struct dsp_pipe *dmic_pipe_in = NULL;
	struct clk *dmic_clk = NULL;

	cur_dmic = kmalloc(sizeof(struct jz_dmic), GFP_KERNEL);

	dmic_prepare_ok = 0;
	dmic_resource = platform_get_resource(pdev,IORESOURCE_MEM,0);
	if (dmic_resource == NULL) {
		printk("%s dmic_resource get failed!\n", __func__);
		return -1;
	}

	init_waitqueue_head(&cur_dmic->dmic_wait);
	spin_lock_init(&cur_dmic->dmic_irq_lock);
	atomic_set(&cur_dmic->trigger_done, 0);
	/* map io address */
	if (!request_mem_region(dmic_resource->start, resource_size(dmic_resource), pdev->name)) {
		printk("%s request mem region failed!\n", __func__);
		return -EBUSY;
	}
	dmic_iomem = ioremap(dmic_resource->start, resource_size(dmic_resource));
	if (!dmic_iomem) {
		printk("%s ioremap failed!\n", __func__);
		ret =  -ENOMEM;
		goto __err_ioremap;
	}

	ret = dmic_init_pipe(&dmic_pipe_out,DMA_TO_DEVICE,dmic_resource->start);
	if (ret < 0) {
		printk("%s init write pipe failed!\n", __func__);
		goto __err_init_pipeout;
	}
	ret = dmic_init_pipe(&dmic_pipe_in,DMA_FROM_DEVICE,dmic_resource->start);
	if (ret < 0) {
		printk("%s init read pipe failed!\n", __func__);
		goto __err_init_pipein;
	}

	dmic_endpoints.out_endpoint = dmic_pipe_out;
	dmic_endpoints.in_endpoint = dmic_pipe_in;

	/*dmic_endpoints->in_endpoint->filter = convert_32bits_stereo2_16bits_mono;*/
	//printk("dp in_endpoint filter set!\n");


	/*request aic clk */
	dmic_clk = clk_get(&pdev->dev, "dmic");
	if (IS_ERR(dmic_clk)) {
		dev_err(&pdev->dev, "----> aic clk_get failed\n");
		goto __err_aic_clk;
	}
	clk_enable(dmic_clk);
#ifdef DMIC_IRQ
	/* request irq */
	dmic_resource = platform_get_resource(pdev,IORESOURCE_IRQ,0);
	if (dmic_resource == NULL) {
		printk("----> get resource error\n");
		ret = -1;
		goto __err_irq;
	}

	ret = request_irq(IRQ_DMIC, dmic_irq_handler,
					  IRQF_DISABLED, "dmic_irq", NULL);
	if (ret < 0) {
		printk("----> request irq error\n");
		goto __err_irq;
	}
#endif

	__dmic_reset();
	while(__dmic_get_reset());
	__dmic_select_chnum(2);
	__dmic_select_8k_mode();
	__dmic_enable_hpf();
	__dmic_set_gm(10);
	__dmic_disable_all_int();
	__dmic_enable_rdms();
	__dmic_select_stereo();
	__dmic_enable_pack();
	__dmic_disable_unpack_dis();
	__dmic_disable_switch_rl();
	__dmic_enable_lp_mode();
	__dmic_enable_tri();
	__dmic_set_request(16);
	__dmic_enable_hpf2();
	__dmic_set_thr_high(32);
	__dmic_set_thr_low(16);

	__dmic_disable();
	printk("dmic init success.\n");
	return  0;

__err_irq:
__err_aic_clk:
	clk_put(dmic_clk);
__err_init_pipein:
	kfree(dmic_pipe_out);
__err_init_pipeout:
	iounmap(dmic_iomem);
__err_ioremap:
	release_mem_region(dmic_resource->start,resource_size(dmic_resource));
	return ret;
}

int dmic_init(struct platform_device *pdev)
{
	int ret = 0;
	ret = dmic_global_init(pdev);
	if (ret)
		printk("dmic init error!\n");

	return ret;
}

void dmic_shutdown(struct platform_device *pdev)
{
	/* close dmic and current codec */
	__dmic_disable();
	return;
}

 int dmic_suspend(struct platform_device *pdev, pm_message_t state)
{
	__dmic_disable();
	return 0;
}

int dmic_resume(struct platform_device *pdev)
{
	__dmic_enable();
	return 0;
}

struct snd_dev_data dmic_data = {
	.dev_ioctl	   	= dmic_ioctl,
	.ext_data		= &dmic_endpoints,
	.minor			= SND_DEV_DSP3,
	.init			= dmic_init,
	.shutdown		= dmic_shutdown,
	.suspend		= dmic_suspend,
	.resume			= dmic_resume,
};

struct snd_dev_data snd_mixer3_data = {
	.dev_ioctl	   	= dmic_ioctl,
	.minor			= SND_DEV_MIXER3,
};

int __init init_dmic(void)
{
	return 0;
}
module_init(init_dmic);
