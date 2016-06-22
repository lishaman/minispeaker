#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/vmalloc.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>
#include <sound/soc-dai.h>
#include <soc/irq.h>

#include "jz4780_icdc.h"
#include "../xburst/include/jz4780aic.h"
#include "../xburst/include/dma.h"

static int jz_icdc_debug = 0;
module_param(jz_icdc_debug, int, 0644);

#define USE_AGC_MIXER_FUNC  0

#define DEBUG_MSG(msg...)			\
	do {					\
		if (jz_icdc_debug)		\
			printk("ICDC: " msg);	\
	} while(0)

#define debug  1

/* codec private data */
struct jz_icdc_priv {
	u8 reg_cache[JZ_ICDC_MAX_NUM];
};

/* old variable, no use now */
static int hpout_enable = 0;
//static int lineout_enable = 0;
static int bypass_to_hp = 0;
static int bypass_to_lineout = 0;

static struct work_struct dlv_irq_work;
static spinlock_t dlv_irq_lock;
static struct semaphore *g_dlv_sem = NULL;

static struct snd_soc_codec *jz4780_codec = NULL;
static struct jz_icdc_priv jz4780_priv;

/*
 * read register cache
 */
static inline unsigned int jz_icdc_read_reg_cache(struct snd_soc_codec *codec,
						 unsigned int reg)
{
	u8 *cache = codec->reg_cache;
#if debug
	if (ICDC_REG_IS_MISSING(reg))
		return 0;
#endif
	if (reg >= JZ_ICDC_MAX_NUM)
		return -1;

	return cache[reg];
}

/*
 * write register cache
 */
static inline void jz_icdc_write_reg_cache(struct snd_soc_codec *codec,
					  unsigned int reg, unsigned int value)
{
	u8 *cache = codec->reg_cache;

#if debug
	if (ICDC_REG_IS_MISSING(reg))
		return;
#endif
	if (reg >= JZ_ICDC_MAX_NUM)
		return;

	cache[reg] = (u8)value;
}



static inline u8 jz_icdc_read_reg_hw(struct snd_soc_codec *codec, unsigned int reg)
{
        volatile int val;

#if debug
	if (ICDC_REG_IS_MISSING(reg))
		return 0;
#endif
	if (reg >= JZ_ICDC_MAX_REGNUM)
		return 0;

        while (__icdc_rgwr_ready()) {
                ;//nothing...
        }

        __icdc_set_addr(reg);

	/* wait 4+ cycle */
	val = __icdc_get_value();
        val = __icdc_get_value();
        val = __icdc_get_value();
        val = __icdc_get_value();
        val = __icdc_get_value();
        val = __icdc_get_value();

	val = __icdc_get_value();

	return (u8)val;
}

static inline void jz_icdc_write_reg_hw(unsigned char reg, u8 value)
{
#if debug
	if (ICDC_REG_IS_MISSING(reg))
		return;
#endif
        while (__icdc_rgwr_ready());

        REG_ICDC_RGADW = ICDC_RGADW_RGWR | ((reg << ICDC_RGADW_RGADDR_LSB) | value);

        while (__icdc_rgwr_ready());
}

static int jz_icdc_hw_write(void * unused, const char* data, int num) {
	jz_icdc_write_reg_hw(data[0], data[1]);

	return 2;
}

/*
 * write to the register space
 */
static int jz_icdc_write(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int value)
{
	u8 data[2];

	data[0] = reg;
	data[1] = value;

	DEBUG_MSG("write reg=0x%02x to val=0x%02x\n", reg, value);

	jz_icdc_write_reg_cache(codec, reg, value);
	
	if (reg < JZ_ICDC_MAX_REGNUM) {
		if (jz_icdc_hw_write(codec->control_data, data, 2) == 2)
			return 0;
		else
			return -EIO;
	}

	return 0;
}

static inline void jz_icdc_update_reg(struct snd_soc_codec *codec,
					 unsigned int reg,
					 int lsb, int mask, u8 nval) {
        u8 oval;

	if(reg < JZ_ICDC_MAX_REGNUM)
		oval = jz_icdc_read_reg_hw(codec, reg);
	else
		oval = jz_icdc_read_reg_cache(codec, reg);

	oval &= ~(mask << lsb);
	oval |= (nval << lsb);

	jz_icdc_write(codec, reg, oval);
}

/*
 * CODEC write a bit of a register
 *
 * addr:        address of register
 * bitval:      bit value to modifiy
 * mask_bit:    indicate which bit will be modifiy (1 << nbit)
 */
static int jz_icdc_write_reg_bit(struct snd_soc_codec *codec, unsigned int addr, u8 bitval, u8 bit_mask)
{
	u8 val = jz_icdc_read_reg_hw(codec, addr);

	if (bitval)
		val |= bit_mask;
	else
		val &= ~bit_mask;
	jz_icdc_write_reg_hw((unsigned char)addr, val);

	return 1;
}

             /** 4780 CODEC MIXER and AGC operate function **/
/*
 * CODEC  write mixer function register
 *
 * addr:        address of register
 * val:         value to set
 */

static void codec_write_mixer_reg(struct snd_soc_codec *codec, u8 addr, u8 val) {
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_MIX, 0, (1 << 6));
	// first wirte data 
	jz_icdc_write_reg_hw(JZ_ICDC_DR_MIX, val);
	// second set addr 
	jz_icdc_update_reg(codec, JZ_ICDC_CR_MIX, 0, 0x3f, addr);
	// third, set load bit 
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_MIX, 1, (1 << 6));
	mdelay(1);
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_MIX, 0, (1 << 6));
}

#if USE_AGC_MIXER_FUNC
/*
 * CODEC read mixer function register
 *
 * addr:        address of register
 */
static unsigned char codec_read_mixer_reg(struct snd_soc_codec *codec, u8 addr)
{
	u8 regval;
	jz_icdc_update_reg(codec, JZ_ICDC_CR_MIX, 0, 0x3f, addr);
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_MIX, 0, (1 << 6));
	mdelay(1);
	regval = jz_icdc_read_reg_hw(codec, JZ_ICDC_DR_MIX);
	return regval;
}
/*
 * CODEC Mixer function disable
 */
static void codec_mixer_disable(struct snd_soc_codec *codec){
	u8 i;
	for(i = 0; i < 4; i++)
	 	codec_write_mixer_reg(codec, i, 0x0);
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_MIX, 0, (1 << 7));
}
/*
 *   CODEC write  mixer function reigster bit
 *    
 *     addr:        addr of mixer register
 *      val:         value of mixer reigsiter
 *       bit_mask:    indicate which bit will be modifiy (1 << nbit)
 */
static void codec_write_mixer_reg_bit(struct snd_soc_codec *codec, u8 addr, u8 bitval, u8 bit_mask)
{
	u8 val = codec_read_mixer_reg(codec, addr);
	if (bitval)
		val |= bit_mask;
	else
		val &= ~bit_mask;
        codec_write_mixer_reg(codec, addr, val);
}
/*
 * CODEC write agc  function register
 *
 * addr:        address of register
 * val:         value to set
 */
static void codec_write_agc_reg(struct snd_soc_codec *codec, u8 addr, u8 val)
{
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_ADC_AGC, 0, (1 << 6));
	// set data 
	jz_icdc_write_reg_hw(JZ_ICDC_DR_ADC_AGC, val);
	// set addr 
	jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC_AGC, 0, 0x3f, addr);
	// set load 
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_ADC_AGC, 1, (1 << 6));
	mdelay(1);
	// clear load 
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_ADC_AGC, 0, (1 << 6));
}
/*
 * CODEC read agc  function register
 *
 * addr:        address of register
 */
static unsigned char codec_read_agc_reg(struct snd_soc_codec *codec, u8 addr)
{
	u8 regval;

	jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC_AGC, 0, 0x3f, addr);
	jz_icdc_write_reg_bit(codec, JZ_ICDC_CR_ADC_AGC, 0, (1 << 6));
	mdelay(1);
	regval = jz_icdc_read_reg_hw(codec, JZ_ICDC_DR_ADC_AGC);

	return regval;
}
/*
 * CODEC write  agc function reigster bit
 * 
 * addr:        addr of mixer register
 * val:         value of mixer reigsiter
 * bit_mask:    indicate which bit will be modifiy (1 << nbit)
 */
static void codec_write_agc_reg_bit(struct snd_soc_codec *codec, u8 addr, u8 bitval, u8 bit_mask)
{
	u8 val = codec_read_agc_reg(codec, addr);
	if (bitval)
		val |= bit_mask;
	else
		val &= ~bit_mask;
	codec_write_agc_reg(codec, addr, val);
}
             /*** end MIXER and AGC function  ***/
#endif

__attribute__((__unused__)) static void dump_icdc_regs(struct snd_soc_codec *codec,
		   const char *func, int line)
{
        unsigned int i;

        printk("codec register dump, %s:%d:\n", func, line);
	for (i = 0; i < JZ_ICDC_MAX_REGNUM; i++)
		printk("address = 0x%02x, data = 0x%02x\n",
				i, jz_icdc_read_reg_hw(codec, i));
}

__attribute__((__unused__)) static void dump_aic_regs(const char *func, int line)
{
        char *regname[] = {"aicfr","aiccr","aiccr1","aiccr2","i2scr","aicsr","acsr","i2ssr",
                           "accar", "accdr", "acsar", "acsdr", "i2sdiv"};
        int i;
        unsigned int addr;

        printk("AIC regs dump, %s:%d:\n", func, line);
        for (i = 0; i <= 0x30; i += 4) {
                addr = 0xb0020000 + i;
                printk("%s\t0x%08x -> 0x%08x\n", regname[i/4], addr, *(unsigned int *)addr);
        }
}

static void turn_on_sb_hp(struct snd_soc_codec *codec) {
	if (jz_icdc_read_reg_hw(codec, JZ_ICDC_CR_HP) & (1 << 4)) {
		/* power on sb hp */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 4, 0x1, 0);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 1)))
			mdelay(10);
		/* clear DAC_MODE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 1, 0x1, 1);
		udelay(10);
		
		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 1)))
			mdelay(10);
		/* clear DAC_MODE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 1, 0x1, 1);
	}
}

static void turn_off_sb_hp(struct snd_soc_codec *codec) {
	if (!(jz_icdc_read_reg_hw(codec, JZ_ICDC_CR_HP) & (1 << 4))) {
		/* power off sb hp */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 4, 0x1, 1);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 1)))
			mdelay(10);
		/* clear DAC_MODE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 1, 0x1, 1);
	        udelay(10);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 1)))
			mdelay(10);
		/* clear DAC_MODE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 1, 0x1, 1);
	}
}

static void turn_on_dac(struct snd_soc_codec *codec) {
	/* DAC_MUTE */
	if (jz_icdc_read_reg_hw(codec, JZ_ICDC_CR_DAC) & (1 << 7)) {
		/* dac mute inactive */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 7, 0x1, 0);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 0)))
			mdelay(10);
		/* clear DAC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 0, 0x1, 1);
		udelay(10);
                
		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 0)))
			mdelay(10);
		/* clear DAC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 0, 0x1, 1);
		udelay(10);
	}
}

static void turn_off_dac(struct snd_soc_codec *codec) {
	if (!(jz_icdc_read_reg_hw(codec, JZ_ICDC_CR_DAC) & (1 << 7))) {
		/* dac mute active */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 7, 0x1, 1);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 0)))
			mdelay(10);
		/* clear DAC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 0, 0x1, 1);
		udelay(10);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 0)))
			mdelay(10);
		/* clear DAC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 0, 0x1, 1);
		udelay(10);
	}
}

static void turn_on_adc(struct snd_soc_codec *codec) {
	/* DAC_MUTE */
	if (jz_icdc_read_reg_hw(codec, JZ_ICDC_CR_ADC) & (1 << 7)) {
		/* adc mute inactive */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC, 7, 0x1, 0);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 2)))
			mdelay(10);
		/* clear ADC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 2, 0x1, 1);
		udelay(10);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 2)))
			mdelay(10);
		/* clear ADC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 2, 0x1, 1);
		udelay(10);
	}
}

static void turn_off_adc(struct snd_soc_codec *codec) {
	if (!(jz_icdc_read_reg_hw(codec, JZ_ICDC_CR_ADC) & (1 << 7))) {
		/* adc mute active */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC, 7, 0x1, 1);
                 
		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 2)))
			mdelay(10);
		/* clear ADC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 2, 0x1, 1);
		udelay(10);

		while( !(jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR) & (1 << 2)))
			mdelay(10);
		/* clear ADC_MUTE_EVENT flag */
		jz_icdc_update_reg(codec, JZ_ICDC_IFR, 2, 0x1, 1);
		udelay(10);
	}
}

static int jz_icdc_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 5, 0x1, 0);   //mic2_bias not use here
		mdelay(10);
		break;
	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_OFF:
		jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 5, 0x1, 1);
		mdelay(10);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static int jz_icdc_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;

	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);

	int bit_width = -1;
	int speed = -1;

	/* bit width */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		bit_width = 0;
		break;
	case SNDRV_PCM_FORMAT_S18_3LE:
		bit_width = 1;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		bit_width = 2;
		break;
	case SNDRV_PCM_FORMAT_S24_3LE:
		bit_width = 3;
		break;
	}

	if (bit_width < 0)
		return -EINVAL;

	if (playback)
		jz_icdc_update_reg(codec, JZ_ICDC_AICR_DAC, 6, 0x3, bit_width);
	else
		jz_icdc_update_reg(codec, JZ_ICDC_AICR_ADC, 6, 0x3, bit_width);

	/* sample rate */
	switch (params_rate(params)) {
        case 96000:
                speed = 10;
                break;
	case 88200:
		speed = 9;
		break;
        case 48000:
                speed = 8;
                break;
        case 44100:
                speed = 7;
                break;
        case 32000:
                speed = 6;
                break;
        case 24000:
                speed = 5;
                break;
        case 22050:
                speed = 4;
                break;
        case 16000:
                speed = 3;
                break;
        case 12000:
                speed = 2;
                break;
        case 11025:
                speed = 1;
                break;
        case 8000:
                speed = 0;
		break;
        }

	if (speed < 0)
		return -EINVAL;

	if (playback)
		jz_icdc_update_reg(codec, JZ_ICDC_FCR_DAC, 0, 0xf, speed);
	else
		jz_icdc_update_reg(codec, JZ_ICDC_FCR_ADC, 0, 0xf, speed);

	return 0;
}

static int replaying = 0;    //now playing flag 

static void jz_icdc_shutdown(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
        struct snd_soc_pcm_runtime *rtd = substream->private_data;
        struct snd_soc_codec *codec = rtd->codec;

        DEBUG_MSG("enter jz_icdc_shutdown, stream = %s\n", (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "playback" : "capture"));

        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		
		replaying = 0;
		/* mute DAC */
		turn_off_dac(codec);
		mdelay(1);

		/* power off SB_DAC */
		jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 1);
		mdelay(1);
        }
        mdelay(10);
}

static int jz_icdc_pcm_trigger(struct snd_pcm_substream *substream,
			      int cmd, struct snd_soc_dai *codec_dai) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	int ret = 0;

	DEBUG_MSG("enter %s:%d substream = %s bypass_to_hp = %d bypassto_lineout = %d cmd = %d\n",
		  __func__, __LINE__,
		  (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "playback" : "capture"),
		  bypass_to_hp, bypass_to_lineout,
		  cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
			
			replaying = 1;
			/* power on SB_DAC */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 0);
			mdelay(1);
			
			/* disable DAC mute */
			turn_on_dac(codec);
			mdelay(1);
			
			//dump_icdc_regs(codec, __func__, __LINE__);
			//dump_aic_regs(__func__, __LINE__);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* do nothing */
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int jz_icdc_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int jz_icdc_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

#define JZ_ICDC_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |	\
		       SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |	\
		       SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |	\
		       SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

#define JZ_ICDC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S18_3LE | \
			 SNDRV_PCM_FMTBIT_S20_3LE |SNDRV_PCM_FMTBIT_S24_3LE)

static struct snd_soc_dai_ops jz_icdc_dai_ops = {
	.hw_params	= jz_icdc_hw_params,
	.trigger	= jz_icdc_pcm_trigger,
	.shutdown       = jz_icdc_shutdown,
	.digital_mute	= jz_icdc_mute,
	.set_sysclk	= jz_icdc_set_dai_sysclk,
};

static struct snd_soc_dai_driver  jz4780_codec_dai = {
	.name = "jz4780-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = JZ_ICDC_RATES,
		.formats = JZ_ICDC_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = JZ_ICDC_RATES,
		.formats = JZ_ICDC_FORMATS,
	},
	.ops = &jz_icdc_dai_ops,
};


/* unit: 0.01dB */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -3100, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(out_tlv, -2500, 100, 0);
static const DECLARE_TLV_DB_SCALE(mic1_boost_tlv, 0, 400, 0);
static const DECLARE_TLV_DB_SCALE(mic2_boost_tlv, 0, 400, 0);
static const DECLARE_TLV_DB_SCALE(linein_tlv, -2500, 100, 0);

static const struct snd_kcontrol_new jz_icdc_snd_controls[] = {
	/* playback gain control */
	SOC_DOUBLE_R_TLV("PCM Volume", JZ_ICDC_GCR_DACL, JZ_ICDC_GCR_DACR, 0, 31, 1, dac_tlv),
	SOC_DOUBLE_R_TLV("Master Playback Volume", JZ_ICDC_GCR_HPL, JZ_ICDC_GCR_HPR,
			 0, 31, 1, out_tlv),

	/* record gain control */
	SOC_DOUBLE_R_TLV("ADC Capture Volume", JZ_ICDC_GCR_ADCL, JZ_ICDC_GCR_ADCR, 0, 23, 0,
			 adc_tlv),

	SOC_SINGLE_TLV("Mic1 Capture Volume", JZ_ICDC_GCR_MIC1, 0, 5, 0, mic1_boost_tlv),
	SOC_SINGLE_TLV("Mic2 Capture Volume", JZ_ICDC_GCR_MIC2, 0, 5, 0, mic2_boost_tlv),

	SOC_DOUBLE_R_TLV("Linein Bypass Capture Volume", JZ_ICDC_GCR_LIBYL, JZ_ICDC_GCR_LIBYR,
			 0, 31, 1, linein_tlv),
};

static int hpout_event(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol, int event) {
	DEBUG_MSG("enter %s, event = 0x%08x\n", __func__, event);
	//struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		hpout_enable = 1;
		break;

	case SND_SOC_DAPM_POST_PMD:
		hpout_enable = 0;
		break;
	}

	return 0;
}

static int lineout_event(struct snd_soc_dapm_widget *w,
		     struct snd_kcontrol *kcontrol, int event) {
	struct snd_soc_codec *codec = w->codec;

	DEBUG_MSG("enter %s, event = 0x%08x\n", __func__, event);
        
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
	        mdelay(1);
		jz_icdc_update_reg(codec, JZ_ICDC_CR_LO, 7, 0x1, 0);
		//lineout_enable = 1;
		break;

	case SND_SOC_DAPM_PRE_PMD:
		jz_icdc_update_reg(codec, JZ_ICDC_CR_LO, 7, 0x1, 1);
	        mdelay(1);
		//lineout_enable = 0;
		break;
	}

	return 0;
}

static const char *jz_icdc_input_sel[] = {"Aipn1", "Aip2", "Aip3"};
static const char *jz_icdc_hp_sel[] = {"Dacl", "Dacr", "AILatt", "AIRatt"};
static const char *jz_icdc_lineout_sel[] = {"Dacl", "Dacr", "AILatt", "AIRatt"};

#define INSEL_FROM_AIPN1	0
#define INSEL_FROM_AIP2		1
#define INSEL_FROM_AIP3		2

#define HP_SEL_FROM_DACL        0
#define HP_SEL_FROM_DACR        1
#define HP_SEL_FROM_AILatt      2
#define HP_SEL_FROM_AIRatt      3

#define LO_SEL_FROM_DACL        0
#define LO_SEL_FROM_DACR        1
#define LO_SEL_FROM_AILatt      2
#define LO_SEL_FROM_AIRatt      3

static int hp_out_mux_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	u8 *cache = codec->reg_cache;

	u8 outsel = 0xff;      /* an invalid value */

	switch (event) {
	case SND_SOC_DAPM_POST_REG:
		DEBUG_MSG("%s:%d, POST_REG, lhpsel = %d rhpsel = %d\n",
		       __func__, __LINE__, cache[JZ_ICDC_LHPSEL], cache[JZ_ICDC_RHPSEL]);

		/*
		 * panda out, take care!!!
		 * only the following are supported, else the result is unexpected!
		 *	* L(DACL) + R(DACR)
		 *	* L(DACL) + R(DACL)
		 *	* L(AILatt) + R(AIRatt) 
		 *	* L(AILatt) + R(AILatt)
		 *	* L(AIRatt) + R(AIRatt)
		 **/
		if ((cache[JZ_ICDC_LHPSEL] == HP_SEL_FROM_DACL) &&
		    (cache[JZ_ICDC_RHPSEL] == HP_SEL_FROM_DACR)) {

			outsel = 0;

		} else if ((cache[JZ_ICDC_LHPSEL] == HP_SEL_FROM_DACL) &&
		    (cache[JZ_ICDC_RHPSEL] == HP_SEL_FROM_DACL)) {

			outsel = 1;

		} else if ((cache[JZ_ICDC_LHPSEL] == HP_SEL_FROM_AILatt) &&
			   (cache[JZ_ICDC_RHPSEL] == HP_SEL_FROM_AIRatt)) {
			outsel = 2;
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 6, 0x1, 0);
			/* AIP2 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 0, 0x1, 1);
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 6, 0x1, 0);
			/* AIP3 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 0, 0x1, 0);

		} else if ((cache[JZ_ICDC_LHPSEL] == HP_SEL_FROM_AILatt) &&
			   (cache[JZ_ICDC_RHPSEL] == HP_SEL_FROM_AILatt)) {

			outsel = 3;
			/* differential input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 6, 0x1, 1);
			/* AIPN1 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 0, 0x1, 0);

		} else if ((cache[JZ_ICDC_LHPSEL] == HP_SEL_FROM_AIRatt) &&
			   (cache[JZ_ICDC_RHPSEL] == HP_SEL_FROM_AIRatt)) {

			outsel = 4;
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 6, 0x1, 0);
			/* AIP3 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 0, 0x1, 0);
		}

		DEBUG_MSG("%s:%d, outsel = %d,\n",
		       __func__, __LINE__, outsel);

		if (outsel != 0xff) {
			jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 0, 0x7, outsel);

			if (outsel > 1)
				bypass_to_hp = 1;
			else
				bypass_to_hp = 0;
		}else
			printk("your config route is not support!\n ");

		break;
	}

	return 0;
}

static int lineout_mux_event(struct snd_soc_dapm_widget *w,
			    struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	u8 *cache = codec->reg_cache;

	u8 outsel = 0xff;      /* an invalid value */


	switch (event) {
	case SND_SOC_DAPM_POST_REG:
		DEBUG_MSG("%s:%d, POST_REG, llosel = %d rlosel = %d\n",
			  __func__, __LINE__, cache[JZ_ICDC_LLOSEL], cache[JZ_ICDC_RLOSEL]);

		/*
		 * panda out, take care!!!
		 * only the following are supported, else the result is unexpected!
		 *	* L(DACL) + R(DACL)
		 *	* L(DACR) + R(DACR)
		 *	* L(DACL) + R(DACR)
		 *	* L(DACR) + R(DACL)
		 *	* L(AILatt) + R(AILatt) 
		 *	* L(AIRatt) + R(AIRatt)
		 *	* L(AILatt) + R(AIRatt)
		 *	* L(AIRatt) + R(AILatt)
		 **/
		if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_DACL) &&
		    (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_DACL)) {

			outsel = 0;

		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_DACR) &&
			   (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_DACR)) {

			outsel = 1;

		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_DACL) &&
			   (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_DACR)) {

			outsel = 2;
		
		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_DACR) &&
			   (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_DACL)) {

			outsel = 2;

		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_AILatt) &&
			   (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_AILatt)) {

			outsel = 4;
			/* differential input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 6, 0x1, 1);
			/* AIPN1 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 0, 0x1, 0);

		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_AIRatt) &&
			   (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_AIRatt)) {

			outsel = 5;
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 6, 0x1, 0);
			/* AIP3 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 0, 0x1, 0);

		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_AILatt) &&
			    (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_AIRatt)) {

			outsel = 6;
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 6, 0x1, 0);
			/* AIP2 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 0, 0x1, 1);
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 6, 0x1, 0);
			/* AIP3 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 0, 0x1, 0);
		} else if ((cache[JZ_ICDC_LLOSEL] == LO_SEL_FROM_AIRatt) &&
			    (cache[JZ_ICDC_RLOSEL] == LO_SEL_FROM_AILatt)) {

			outsel = 6;
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 6, 0x1, 0);
			/* AIP2 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI1, 0, 0x1, 1);
			/* single input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 6, 0x1, 0);
			/* AIP3 input */
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LI2, 0, 0x1, 0);
		}

		DEBUG_MSG("%s:%d, outsel = %d,\n",
			  __func__, __LINE__, outsel);

		if (outsel != 0xff) {
			jz_icdc_update_reg(codec, JZ_ICDC_CR_LO, 0, 0x7, outsel);

			if (outsel > 3)
				bypass_to_lineout = 1;
			else 
				bypass_to_lineout = 0;
		}else
			printk("your config route is not support!\n ");

		break;
	}

	return 0;
}

static int capture_mux_event(struct snd_soc_dapm_widget *w,
			      struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	u8 *cache = codec->reg_cache;

	int mic_stereo = 0;
	u8 insel = 0xff;      /* an invalid value */

	switch (event) {
	case SND_SOC_DAPM_POST_REG:
		DEBUG_MSG("%s:%d, POST_REG, linsel = %d rinsel = %d\n",
			  __func__, __LINE__, cache[JZ_ICDC_LINSEL], cache[JZ_ICDC_RINSEL]);

		/*
		 * panda out, take care!!!
		 * only the following are supported, else the result is unexpected!
		 *	* L(AIPN1) + R(AIPN1)     mic differential input
		 *	* L(AIP2) + R(AIP3)       linein single input
		 *	* L(AIP3) + R(AIP2)       linein single input 
		 **/
		if ((cache[JZ_ICDC_LINSEL] == INSEL_FROM_AIPN1) &&
		    (cache[JZ_ICDC_RINSEL] == INSEL_FROM_AIPN1)) {

			insel = 0;
		
		} else if ((cache[JZ_ICDC_LINSEL] == INSEL_FROM_AIP2) &&
			   (cache[JZ_ICDC_RINSEL] == INSEL_FROM_AIP3)) {

			insel = 0;
			mic_stereo = 1;

		} else if ((cache[JZ_ICDC_LINSEL] == INSEL_FROM_AIP3) &&
			   (cache[JZ_ICDC_RINSEL] == INSEL_FROM_AIP2)) {

			insel = 1;
			mic_stereo = 1;
		}

		DEBUG_MSG("%s:%d, insel = %d, mic_stereo = %d\n",
			  __func__, __LINE__, insel, mic_stereo);

		if (insel != 0xff) {
			jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC, 0, 0x3, insel);

			if (mic_stereo){
				/* Mic stereo inputs */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 7, 0x1, 1);
				/* single input */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 6, 0x1, 0);  
				/* AIP2 input */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 0, 0x1, 1);
                                /* single input */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC2, 6, 0x1, 0);
				/* AIP3 input */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC2, 0, 0x1, 0);
				/* MIXER left/right channel both normal input */
                                codec_write_mixer_reg(codec, 0x02, 0x00); 
			
			}else{
				/* Mic mono inputs */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 7, 0x1, 0);
				/* differential input */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 6, 0x1, 1);  
				/* AIPN1 input */
				jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 0, 0x1, 0);
				/* MIXER left channel normal input, right channel cross input */
                                codec_write_mixer_reg(codec, 0x02, 0x10); 
			}
		}else
			printk("your config route is not support!\n");

		break;

	default:
		break;
	}
	return 0;
}

static int micbias_event(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	//u8 *cache = codec->reg_cache;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		jz_icdc_set_bias_level(codec, SND_SOC_BIAS_ON);
		break;
	case SND_SOC_DAPM_POST_PMD:
		jz_icdc_set_bias_level(codec, SND_SOC_BIAS_OFF);
		break;
	default:
		break;
	}

	return 0;
}

static int adc_poweron_event(struct snd_soc_dapm_widget *w,
			 struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		turn_on_adc(codec);
		mdelay(10);
		break;
	case SND_SOC_DAPM_POST_PMD:
		turn_off_adc(codec);
		mdelay(10);
		break;
	default:
		break;
	}

	return 0;
}


static const struct soc_enum jz_icdc_enum[] = {
	SOC_ENUM_SINGLE(JZ_ICDC_LHPSEL, 0, 4, jz_icdc_hp_sel),
	SOC_ENUM_SINGLE(JZ_ICDC_RHPSEL, 0, 4, jz_icdc_hp_sel),

	SOC_ENUM_SINGLE(JZ_ICDC_LLOSEL, 0, 4, jz_icdc_lineout_sel),
	SOC_ENUM_SINGLE(JZ_ICDC_RLOSEL, 0, 4, jz_icdc_lineout_sel),

	SOC_ENUM_SINGLE(JZ_ICDC_LINSEL, 0, 3, jz_icdc_input_sel),
	SOC_ENUM_SINGLE(JZ_ICDC_RINSEL, 0, 3, jz_icdc_input_sel),
};

static const struct snd_kcontrol_new icdc_left_hp_mux_controls =
	SOC_DAPM_ENUM("Route", jz_icdc_enum[0]);

static const struct snd_kcontrol_new icdc_right_hp_mux_controls =
	SOC_DAPM_ENUM("Route", jz_icdc_enum[1]);

static const struct snd_kcontrol_new icdc_left_lo_mux_controls =
	SOC_DAPM_ENUM("Route", jz_icdc_enum[2]);

static const struct snd_kcontrol_new icdc_right_lo_mux_controls =
	SOC_DAPM_ENUM("Route", jz_icdc_enum[3]);

static const struct snd_kcontrol_new icdc_adc_left_controls =
	SOC_DAPM_ENUM("Route", jz_icdc_enum[4]);

static const struct snd_kcontrol_new icdc_adc_right_controls =
	SOC_DAPM_ENUM("Route", jz_icdc_enum[5]);


static const struct snd_soc_dapm_widget jz_icdc_dapm_widgets[] = {
	//SND_SOC_DAPM_PGA_E("HP Out",  SND_SOC_NOPM, 0, 0, NULL, 0,
	SND_SOC_DAPM_PGA_E("HP Out", JZ_ICDC_CR_HP, 7, 1, NULL, 0,
			   hpout_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("Line Out", JZ_ICDC_CR_LO, 4, 1, NULL, 0,
			   lineout_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_PGA("Mic1 Input", JZ_ICDC_CR_MIC1, 4, 1, NULL, 0),

	SND_SOC_DAPM_PGA("Mic2 Input", JZ_ICDC_CR_MIC2, 4, 1, NULL, 0),

	SND_SOC_DAPM_PGA("Linein1 Bypass", JZ_ICDC_CR_LI1, 4, 1, NULL, 0),
	
	SND_SOC_DAPM_PGA("Linein2 Bypass", JZ_ICDC_CR_LI2, 4, 1, NULL, 0),

	
	SND_SOC_DAPM_ADC_E("ADC", "HiFi Capture", JZ_ICDC_CR_ADC, 4, 1,
			   adc_poweron_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	
	SND_SOC_DAPM_DAC("DAC", "HiFi Playback", SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_MICBIAS_E("Mic Bias", SND_SOC_NOPM, 0, 0,
			       micbias_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX_E("Playback HP Left Mux", SND_SOC_NOPM, 0, 0,
			   &icdc_left_hp_mux_controls,
			   hp_out_mux_event,
			   SND_SOC_DAPM_POST_REG),

	SND_SOC_DAPM_MUX_E("Playback HP Right Mux", SND_SOC_NOPM, 0, 0,
			   &icdc_right_hp_mux_controls,
			   hp_out_mux_event,
			   SND_SOC_DAPM_POST_REG),

	SND_SOC_DAPM_MUX_E("Playback Lineout Left Mux", SND_SOC_NOPM, 0, 0,
			   &icdc_left_lo_mux_controls,
			   lineout_mux_event,
			   SND_SOC_DAPM_POST_REG),

	SND_SOC_DAPM_MUX_E("Playback Lineout Right Mux", SND_SOC_NOPM, 0, 0,
			   &icdc_right_lo_mux_controls,
			   lineout_mux_event,
			   SND_SOC_DAPM_POST_REG),

	SND_SOC_DAPM_MUX_E("Capture Left Mux", SND_SOC_NOPM, 0, 0,
			   &icdc_adc_left_controls,
			   capture_mux_event,
			   SND_SOC_DAPM_POST_REG),

	SND_SOC_DAPM_MUX_E("Capture Right Mux", SND_SOC_NOPM, 0, 0,
			   &icdc_adc_right_controls,
			   capture_mux_event,
			   SND_SOC_DAPM_POST_REG),

	SND_SOC_DAPM_OUTPUT("LHPOUT"),
	SND_SOC_DAPM_OUTPUT("RHPOUT"),

	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),

	SND_SOC_DAPM_INPUT("AIP1"),
	SND_SOC_DAPM_INPUT("AIN1"),
	SND_SOC_DAPM_INPUT("AIP2"),
	SND_SOC_DAPM_INPUT("AIP3"),
	
	SND_SOC_DAPM_INPUT("LINE1BY"),
	SND_SOC_DAPM_INPUT("LINE2BY"),
};

static const struct snd_soc_dapm_route intercon[] = {
	/* Destination Widget  <=== Path Name <=== Source Widget */
	{ "Mic1 Input", NULL, "AIP1" },
	{ "Mic1 Input", NULL, "AIN1" },
	{ "Mic1 Input", NULL, "AIP2" },
	
	{ "Mic2 Input", NULL, "AIP3" },

        { "Linein1 Bypass", NULL, "LINE1BY" },
        { "Linein2 Bypass", NULL, "LINE2BY" },

	{ "Capture Left Mux", "Aipn1", "Mic1 Input"  },
	{ "Capture Left Mux", "Aip2", "Mic1 Input"  },
	{ "Capture Left Mux", "Aip3", "Mic2 Input" },

	{ "Capture Right Mux", "Aipn1", "Mic1 Input"  },
	{ "Capture Right Mux", "Aip2", "Mic1 Input"  },
	{ "Capture Right Mux", "Aip3", "Mic2 Input" },

	{ "ADC", NULL, "Capture Right Mux" },
	{ "ADC", NULL, "Capture Left Mux" },

	{ "Playback HP Left Mux", "Dacl", "DAC" },
	{ "Playback HP Left Mux", "Dacr", "DAC" },
	{ "Playback HP Left Mux", "AILatt", "Linein1 Bypass" },
	{ "Playback HP Left Mux", "AIRatt", "Linein2 Bypass" },


	{ "Playback HP Right Mux", "Dacl", "DAC" },
	{ "Playback HP Right Mux", "Dacr", "DAC" },
	{ "Playback HP Right Mux", "AILatt", "Linein1 Bypass" },
	{ "Playback HP Right Mux", "AIRatt", "Linein2 Bypass" },

	{ "HP Out", NULL, "Playback HP Left Mux" },
	{ "HP Out", NULL, "Playback HP Right Mux" },

	{ "LHPOUT", NULL, "HP Out"},
	{ "RHPOUT", NULL, "HP Out"},


	{ "Playback Lineout Left Mux", "Dacl", "DAC" },
	{ "Playback Lineout Left Mux", "Dacr", "DAC" },
	{ "Playback Lineout Left Mux", "AILatt", "Linein1 Bypass" },
	{ "Playback Lineout Left Mux", "AIRatt", "Linein2 Bypass" },

	{ "Playback Lineout Right Mux", "Dacl", "DAC" },
	{ "Playback Lineout Right Mux", "Dacr", "DAC" },
	{ "Playback Lineout Right Mux", "AILatt", "Linein1 Bypass" },
	{ "Playback Lineout Right Mux", "AIRatt", "Linein2 Bypass" },

	{ "Line Out", NULL, "Playback Lineout Left Mux" },
	{ "Line Out", NULL, "Playback Lineout Right Mux" },

	{ "LOUT", NULL, "Line Out"},
	{ "ROUT", NULL, "Line Out"},
};

/*
static int jz_icdc_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(&(codec->dapm), jz_icdc_dapm_widgets,
				  ARRAY_SIZE(jz_icdc_dapm_widgets));

	snd_soc_dapm_add_routes(&(codec->dapm), intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(&(codec->dapm));
	return 0;
}
*/

static int jz_icdc_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	return 0;
}

static int jz_icdc_resume(struct snd_soc_codec *codec)
{
	return 0;
}

/* 4780 CODEC's gain controller init, it's a neccessary program */
static void dlv_set_non_default_gain(void)
{
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPL, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPR, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_DACL, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_DACR, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_ADCL, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_ADCR, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_MIC1, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_MIC2, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_LIBYL, 1);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_LIBYR, 1);
	mdelay(50);
}

static void dlv_recover_previous_gain(struct snd_soc_codec *codec)
{
	u8 *cache = codec->reg_cache;

	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPL, cache[JZ_ICDC_GCR_HPL]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPR, cache[JZ_ICDC_GCR_HPR]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_DACL, cache[JZ_ICDC_GCR_DACL]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_DACR, cache[JZ_ICDC_GCR_DACR]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_ADCL, cache[JZ_ICDC_GCR_ADCL]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_ADCR, cache[JZ_ICDC_GCR_ADCR]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_MIC1, cache[JZ_ICDC_GCR_MIC1]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_MIC2, cache[JZ_ICDC_GCR_MIC2]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_LIBYL, cache[JZ_ICDC_GCR_LIBYL]);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_LIBYR, cache[JZ_ICDC_GCR_LIBYR]);
}

/*
 * CODEC short circut handler
 *
 * To protect CODEC, CODEC will be shutdown when short circut occured.
 * Then we have to restart it.
 */
static inline void dlv_short_circut_handler(void)
{
	unsigned int    dlv_ifr;
	struct snd_soc_codec *codec = jz4780_codec;

	printk("CODEC will be restart.\n");

	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPL, 0x1f);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPR, 0x1f);
	mdelay(100);
	/* enable DAC_MUTE */
	turn_off_dac(codec);
	/* set Hp gain to +6dB */
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPL, 0x0);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPR, 0x0);
	mdelay(1);
	/* power off SB_HP */
	turn_off_sb_hp(codec);
	mdelay(1);
	/* enable HP_MUTE */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 7, 0x1, 1);
	mdelay(1);
	/* power off SB_DAC */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 1);
	mdelay(1);
	/* CODEC SLEEP*/
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 1, 0x1, 1);
	mdelay(1);
	/* CODEC SB*/
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 0, 0x1, 1);
	mdelay(1);

	while (1) {
		dlv_ifr = jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR);
		printk("waiting for short circuit recover --- dlv_ifr = 0x%02x\n", dlv_ifr);
		if ((dlv_ifr & DLC_IFR_SCLR ) == 0)
			break;
		jz_icdc_write_reg_hw(JZ_ICDC_IFR, DLC_IFR_SCLR);
		msleep(10);
	}

	/* clear SB */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 0, 0x1, 0);
	mdelay(300);

	/* clear SB_SLEEP */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 1, 0x1, 0);
	mdelay(400);

	/* 4780 CODEC gain controller init */
	dlv_set_non_default_gain();
	
	/* power on SB_DAC */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 0);
	mdelay(1);

	/* disable HP_MUTE */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 7, 0x1, 0);
	mdelay(1);

	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPL, 0x0);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPR, 0x0);
	mdelay(1);
	/* power on SB_HP*/
	turn_on_sb_hp(codec);
	mdelay(1);

	dlv_recover_previous_gain(codec);

	/* disable DAC mute */
	turn_on_dac(codec);
	mdelay(10);

	if(!replaying){
		/* power down the below parts for saving energy */
		turn_off_dac(codec);
		mdelay(1);

		jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 1);
		mdelay(1);
	}
	mdelay(50);
}

/*
 * CODEC work queue handler
 *
 * Handle bottom-half of SCLR & JACKE irq
 *
 */
//static unsigned int s_dlv_sr_jack = 2;  //init it to a invalid value
static void dlv_irq_work_handler(struct work_struct *work)
{
	unsigned int    dlv_ifr;
	struct snd_soc_codec *codec = jz4780_codec;
	u8 *cache = NULL;
	
	DLV_LOCK();
	do {
		dlv_ifr = jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR);
		if (dlv_ifr & DLC_IFR_SCLR) {
			printk("JZ CODEC: Short circut detected! IFR = 0x%02x\n",dlv_ifr);
			dlv_short_circut_handler();
		}
		dlv_ifr = jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR);

	} while(dlv_ifr & DLC_IFR_SCLR);
#if 0	
	/* detect HP jack */
	if(dlv_ifr & DLC_IFR_JACK){
		msleep(200);
		dlv_sr_jack = ((jz_icdc_read_reg_hw(codec, JZ_ICDC_SR) & (1 << 5))? 1 : 0);
		if(dlv_sr_jack != s_dlv_sr_jack){
			s_dlv_sr_jack = dlv_sr_jack;
			jz_icdc_write_reg_hw(JZ_ICDC_IFR, DLC_IFR_JACK);
			if(dlv_sr_jack){
				printk("HP plug in...\n");           //jack function hasnot add here
			}else{
				printk("HP take out...\n");
			}
		}
	}
#endif
	/* Unmask*/
	cache = codec->reg_cache;
	cache[JZ_ICDC_IFR] = jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR);
	jz_icdc_write_reg_hw(JZ_ICDC_IMR, ICR_COMMON_MASK);

	DLV_UNLOCK();
}

/*
 * IRQ routine
 *
 * Now we are only interested in SCLR.
 */
static irqreturn_t dlv_codec_irq(int irq, void *dev_id)
{
	unsigned char dlv_ifr;
	unsigned long flags;
	struct snd_soc_codec *codec = jz4780_codec;

	spin_lock_irqsave(&dlv_irq_lock, flags);

	dlv_ifr = jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR);
	jz_icdc_write_reg_hw(JZ_ICDC_IMR, ICR_ALL_MASK);

	if (!(dlv_ifr & (DLC_IFR_SCLR | DLC_IFR_JACK))) {
		/* CODEC may generate irq with flag = 0xc0.
		 * We have to ingore it in this case as there is no mask for the reserve bit.
		 */
		printk("AIC interrupt, IFR = 0x%02x\n", dlv_ifr);
		jz_icdc_write_reg_hw(JZ_ICDC_IMR, ICR_COMMON_MASK);
		spin_unlock_irqrestore(&dlv_irq_lock, flags);
		return IRQ_HANDLED;
	} else {
		spin_unlock_irqrestore(&dlv_irq_lock, flags);
		/* Handle SCLR and JACK in work queue. */
		schedule_work(&dlv_irq_work);
		return IRQ_HANDLED;
	}
}

static void dlv_set_initial_volume(struct snd_soc_codec *codec)
{
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_HPL, 0, 0xff, 6);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_HPR, 0, 0xff, 6);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_DACL, 0, 0xff, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_DACR, 0, 0xff, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_ADCL, 0, 0xff, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_ADCR, 0, 0xff, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_MIC1, 0, 0xff, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_MIC2, 0, 0xff, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_LIBYL, 0, 0xff, 6);
	jz_icdc_update_reg(codec, JZ_ICDC_GCR_LIBYR, 0, 0xff, 6);
}


static int jz_icdc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	int i = 0;
	u8 *cache = NULL;
	jz4780_codec = codec;

#if 1
	spin_lock_init(&dlv_irq_lock);
	
	/* init and locked */
	DLV_LOCKINIT();
	/* unlock */
	DLV_UNLOCK();

	/* register codec irq */
	INIT_WORK(&dlv_irq_work, dlv_irq_work_handler);
	ret = request_irq(IRQ_AIC0, dlv_codec_irq, IRQF_DISABLED, "dlv_codec_irq", NULL);
	if (ret) {
		printk("JZ DLV: Could not get AIC CODEC irq %d\n", IRQ_AIC0);
		return ret;
	}
#endif
	printk("start CODEC init...\n");
	
	/* clr SB */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 0, 0x1, 0);
	mdelay(300);

	/* clr SB_SLEEP */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 1, 0x1, 0);
	mdelay(400);

	/* 4780 CODEC gain controller init */
	dlv_set_non_default_gain();

	/* read CODEC register's value to array reg_cache */	
	cache = codec->reg_cache;
	for (i = 0; i < JZ_ICDC_MAX_REGNUM; i++)
		cache[i] = jz_icdc_read_reg_hw(codec, i);
	
	cache[JZ_ICDC_LHPSEL] = HP_SEL_FROM_DACL;
	cache[JZ_ICDC_RHPSEL] = HP_SEL_FROM_DACR;

	cache[JZ_ICDC_LLOSEL] = LO_SEL_FROM_DACL;
	cache[JZ_ICDC_RLOSEL] = LO_SEL_FROM_DACR;

	cache[JZ_ICDC_LINSEL] = INSEL_FROM_AIPN1;
	cache[JZ_ICDC_RINSEL] = INSEL_FROM_AIPN1;
	
	/* ADC/DAC: serial  i2s */
	jz_icdc_update_reg(codec, JZ_ICDC_AICR_ADC, 0, 0x3, 0x3);
	jz_icdc_update_reg(codec, JZ_ICDC_AICR_DAC, 0, 0x3, 0x3);
	/* ADC/DAC: audio interface open */
	jz_icdc_update_reg(codec, JZ_ICDC_AICR_ADC, 4, 0x1, 0);
	jz_icdc_update_reg(codec, JZ_ICDC_AICR_DAC, 4, 0x1, 0);
	/* CODEC master mode  */
	jz_icdc_update_reg(codec, JZ_ICDC_AICR_DAC, 5, 0x1, 0);
	
	/* 12M */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_CK, 0, 0xf, 0x0);
	
	/* default: DAC->HP */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 0, 0x7, 0x0);
	
	/* default: DAC->lineout */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_LO, 0, 0x7, 0x2);

	/* default: L(mic1) + R(mic1) */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC, 0, 0x3, 0x0);
	/* mic mono */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 7, 0x1, 0);
	/* default to MICDIFF */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_MIC1, 6, 0x1, 1);
	/* MIXER left channel normal input, right channel cross input */
	codec_write_mixer_reg(codec, 0x02, 0x10); 
	
	/* The generated IRQ is a high level */
	jz_icdc_update_reg(codec, JZ_ICDC_ICR, 6, 0x3, 0x0);
	jz_icdc_update_reg(codec, JZ_ICDC_IMR, 0, 0xe7, ICR_COMMON_MASK);

	jz_icdc_update_reg(codec, JZ_ICDC_IFR, 0, 0xe7, 0xe7); //write 1 will reset the flag to 0
	mdelay(5);
	cache[JZ_ICDC_IFR] = jz_icdc_read_reg_hw(codec, JZ_ICDC_IFR); 

	/* disable AGC */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_ADC_AGC, 7, 0x1, 0);

	/* mute lineout/HP */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_LO, 7, 0x1, 1);
	jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 7, 0x1, 1);
	
	/* default open high_pass_filter in record channel*/
	jz_icdc_update_reg(codec, JZ_ICDC_FCR_ADC, 6, 0x1, 1);

	/* anti HP pop sequences */
	/* power on SB_DAC */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 0);
	mdelay(1);	

	/* disable HP_MUTE */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 7, 0x1, 0);
	mdelay(1);

	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPL, 0x0);
	jz_icdc_write_reg_hw(JZ_ICDC_GCR_HPR, 0x0);

	/* power on SB_HP*/
	turn_on_sb_hp(codec);
	mdelay(1);

	dlv_set_initial_volume(codec);
	
	/* disable DAC mute */
	turn_on_dac(codec);
	mdelay(10);

	/* power down the below parts for saving energy */
	turn_off_dac(codec);
	mdelay(1);
	
	jz_icdc_update_reg(codec, JZ_ICDC_CR_HP, 7, 0x1, 1);
	mdelay(1);
	
	jz_icdc_update_reg(codec, JZ_ICDC_CR_DAC, 4, 0x1, 1);
	mdelay(1);
	/* end CODEC init*/
	
	return ret;
}

/* power down chip */
static int jz_icdc_remove(struct snd_soc_codec *codec)
{
	/* power off SB_HP */
	turn_off_sb_hp(codec);
	mdelay(10);
	jz_icdc_set_bias_level(codec, SND_SOC_BIAS_OFF);
	
	/* clear SB_SLEEP */ 
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 1, 0x1, 1);
	mdelay(10);

	/* clear SB */
	jz_icdc_update_reg(codec, JZ_ICDC_CR_VIC, 0, 0x1, 1);

	free_irq(IRQ_AIC0, NULL);
	
	DLV_LOCKDEINIT();	
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_jz4780_codec = {
	.probe = 	jz_icdc_probe,
	.remove = 	jz_icdc_remove,
	.suspend =	jz_icdc_suspend,
	.resume =	jz_icdc_resume,

	.read = 	jz_icdc_read_reg_cache,
	.write = 	jz_icdc_write,
	.set_bias_level = jz_icdc_set_bias_level,
	.reg_cache_default = jz4780_priv.reg_cache,
	.reg_word_size = sizeof(u8),
	.reg_cache_step = 1,
	.reg_cache_size = JZ_ICDC_MAX_NUM,

	.controls = 	jz_icdc_snd_controls,
	.num_controls = ARRAY_SIZE(jz_icdc_snd_controls),
	.dapm_widgets = jz_icdc_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(jz_icdc_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};

static __devinit int jz4780_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_jz4780_codec, &jz4780_codec_dai, 1);
}

static int __devexit jz4780_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver jz4780_codec_driver = {
	.driver = {
		.name = "jz4780-codec",
		.owner = THIS_MODULE,
	},

	.probe = jz4780_probe,
	.remove = __devexit_p(jz4780_remove),
};

static int __init jz_icdc_modinit(void)
{
	return platform_driver_register(&jz4780_codec_driver);
}
module_init(jz_icdc_modinit);

static void __exit jz_icdc_exit(void)
{
	platform_driver_unregister(&jz4780_codec_driver);
}
module_exit(jz_icdc_exit);

MODULE_DESCRIPTION("Jz4780 Internal Codec Driver");
MODULE_AUTHOR("Lutts Wolf<slcao@ingenic.cn>");
MODULE_LICENSE("GPL");
