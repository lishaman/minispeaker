#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>

#include "jz47xx-pcm.h"
#include "jz47xx-i2s.h"
#include "include/dma.h"

#ifdef CONFIG_SOC_4780
#include "include/jz4780pdma.h"
#include "include/jz4780aic.h"
#include "include/jz4780cpm.h"
#endif

#ifdef CONFIG_SOC_4775
#include "include/jz4775pdma.h"
#include "include/jz4775aic.h"
#include "include/jz4775cpm.h"
#endif


#define I2S_RFIFO_DEPTH 32
#define I2S_TFIFO_DEPTH 64

#ifdef CONFIG_SND_SOC_JZ4770_ICDC
#define  CGM_AIC0       CGM_AIC
#else
#define  CGM_AIC0       CGM_AIC0
#endif

static int jz_i2s_debug = 0;
module_param(jz_i2s_debug, int, 0644);
#define I2S_DEBUG_MSG(msg...)			\
	do {					\
		if (jz_i2s_debug)		\
			printk("I2S: " msg);	\
	} while(0)

static struct jz47xx_dma_client jz47xx_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct jz47xx_dma_client jz47xx_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct jz47xx_pcm_dma_params jz47xx_i2s_pcm_stereo_out = {
	.client		= &jz47xx_dma_client_out,
	.channel	= DMA_ID_AIC_TX,
	.dma_addr	= AIC_DR,
	.dma_size	= 2,
};

static struct jz47xx_pcm_dma_params jz47xx_i2s_pcm_stereo_in = {
	.client		= &jz47xx_dma_client_in,
	.channel	= DMA_ID_AIC_RX,
	.dma_addr	= AIC_DR,
	.dma_size	= 2,
};

static int jz47xx_i2s_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	return 0;
}

static int jz47xx_i2s_set_dai_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		/* 1 : ac97 , 0 : i2s */
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
	        /* 0 : slave */
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		/* 1 : master */
		break;
	default:
		break;
	}

	return 0;
}

/*
* Set Jz47xx Clock source
*/
static int jz47xx_i2s_set_dai_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int is_recording = 0;
static int is_playing = 0;

static void jz47xx_snd_tx_ctrl(int on)
{
	I2S_DEBUG_MSG("enter %s, on = %d\n", __func__, on);
	if (on) {
		is_playing = 1;
                /* enable replay */
	        __i2s_enable_transmit_dma();
		__i2s_enable_replay();
		__i2s_enable();

	} else if (is_playing) {
		is_playing = 0;
		/* disable replay & capture */
		__i2s_disable_transmit_dma();
		__aic_write_tfifo(0x0);
		__aic_write_tfifo(0x0);
		while(!__aic_transmit_underrun());
		__i2s_disable_replay();
		__aic_clear_errors();

		if (!is_recording)
			__i2s_disable();
	}
}

static void jz47xx_snd_rx_ctrl(int on)
{
	I2S_DEBUG_MSG("enter %s, on = %d\n", __func__, on);
	if (on) {
		is_recording = 1;
                /* enable capture */
		__aic_flush_rfifo();
		mdelay(10);
		__i2s_enable_receive_dma();
		__i2s_enable_record();
		__i2s_enable();

	} else {
		is_recording = 0;
                /* disable replay & capture */
		__i2s_disable_record();
		__i2s_disable_receive_dma();
		if (!is_playing)
			__i2s_disable();
	}
}

static int jz47xx_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int channels = params_channels(params);

	I2S_DEBUG_MSG("enter %s, substream = %s\n",
		      __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	/* NOTE: when use internal codec, nothing to do with sample rate here.
	 * 	if use external codec and bit clock is provided by I2S controller, set clock rate here!!!
	 */

	/* set channel params */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		cpu_dai->playback_dma_data = &jz47xx_i2s_pcm_stereo_out;
		if (channels == 1) {
			__aic_enable_mono2stereo();
			__aic_out_channel_select(0);
		} else {
			__aic_disable_mono2stereo();
			__aic_out_channel_select(1);
		}
	} else
		cpu_dai->capture_dma_data = &jz47xx_i2s_pcm_stereo_in;


	/* set format */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_U8:
			__aic_enable_unsignadj();
			__i2s_set_oss_sample_size(8);
			break;
		case SNDRV_PCM_FORMAT_S8:
			__aic_disable_unsignadj();
			__i2s_set_oss_sample_size(8);
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
			__aic_disable_unsignadj();
			__i2s_set_oss_sample_size(16);
			break;
		case SNDRV_PCM_FORMAT_S24_3LE:
			__aic_disable_unsignadj();
			__i2s_set_oss_sample_size(24);
			break;
		}
	} else {
		int sound_data_width = 0;
		switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_U8:
			__aic_enable_unsignadj();
			__i2s_set_iss_sample_size(8);
			sound_data_width = 8;
			break;
		case SNDRV_PCM_FORMAT_S8:
			__aic_disable_unsignadj();
			__i2s_set_iss_sample_size(8);
			sound_data_width = 8;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
			__aic_disable_unsignadj();
			__i2s_set_iss_sample_size(16);
			sound_data_width = 16;
			break;
		case SNDRV_PCM_FORMAT_S24_3LE:
		default:
			__aic_disable_unsignadj();
			__i2s_set_iss_sample_size(24);
			sound_data_width = 24;
			break;
		}
		//__i2s_set_receive_trigger(((16 * 8) / sound_data_width) / 2);
		/* use 2 sample as trigger */
		__i2s_set_receive_trigger((sound_data_width / 8 * channels) * 2 / 2 - 1);
	}

	return 0;
}

static int jz47xx_i2s_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	I2S_DEBUG_MSG("enter %s, substream = %s cmd = %d\n",
		      __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture",
		      cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			jz47xx_snd_rx_ctrl(1);
		else
			jz47xx_snd_tx_ctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			jz47xx_snd_rx_ctrl(0);
		else
			jz47xx_snd_tx_ctrl(0);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void jz47xx_i2s_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	} else {
	}

	return;
}


/*
 *  * Get the xPLL clock
 *   */
unsigned int cpm_get_xpllout(src_clk sclk_name)
{
#ifdef CONFIG_SOC_4780
	unsigned int exclk = JZ_EXTAL;
	unsigned int xpll_ctrl = 0, pllout;
	unsigned int m, n, od;

	switch (sclk_name) {
        case SCLK_APLL:
                xpll_ctrl = INREG32(CPM_CPAPCR);
                break;
        case SCLK_MPLL:
                xpll_ctrl = INREG32(CPM_CPMPCR);
                break;
        case SCLK_EPLL:
                xpll_ctrl = INREG32(CPM_CPEPCR);
                break;
        case SCLK_VPLL:
                xpll_ctrl = INREG32(CPM_CPVPCR);
                break;
        default:
                printk("WARNING: can NOT get the %dpll's clock\n", sclk_name);
                break;
	}

	if (xpll_ctrl & CPXPCR_XPLLBP) {
		pllout = exclk;
		printk("here2\n");
		return pllout;
	}
	if (!(xpll_ctrl & CPXPCR_XPLLEN)) {
		pllout = 0;
		printk("here1\n");
		return 0;
	}


	m = ((xpll_ctrl & CPXPCR_XPLLM_MASK) >> CPXPCR_XPLLM_LSB) + 1;
	n = ((xpll_ctrl & CPXPCR_XPLLN_MASK) >> CPXPCR_XPLLN_LSB) + 1;
	od = ((xpll_ctrl & CPXPCR_XPLLOD_MASK) >> CPXPCR_XPLLOD_LSB) + 1;

	pllout = exclk * m / (n * od);
	printk("pllout == %d\n", pllout);
#else
        unsigned int cpxpcr = 0, pllout =  __cpm_get_extalclk();
	unsigned int m, n, no;

	switch (sclk_name) {
	case SCLK_APLL:
		cpxpcr = INREG32(CPM_CPAPCR);
		if ((cpxpcr & CPAPCR_EN) && (!(cpxpcr & CPAPCR_BP))) {
			m = ((cpxpcr & CPAPCR_M_MASK) >> CPAPCR_M_LSB) + 1;
			n = ((cpxpcr & CPAPCR_N_MASK) >> CPAPCR_N_LSB) + 1;
			no = 1 << ((cpxpcr & CPAPCR_OD_MASK) >> CPAPCR_OD_LSB);
			pllout = ((JZ_EXTAL) * m / (n * no));
		}
		break;
	case SCLK_MPLL:
		cpxpcr = INREG32(CPM_CPMPCR);
		if ((cpxpcr & CPMPCR_EN) && (!(cpxpcr & CPMPCR_BP))) {
			m = ((cpxpcr & CPMPCR_M_MASK) >> CPMPCR_M_LSB) + 1;
			n = ((cpxpcr & CPMPCR_N_MASK) >> CPMPCR_N_LSB) + 1;
			no = 1 << ((cpxpcr & CPMPCR_OD_MASK) >> CPMPCR_OD_LSB);
			pllout = ((JZ_EXTAL) * m / (n * no));
		}
		break;
	default:
		printk("WARNING: can NOT get the %dpll's clock\n", sclk_name);
		break;
	}
#endif

	return pllout;
}

/*
 *  * set I2S clock
 *   * for clk_sour  1: Apll, 0 : Epll
 *    */
static void set_i2s_clock(int clk_sour, unsigned int clk_hz)
{
        unsigned int extclk = __cpm_get_extalclk();
	unsigned int pllclk;
	unsigned int div;

#ifdef CONFIG_SOC_4780
	if(clk_hz == extclk){
		__cpm_select_i2sclk_exclk();
		REG_CPM_I2SCDR |= I2SCDR_CE_I2S;
		while(REG_CPM_I2SCDR & (I2SCDR_I2S_BUSY));
	}else{
		if(clk_sour){
			pllclk = cpm_get_xpllout(SCLK_APLL);
			printk("i2S from APLL and clock is %d\n", pllclk);
		}else{
			pllclk = cpm_get_xpllout(SCLK_EPLL);
			printk("i2S from EPLL and clock is %d\n", pllclk);
		}
		if((pllclk % clk_hz) * 2 >= clk_hz)
			div = (pllclk / clk_hz) + 1;
		else
			div = pllclk / clk_hz;
		printk("div==%d\n",div);

		if(clk_sour){
			OUTREG32(CPM_I2SCDR, I2SCDR_I2CS | I2SCDR_CE_I2S | (div - 1));
		}else{
			OUTREG32(CPM_I2SCDR, I2SCDR_I2CS | I2SCDR_CE_I2S | (div - 1) | I2SCDR_I2PCS);
		}

		while (INREG32(CPM_I2SCDR) & I2SCDR_I2S_BUSY){
			printk("Wait : I2SCDR stable\n");
			udelay(100);
		}
	}
#else
	if(clk_hz == (extclk / 2)){
		printk("i2S from exclk and clock is %d\n", (extclk/2));
		__cpm_select_i2sclk_exclk();
		REG_CPM_I2SCDR |= I2SCDR_CE_I2S;
		while (INREG32(CPM_I2SCDR) & I2SCDR_I2S_BUSY){
			printk("Wait : I2SCDR stable\n");
			udelay(100);
		}
	}else{
		if(clk_sour){
			pllclk = cpm_get_xpllout(SCLK_APLL);
			printk("i2S from APLL and clock is %d\n", pllclk);
		}else{
			pllclk = cpm_get_xpllout(SCLK_MPLL);
			printk("i2S from MPLL and clock is %d\n", pllclk);
		}
		if((pllclk % clk_hz) * 2 >= clk_hz)
			div = (pllclk / clk_hz) + 1;
		else
			div = pllclk / clk_hz;

		if(clk_sour)
			OUTREG32(CPM_I2SCDR, I2SCDR_I2CS | I2SCDR_CE_I2S | (div - 1));
		else
			OUTREG32(CPM_I2SCDR, I2SCDR_I2CS | I2SCDR_CE_I2S | I2SCDR_I2PCS | (div - 1));

		printk("div==%d\n",div);
		while (INREG32(CPM_I2SCDR) & I2SCDR_I2S_BUSY){
			printk("Wait : I2SCDR stable\n");
			udelay(100);
		}
	}
#endif
}

void cpm_start_clock(clock_gate_module module_name)
{
#ifdef CONFIG_SOC_4780
	unsigned int cgr_index, module_index;

	if (module_name == CGM_ALL_MODULE) {
		OUTREG32(CPM_CLKGR0, 0x0);
		OUTREG32(CPM_CLKGR1, 0x0);
		return;
	}

	cgr_index = module_name / 32;
	module_index = module_name % 32;
	switch (cgr_index) {
		case 0:
			CLRREG32(CPM_CLKGR0, 1 << module_index);
			switch(module_index) {
				case 3:
				case 11:
				case 12:
					SETREG32(CPM_MSC0CDR, MSCCDR_MPCS_MPLL);
					break;
				default:
					;
			}
			break;
		case 1:
			CLRREG32(CPM_CLKGR1, 1 << module_index);
			break;
		default:
			printk("WARNING: can NOT start the %d's clock\n",
					module_name);
			break;
	}
#else
	if (module_name == CGM_ALL_MODULE) {
		OUTREG32(CPM_CLKGR0, 0x0);
		return;
	}

	if (module_name & 0x1f) {
		CLRREG32(CPM_CLKGR0, 1 << module_name);
        } else {
		printk("WARNING: can NOT start the %d's clock\n", module_name);
	}
#endif
}


static int jz47xx_i2s_probe(struct snd_soc_dai *dai)
{

	cpm_start_clock(CGM_AIC0);
	/* Select exclk as i2s clock */
	REG_AIC_I2SCR |= AIC_I2SCR_ESCLK;

#ifdef  CONFIG_SND_SOC_JZ4770_ICDC
	cpm_set_clock(CGU_I2SCLK, JZ_EXTAL);
#else
	set_i2s_clock(0, 12000000);
#endif
	printk("start set AIC register....\n");

	__i2s_disable();
	__aic_disable_transmit_dma();
	__aic_disable_receive_dma();
	__i2s_disable_record();
	__i2s_disable_replay();
	__i2s_disable_loopback();

	__i2s_internal_codec();
	__i2s_as_slave();
	__i2s_select_i2s();
	__aic_select_i2s();
	__aic_play_lastsample();
	__i2s_set_transmit_trigger(I2S_TFIFO_DEPTH / 4);
	__i2s_set_receive_trigger(I2S_RFIFO_DEPTH / 4);
	//__i2s_send_rfirst();

	__aic_write_tfifo(0x0);
	__aic_write_tfifo(0x0);
	__i2s_enable_replay();
	__i2s_enable();
	mdelay(1);

	__i2s_disable_replay();
	__i2s_disable();

	jz47xx_snd_tx_ctrl(0);
	jz47xx_snd_rx_ctrl(0);

	return 0;
}

#ifdef CONFIG_PM
static int jz47xx_i2s_suspend(struct snd_soc_dai *dai)
{
	return 0;
}

static int jz47xx_i2s_resume(struct snd_soc_dai *dai)
{
	return 0;
}

#else
#define jz47xx_i2s_suspend	NULL
#define jz47xx_i2s_resume	NULL
#endif

#define JZ47xx_I2S_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |	\
			  SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | 	\
			  SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |	\
			  SNDRV_PCM_RATE_48000  | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

static struct snd_soc_dai_ops jz47xx_i2s_dai_ops = {
	.startup 	= jz47xx_i2s_startup,
	.shutdown 	= jz47xx_i2s_shutdown,
	.trigger 	= jz47xx_i2s_trigger,
	.hw_params 	= jz47xx_i2s_hw_params,
	.set_fmt 	= jz47xx_i2s_set_dai_fmt,
	.set_sysclk 	= jz47xx_i2s_set_dai_sysclk,
};

//#define JZ_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8  | SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE)
#define JZ_I2S_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE)

static struct snd_soc_dai_driver jz47xx_i2s_dai = {
		.probe   = jz47xx_i2s_probe,
		.suspend = jz47xx_i2s_suspend,
		.resume  = jz47xx_i2s_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = JZ47xx_I2S_RATES,
			.formats = JZ_I2S_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = JZ47xx_I2S_RATES,
			.formats = JZ_I2S_FORMATS,
		},
		.ops = &jz47xx_i2s_dai_ops,
};

static  __devinit int  jz47xx_i2s_dev_probe(struct platform_device *pdev)
{
	return snd_soc_register_dai(&pdev->dev, &jz47xx_i2s_dai);
}

static __devexit int jz47xx_i2s_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
	return 0;
}

static struct platform_driver jz47xx_i2s_plat_driver = {
	.probe  = jz47xx_i2s_dev_probe,
	.remove = jz47xx_i2s_dev_remove,
	.driver = {
		.name = "jz47xx-i2s",
		.owner = THIS_MODULE,
	},
};


static int __init jz47xx_i2s_init(void)
{
        return platform_driver_register(&jz47xx_i2s_plat_driver);
}
module_init(jz47xx_i2s_init);

static void __exit jz47xx_i2s_exit(void)
{
	platform_driver_unregister(&jz47xx_i2s_plat_driver);
}
module_exit(jz47xx_i2s_exit);


MODULE_AUTHOR("Lutts Wolf <slcao@ingenic.cn>");
MODULE_DESCRIPTION("jz47xx I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz47xx-i2s");

