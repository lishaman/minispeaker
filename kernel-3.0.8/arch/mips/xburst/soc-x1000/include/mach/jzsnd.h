/**
 * jzsnd.h
 *
 **/
#ifndef __MACH_JZSND_H__
#define __MACH_JZSND_H__

#include <linux/list.h>
#include <linux/sound.h>
#include <linux/platform_device.h>

enum snd_codec_route_t {
	SND_ROUTE_NONE,

	SND_ROUTE_ALL_CLEAR,
	SND_ROUTE_REPLAY_CLEAR,
	SND_ROUTE_RECORD_CLEAR,

	SND_ROUTE_RECORD_AMIC,
	SND_ROUTE_RECORD_DMIC,
	SND_ROUTE_REPLAY_SPK,
	SND_ROUTE_RECORD_AMIC_AND_REPLAY_SPK,
	SND_ROUTE_REPLAY_SOUND_MIXER_LOOPBACK,
	SND_ROUTE_LINEIN_REPLAY_MIXER_LOOPBACK,
	SND_ROUTE_AMIC_RECORD_MIX_REPLAY_LOOPBACK,
	SND_ROUTE_DMIC_RECORD_MIX_REPLAY_LOOPBACK,
	SND_ROUTE_REPLAY_LINEIN,

	SND_ROUTE_COUNT,
};

typedef enum {
	KEEP_OR_IGNORE = 0, //default
	STATE_DISABLE,
	STATE_ENABLE,
} audio_gpio_state_t;

typedef enum {
	NONE_INSERTED = 0,
	USB_INSERTED,
	HEADSET_INSERTED,
	LOW_POWER,
} board_switch_t;

struct snd_board_route {
	enum snd_codec_route_t route;
	//val : 0 is mean not set use default volume base
	int record_volume_base;					//val:0 ~ 5  => 0(db) ~ 20(db)   | 4db/step;
	int replay_digital_volume_base;				//val:0 ~ 31 => -31(db) ~ 0(db) | 1db/step;
	int record_digital_volume_base;				//val:0 ~ 43 => 0(db) ~ 43(db)   | 1db/step;
	audio_gpio_state_t gpio_spk_en_stat;
};

struct snd_board_gpio {
	int gpio;			/* -1: gpio is unavailable*/
	int active_level;	/* -1 , invalidation 0 ,low avlible  1,high valible */
};

struct snd_codec_data {
	/* clk */
	int codec_sys_clk;
	int codec_dmic_clk;

	/* volume */

	int record_volume_base;				/*amic*/
	int record_digital_volume_base;			/*adc*/
	int replay_digital_volume_base;			/*dac*/

	/* default route */
	struct snd_board_route replay_def_route;
	struct snd_board_route record_def_route;
	/* device <-> route map record*/
	struct snd_board_route record_buildin_mic_route;
	struct snd_board_route record_linein_route;
	/* device <-> route map replay*/
	struct snd_board_route replay_speaker_route;
	struct snd_board_route replay_speaker_record_buildin_mic_route;

	/* gpio */
	struct snd_board_gpio gpio_spk_en;
	struct snd_board_gpio gpio_amp_power;		/* analog power amplifier power pin */
	struct snd_board_gpio gpio_hp_mute;
	struct snd_board_gpio gpio_hp_detect;
	struct snd_board_gpio gpio_linein_detect;

	void (*board_switch_callback)(board_switch_t state);
	void (*board_pa_control)(bool enable);
	void (*board_dmic_control)(bool enable);
};

#ifdef CONFIG_AKM4753_EXTERNAL_CODEC
#define LOW_ENABLE                      0
#define HIGH_ENABLE                     1
struct akm4753_platform_data {
	struct snd_board_gpio *pdn;
};
#endif

#ifdef CONFIG_AKM4951_EXTERNAL_CODEC
#define LOW_ENABLE                      0
#define HIGH_ENABLE                     1
struct akm4951_platform_data {
	struct snd_board_gpio *pdn;
};
#endif

#ifdef CONFIG_AKM4345_EXTERNAL_CODEC
#define LOW_ENABLE                      0
#define HIGH_ENABLE                     1
struct akm4345_platform_data {
	struct snd_board_gpio *pdn;
	struct snd_board_gpio *csn;
	struct snd_board_gpio *cclk;
	struct snd_board_gpio *cdti;
	struct snd_board_gpio *cdto;
};
#endif

/*####################################################*\
* common, used for sound devices
\*####################################################*/
/**
 * device mame and minor
 **/
#define MAX_SND_MINOR  	128
#define SOUND_STEP		16

#define DEV_DSP_NAME	"dsp"

#define SND_DEV_DSP0  	(SND_DEV_DSP + SOUND_STEP * 0)
#define SND_DEV_DSP1  	(SND_DEV_DSP + SOUND_STEP * 1)
#define SND_DEV_DSP2  	(SND_DEV_DSP + SOUND_STEP * 2)
#define SND_DEV_DSP3  	(SND_DEV_DSP + SOUND_STEP * 3)

#define DEV_MIXER_NAME	"mixer"

#define SND_DEV_MIXER0	(SND_DEV_CTL + SOUND_STEP * 0)
#define SND_DEV_MIXER1 	(SND_DEV_CTL + SOUND_STEP * 1)
#define SND_DEV_MIXER2 	(SND_DEV_CTL + SOUND_STEP * 2)
#define SND_DEV_MIXER3 	(SND_DEV_CTL + SOUND_STEP * 3)

#define minor2index(x)	((x) / SOUND_STEP)

/**
 * struct snd_dev_data: sound device platform_data
 * @list: the list is usd to point to next or pre snd_dev_data.
 * @ext_data: used for dsp device, point to
 * (struct dsp_endpoints *) of dsp device.
 * @minor: the minor of the specific device. value will be
 * SND_DEV_DSP(0...), SND_DEV_MIXER(0...) and so on.
 * @is_suspend: indicate if the device is suspend or not.
 * @dev_ioctl: privide by the specific device, used
 * to control the device by command.
 * @init: specific device init.
 * @shutdown: specific device shutdown.
 * @suspend: specific device suspend.
 * @resume: specific device resume.
 **/
struct snd_dev_data {
	struct list_head list;
	struct device *dev;
	struct platform_device *pdev;
	void *ext_data;
	void *priv_data;

	int minor;
	bool is_suspend;
	long (*dev_ioctl) (unsigned int cmd, unsigned long arg);
	long (*dev_ioctl_2)(struct snd_dev_data *dev_data, unsigned int cmd, unsigned long arg);
	struct dsp_endpoints * (*get_endpoints)(struct snd_dev_data *dev_data);

	int (*open)(struct snd_dev_data *ddata, struct file *file);
	ssize_t (*read)(struct snd_dev_data *ddata,
			char __user *buffer, size_t count, loff_t *ppos);
	int (*release)(struct snd_dev_data *ddata, struct file *file);

	/************end**************/
	int (*init)(struct platform_device *pdev);
	void (*shutdown)(struct platform_device *pdev);
	int (*suspend)(struct platform_device *pdev, pm_message_t state);
	int (*resume)(struct platform_device *pdev);
};

extern struct snd_dev_data i2s_data;
extern struct snd_dev_data pcm_data;
extern struct snd_dev_data spdif_data;
extern struct snd_dev_data dmic_data;
extern struct snd_dev_data snd_mixer0_data;		//for i2s debug
extern struct snd_dev_data snd_mixer1_data;		//for pcm debug
extern struct snd_dev_data snd_mixer2_data;		//for spdif debug
extern struct snd_dev_data snd_mixer3_data;		//for dmic debug
/*call state*/
extern bool i2s_is_incall(void);

#endif //__MACH_JZSND_H__
