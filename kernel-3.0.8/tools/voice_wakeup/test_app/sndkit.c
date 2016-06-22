/*
 * Author: Lutts Wolf <lutts.cao@gmail.com>
 *
 * wav/raw file player & recorder for OSS testing
 */

#include "headers.h"

#define AFMT_S24_LE		0x00000800


#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

/* my own header files */
#include "wav.h"

int debug_level = 1;

#define FILE_TYPE_WAV	0
#define FILE_TYPE_RAW	1


#define FMT8BITS	AFMT_U8
#define FMT16BITS	AFMT_S16_LE
#define FMT24BITS	AFMT_S24_LE

#define DEFAULT_SND_DEV	"/dev/snd/dsp3"

//#define DEFAULT_SND_SPD 8000
#define DEFAULT_SND_SPD 16000
#define DEFAULT_SND_CHN 1

/*#define DEFAULT_SND_SPD 44100
#define DEFAULT_SND_CHN 2*/

#define DEFAULT_BUFF_SIZE 4096//	(4096 * 3)
/*#define DEFAULT_BUFF_SIZE 512//	(4096 * 3)*/

/* unit: s */
#define DEFAULT_DURATION	10

#define SND_FMT_S8		0
#define SND_FMT_U8		1
#define SND_FMT_S16_LE		2
#define SND_FMT_S16_BE		3
#define SND_FMT_U16_LE		4
#define SND_FMT_U16_BE		5
#define SND_FMT_S24_3LE		6
#define SND_FMT_S24_3BE		7
#define SND_FMT_U24_3LE		8
#define SND_FMT_U24_3BE		9

/* now sndkit only support: U8, S16_LE, S24_3LE, other formats hasnot be support */
const char *supported_fmts[] = {
	"S8",
	"U8",
	"S16_LE",
	"S16_BE",
	"U16_LE",
	"U16_BE",
	"S24_3LE",
	"S24_3BE",
	"U24_3LE",
	"U24_3BE",
};

static int sign_adjust = 0;
static int endian_adjust = 0;

static int fmtstr_to_fmt(char *fmtstr) {
	unsigned int i = 0;
	int fmt;

	for (i = 0; i < ARRAY_SIZE(supported_fmts); i++) {
		if (!strcmp(supported_fmts[i], fmtstr))
			break;
	}

	if (i == ARRAY_SIZE(supported_fmts))
		return -1;

	sign_adjust = 0;
	endian_adjust = 0;

	switch (i) {
	case SND_FMT_U8:
		sign_adjust = 1;
	case SND_FMT_S8:
		fmt = FMT8BITS;
		break;

	case SND_FMT_U16_BE:
		endian_adjust = 1;
	case SND_FMT_U16_LE:
		sign_adjust = 1;
		fmt = FMT16BITS;
		break;

	case SND_FMT_S24_3BE:
		endian_adjust = 1;
	case SND_FMT_S24_3LE:
		fmt = FMT24BITS;
		break;

	case SND_FMT_U24_3BE:
		endian_adjust = 1;
	case SND_FMT_U24_3LE:
		sign_adjust = 1;
		fmt = FMT24BITS;
		break;

	case SND_FMT_S16_BE:
		endian_adjust = 1;
	case SND_FMT_S16_LE:
	default:
		fmt = FMT16BITS;
	}

	return fmt;
}

static int support_rates[] = {
	8000,
	11025,
	12000,
	16000,
	22050,
	24000,
	32000,
	44100,
	48000,
	96000,
};

static int rate_is_supported(int rate) {
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(support_rates); i++)
		if (support_rates[i] == rate)
			return 1;

	return 0;
}

struct replay_params {
	int dev_fd;
	int replay_fd;
	int buffer_size;
	int replay_data_len;
	int format;
	int channels;
	int speed;
	int file_type;
};

struct record_params {
	int dev_fd;
	int record_fd;
	int buffer_size;
	int duration;
	int format;
	int channels;
	int speed;
	int file_type;
	int record_set_dmic;
	int set_dmic_num;
};

struct rec_replay_params {
	int __null;

	int rec_fd;
	int replay_fd;
	int speed;
	int channels;
	int format;

};
struct wakeup_params {
	int fd_res, fd_sys_set, fd_sys_ctl, fd_wakeup_reading;

	int __null;
};

#define SNDKIT_VERSION	"sndkit 2.0, 2013-08-14"

static struct option long_options[] = {
	{ "help", 0, 0, 'h' },
	{ "version", 0, 0, 'V' },
	{ "makewav", 0, 0, 'W' },
	{ "strip", 0, 0, 'T' },

	{ "device", 1, 0, 'D' },
	{ "set-dmic", 1, 0, 'm' },
	{ "file-type", 1, 0, 't' },
	{ "channels", 1, 0, 'c' },
	{ "format", 1, 0, 'f' },
	{ "rate", 1, 0, 'r'},
	{ "frequency", 1, 0, 'F' },
	{ "duration", 1, 0, 'd' },
	{ "buffer-size", 1, 0, 'B' },
	{ "verbose", 0, 0, 'v'},

	{ "replay", 0, 0, 'P' },
	{ "replay-file", 1, 0, 'p' },
	{ "save-replay", 0, 0, 'S' },
	{ "save-file", 1, 0, 's' },


	{ "record", 0, 0, 'R' },
	{ "record-file", 1, 0, 'w' },

	{ "volume", 1, 0, 'o' },

	{0, 0, 0, 0}
};

static char optstring[] = "+hVWTD:m:t:c:f:r:F:d:B:vPp:Ss:Rw:o:";

static void usage(void) {
	unsigned int i = 0;

	printf("Usage: sndkit [OPTION]... [FILE]...\n\n");

	printf("-h --help\t\tprint this message\n");
	printf("-V --version\t\tprint current version\n");
	printf("-W --makewav\t\tconvert raw audio data to wav format\n");
	printf("-T --strip\t\tstrip the wav header(aka, convert wav to raw)\n");
	printf("-D --device=NAME\tselect OSS snd device(default is /dev/dsp)\n");
	printf("-m --set-dmic\tset dmic param(num is 1~65)\n");

	printf("-t --file-type=TYPE\tfile type (wav, raw)\n");
	printf("-c --channels=#\t\tchannels\n");
	printf("-f --format=FORMAT\tsample format (case insensitive)\n");
	printf("-r --rate=#\t\tsample rate(fs)\n");
	printf("-F --frequency=#\tsine wave frequency(fa)\n");
	printf("-d --duration=#\t\trecord duration default is 10s, or analysis size\n");
	printf("-B --buffer-size=#\tbuffer size\n");
	printf("-v --verbose\t\tshow verbose(debug) messages\n");

	printf("-P --replay\t\tenable playback\n");
	printf("-p --replay-file=FILE\tthe sound file to play\n");
	printf("-S --save-replay\tsave the decoded data send to driver\n");
	printf("-s --save-file\t\tfile to save the decoded data\n");

	printf("-R --record\t\tenable record\n");
	printf("-w --record-file=FILE\tfile to save the recorded data\n");

	printf("-o --volume=0-100\tvolume\n");

	printf("\n");
	printf("Supported speeds are:");
	for (i = 0; i < ARRAY_SIZE(support_rates); i++) {
		printf("%d", support_rates[i]);

		if (i != (ARRAY_SIZE(support_rates) - 1))
			printf(", ");
	}

	printf("\n\n");

	printf("Recognized sample formats are:");
	printf(" U8, S16_LE, S24_3LE");
	printf("\n");
}

static int open_device(char *file, int oflag, int *is_reg_file) {
	struct stat stats;
	int fd = -1;

	*is_reg_file = 0;

	if ((lstat(file, &stats) == 0) &&
	    S_ISREG(stats.st_mode)) {
		sndkit_dbg("NOTE: device file is a regular file.\n");
		*is_reg_file = 1;
	}

	fd = open(file, oflag);
	if (fd < 0) {
		fprintf(stderr, "%s: cannot open file %s: %s\n",
			__func__,
			file,
			strerror(errno));

	}

	return fd;
}

static int open_file(char *file, int oflag) {
	int fd = -1;

	fd = open(file, oflag);
	if (fd < 0) {
		fprintf(stderr, "%s: cannot open file %s: %s\n",
			__func__,
			file,
			strerror(errno));
	}
	return fd;
}

static int config_device(int fd, int format, int channels, int speed)
{
	int ioctl_val;

	/* NOTE: config order is important: format, channel, speed */

	/* set bit format */
	ioctl_val = format;
	if (ioctl(fd, SNDCTL_DSP_SETFMT, &ioctl_val) == -1) {
		fprintf(stderr, "set format failed: %s\n",
			strerror(errno));
		return -1;
	}
	if (ioctl_val != format) {
		fprintf(stderr, "format not supported, changed by driver to 0x%08x\n",
			ioctl_val);
		return -1;
	}

	/*set channel */
	ioctl_val = channels;
	if ((ioctl(fd, SNDCTL_DSP_CHANNELS, &ioctl_val)) == -1) {
		fprintf(stderr, "set channels failed: %s\n",
			strerror(errno));
		return -1;
	}
	if (ioctl_val != channels) {
		fprintf(stderr, "%d channel is not supported, changed to %d by driver\n",
			channels, ioctl_val);
		return -1;
	}

	/*set speed */
	ioctl_val = speed;
	if (ioctl(fd, SNDCTL_DSP_SPEED, &ioctl_val) == -1) {
		fprintf(stderr, "set speed failed: %s\n",
			strerror(errno));
		return -1;
	}
	if (ioctl_val != speed) {
		fprintf(stderr, "speed %d is not supported, changed to %d by driver\n",
			speed, ioctl_val);
		return -1;
	}

	return 0;
}

static struct replay_params replay_params;
static int play(struct replay_params *params) {
	int dev_fd = params->dev_fd;
	int replay_fd = params->replay_fd;
	int buffer_size = params->buffer_size;
	int replay_data_len = params->replay_data_len;
	int format = params->format;
	int channels = params->channels;
	int speed = params->speed;
	int file_type = params->file_type;

	int ret = 0;
	int len = 0;
	int n = 0;
	int pos;
	unsigned char *buff;
	int alloc_size = buffer_size;
	int forever = 0;

	if (format == FMT24BITS) {
		if (buffer_size % 3) {
			alloc_size = buffer_size / 3 + 1;
			alloc_size = alloc_size * 3;
		}
		buff = malloc(alloc_size / 3 * 4);
	} else {
		buff = malloc(alloc_size);
	}

	if (buff == NULL) {
		fprintf(stderr, "PLAY: alloc buffer failed, size = %d\n", buffer_size);
		return -1;
	}

	if (replay_data_len == 0)
		forever = 1;


	while (forever || (replay_data_len > 0)) {
		len = read(replay_fd, buff, buffer_size);
		if (len < 0) {
			perror("PLAY: read sound data file failed");
			ret = -1;
			goto out;
		}

		if (len == 0) {
			sndkit_dbg("sound data play complete!\n");
			goto out;
		}

		if (!forever)
			replay_data_len -= len;

		if ( (file_type == FILE_TYPE_WAV) &&
		     (format == FMT24BITS)) {
			len = (len / 3) * 3;
			wav_expand_24bit_to_32bit(buff, len);
			len = len / 3 * 4;
		}

		pos = 0;
		while (len > 0) {
			if ((n = write(dev_fd, buff + pos, len)) < 0) {
				perror("write sound device file failed");
				ret = -1;
				goto out;
			}

			len -= n;
			pos += n;
		}
	}
 out:
	free(buff);
	sndkit_dbg("PLAY: end.\n");
	return ret;
}

static void *replay_thread(void *arg) {
	play((struct replay_params *)arg);

	return NULL;
}

static struct record_params record_params;
static int record(struct record_params *params) {
	int dev_fd = params->dev_fd;
	int record_fd = params->record_fd;
	int buffer_size = params->buffer_size;
	int duration = params->duration;
	int format = params->format;
	int channels = params->channels;
	int speed = params->speed;
	int file_type = params->file_type;
	int record_set_dmic = params->record_set_dmic;
	int set_dmic_num = params->set_dmic_num;

	int ret = 0;
	int n_read = 0;
	int n_to_write = 0;
	int n_to_write2 = 0;
	int n;
	int pos;
	unsigned char *buff;
	unsigned char *buff1;
	int format_bits = 16;
	int record_len = 0;

	buff = malloc(buffer_size);

	if (buff == NULL) {
		fprintf(stderr, "REC: alloc buffer failed, size = %d\n", buffer_size);
		return -1;
	}

	switch(format) {
	case FMT8BITS:
		format_bits = 8;
		break;
	case FMT16BITS:
		format_bits = 16;
		break;
	case FMT24BITS:
		format_bits = 24;
		break;
	default:
		break;
	}

	sndkit_dbg("REC: format_bits = %d, channels = %d, speed = %d, duration = %d\n", format_bits, channels, speed, duration);

	record_len = format_bits * channels * speed / 8 * duration;
	buff1 = malloc(record_len);

	sndkit_dbg("record len = %d\n", record_len);

	while (record_len > 0) {
		n_read = read(dev_fd, buff,
			      (buffer_size > record_len)
			      ? record_len
			      : buffer_size);

		if (n_read < 0) {
			perror("read sound device(file) failed");
			ret = -1;
			goto out;
		}

		if (n_read == 0) {
			printf("REC: zero bytes! why?\n");
			continue;
		}

		if ((file_type == FILE_TYPE_WAV) &&
		    (format_bits == 24)) {
			wav_32bit_to_24bit(buff, n_read);
			n_read = n_read / 4 * 3;
		}

		if (n_read < record_len)
			n_to_write = n_read;
		else
			n_to_write = record_len;

		n_to_write2 = n_to_write;

		pos = 0;
		if ((buff1 = memcpy(buff1, buff + pos, n_to_write)) < 0) {
			perror("REC: write sound data file failed");
			ret = -1;
			goto out;
		}
		buff1 += n_to_write;

		record_len -= n_to_write2;
	}


	printf("start write\n");
	record_len = format_bits * channels * speed / 8 * duration;
	buff1 -= record_len;
	pos = 0;
	while (record_len > 0) {
		if ((n = write(record_fd, buff1 + pos, record_len)) < 0) {
			perror("REC: write sound data file failed");
			ret = -1;
			goto out;
		}

		record_len -= n;
		pos += n;
	}
	printf("writed\n");
 out:
	free(buff);
	free(buff1);
	sndkit_dbg("REC: end.\n");
	return ret;
}

static void *record_thread(void *arg) {
	record((struct record_params *)arg);

	return NULL;
}

struct rec_replay_params  rec_replay_params;

char temp_buffer[4096];
static int rec_replay(struct rec_replay_params * rec_replay_params)
{



	while(1) {
		read(rec_replay_params->rec_fd, temp_buffer, 4096);

		write(rec_replay_params->replay_fd, temp_buffer, 4096);
	}


}
static void *rec_replay_thread(void *arg) {

	rec_replay((struct rec_replay_params*)arg);
	return NULL;
}

#define WAKEUP_SRC_FILE	"/data/TestPcm/ivModel_v21.irf"
#define SYSFS_SET_RES_NODE	"/sys/devices/platform/dsp.3/dmic_wakeup_set_res"
#define SYSFS_CTL_NODE	"/sys/devices/platform/dsp.3/dmic_wakeup_ctl"
#define SYSFS_WAKEUP_READING_NODE	"/sys/devices/platform/dsp.3/dmic_wakeup_reading"

#define SYSFS_CTL_CREATE_OBJECT     "1"
#define SYSFS_CTL_ENABLE_WAKEUP     "2"
#define SYSFS_CTL_DISABEL_WAKEUP    "3"

static struct wakeup_params wakeup_params;
unsigned char tempbuffer[2048];
static int wakeup(struct wakeup_params *wakeup)
{
	printf("wakeup processing\n");
	int fd_res, fd_sys_set, fd_sys_ctl, fd_wakeup_reading;
	int file_len;
	int read_count, write_count, write_count_1, pos = 0;
	int total_size;
	fd_res = wakeup->fd_res;
	fd_sys_set = wakeup->fd_sys_set;
	fd_sys_ctl = wakeup->fd_sys_ctl;
	fd_wakeup_reading = wakeup->fd_wakeup_reading;
	/*set resource file*/
	while(1) {

		read_count = read(fd_res, tempbuffer, 2048);
		if(read_count == 0)  {/*read ok*/
			printf("src file total_size:%d Bytes\n", total_size);
			break;
		}
		write_count = read_count;
		write_count_1 = write(fd_sys_set, tempbuffer, write_count);

		total_size += read_count;
	}
	printf("user set resource file ok!\n");

	/*create object*/
	write(fd_sys_ctl, SYSFS_CTL_CREATE_OBJECT, 1);
	printf("user set ctl create object ok!\n");

	write(fd_sys_ctl, SYSFS_CTL_ENABLE_WAKEUP, 1);
	printf("enable wakeup module\n");

	char buf_ctl[4];
	int i, ret ;
	ret = read(fd_sys_ctl, buf_ctl, 4);
	//ret = read(fd_wakeup_reading, buf_ctl, 4);
	if(ret < 0) {
		perror("read sys ctl\n");
	} else {
		printf("system wakeup or wakeup timeout\n");
	}


}
static void *wakeup_thread(void *arg)
{
	printf("wakeup thread start\n");
	wakeup((struct wakeup_params *)arg);
	return NULL;
}


#define FILE_BLOCK_LEN	4096

static int makewav(char *wav_file, char *raw_file,
	    int channels, int speed, int format) {
	int wav_fd = -1;
	int file_len = 0;
	int raw_fd = -1;
	struct stat stats;
	int head_len = 0;
	int len = 0;
	unsigned char wav_head[WAV_HEAD_SIZE];
	unsigned char *wav_head2 = wav_head;
	unsigned char *buff = NULL;
	int ret = 0;
	int format_bits;

	if ((wav_file == NULL) || (wav_file[0] == '\0')) {
		fprintf(stderr, "wav file is not specified!\n");
		return -1;
	}

	if ((raw_file == NULL) || (raw_file[0] == '\0')) {
		fprintf(stderr, "raw file is not specified!\n");
		return -1;
	}

	wav_fd = open_file(wav_file, O_WRONLY | O_CREAT);
	if (wav_fd < 0) {
		fprintf(stderr, "open wav file %s failed.\n", wav_file);
		return -1;
	}

	if ((lstat(raw_file, &stats) == 0) &&
	    S_ISREG(stats.st_mode)) {
		file_len = (int)stats.st_size;
		sndkit_dbg("CONVERT: file len is %d\n", file_len);
	} else {
		fprintf(stderr, "CONVERT: can not stat %s\n", raw_file);
		return -1;
	}

	raw_fd = open_file(raw_file, O_RDONLY);
	if (raw_fd < 0) {
		fprintf(stderr, "open raw file %s failed.\n", raw_file);
		return -1;
	}

	switch (format) {
	case FMT8BITS:
		format_bits = 8;
		break;
	case FMT16BITS:
		format_bits = 16;
		break;
	case FMT24BITS:
		format_bits = 24;
		break;
	default:
		format_bits = 16;
	}

	init_wav_header(wav_head, file_len, channels, speed, format_bits);
	head_len = WAV_HEAD_SIZE;
	while (head_len > 0) {
		if ((len = write(wav_fd, wav_head2, head_len)) < 0) {
			perror("write sound data file failed");
			return -1;
		}
		head_len -= len;
		wav_head2 += len;
	}

	buff = malloc(FILE_BLOCK_LEN);
	if (!buff) {
		fprintf(stderr, "CONVERT: alloc buffer failed.\n");
		return -1;
	}

	while (file_len > 0) {
		int wn;
		int pos;

		len = read(raw_fd, buff, FILE_BLOCK_LEN);
		if (len < 0) {
			perror("CONVERT: read raw file failed.\n");
			ret = len;
			goto out;
		}

		if (len == 0)
			goto out;

		file_len -= len;

		wn = len;
		pos = 0;

		while (wn > 0) {
			if ((len = write(wav_fd, buff + pos, wn)) < 0) {
				perror("CONVERT: write wav file failed.\n");
				ret = len;
				goto out;
			}

			wn -= len;
			pos += len;
		}
	}

 out:
	free(buff);
	return ret;
}

static int strip_wav(char *raw_file, char *wav_file) {
	int wav_fd;
	int raw_fd;
	int ret = 0;
	unsigned char *buff = NULL;
	int wav_data_len = 0;
	int len = 0;
	unsigned int snd_chn = 0;
	unsigned int snd_spd = 0;
	unsigned int snd_fmt = 0;
	int head_len;

	if ((wav_file == NULL) || (wav_file[0] == '\0')) {
		fprintf(stderr, "wav file is not specified!\n");
		return -1;
	}

	if ((raw_file == NULL) || (raw_file[0] == '\0')) {
		fprintf(stderr, "raw file is not specified!\n");
		return -1;
	}

	wav_fd = open_file(wav_file, O_RDONLY);
	if (wav_fd < 0) {
		fprintf(stderr, "open wav file %s failed.\n", wav_file);
		return -1;
	}

	raw_fd = open_file(raw_file, O_WRONLY | O_CREAT);
	if (raw_fd < 0) {
		fprintf(stderr, "open raw file %s failed.\n", raw_file);
		return -1;
	}

	buff = malloc(FILE_BLOCK_LEN);
	if (!buff) {
		fprintf(stderr, "STRIP: alloc buffer failed.\n");
		return -1;
	}

	/* the file must be at least MAX_WAV_HEAD_SIZE bytes */
	len = read(wav_fd, buff, MAX_WAV_HEAD_SIZE);
	if (len != MAX_WAV_HEAD_SIZE) {
		fprintf(stderr, "WAV file must larger than %d bytes.\n", MAX_WAV_HEAD_SIZE);
		return -1;
	} else {
		wav_data_len = parse_wav_header(buff, &head_len,
					       &snd_chn, &snd_spd, &snd_fmt);

		if (wav_data_len == 0) {
			fprintf(stderr, "the specified file is not a WAV file.\n");
			return -1;
		}

		lseek(wav_fd, head_len, SEEK_SET);
	}

	while (wav_data_len > 0) {
		int wn;
		int pos;

		len = read(wav_fd, buff, FILE_BLOCK_LEN);
		if (len < 0) {
			perror("CONVERT: read raw file failed.\n");
			goto out;
		}

		if (len == 0)
			goto out;

		wav_data_len -= len;

		wn = len;
		pos = 0;

		while (wn > 0) {
			if ((len = write(raw_fd, buff + pos, wn)) < 0) {
				perror("CONVERT: write wav file failed.\n");
				ret = len;
				goto out;
			}

			wn -= len;
			pos += len;
		}
	}
 out:
	free(buff);
	return ret;
}

int main(int argc, char *argv[])
{
	char snd_dev[128];
	int dev_fd = -1;
	int file_type = FILE_TYPE_WAV;

	int channels = DEFAULT_SND_CHN;
	int channels_user = 0;
	int format = FMT16BITS;
	int format_bits = 16;
	int format_user = 0;
	int speed = DEFAULT_SND_SPD;
	int speed_user = 0;

	unsigned int duration = DEFAULT_DURATION;
	unsigned int buffer_size = DEFAULT_BUFF_SIZE;

	int replay_enable = 0;
	char replay_file[128];
	int replay_fd = -1;
	int replay_data_len = 0;
	int save_replay = 0;
	char save_file[128];


	int record_enable = 0;
	char record_file[128];
	int record_fd = -1;

	pthread_t replay_pid;
	int replaying = 0xdead;
	pthread_t record_pid;
	int recording = 0xdead;

	pthread_t rec_replay_pid;
	int rec_replaying = 0xdead;

	int wakeup_enable = 0;
	pthread_t wakeup_pid;
	int wakingup = 0xdead;


	int vol = 0;
	int sine_freq = 1000;
	int ret = 0;
	int is_dev_reg_file;
	int convert_raw_to_wav = 0;
	int is_strip_wav = 0;
	int record_set_dmic = 0;
	int set_dmic_num = 0;

	char c;

	strcpy(snd_dev, DEFAULT_SND_DEV);
	memset(replay_file, 0, sizeof(replay_file));
	memset(record_file, 0, sizeof(record_file));

	while (1) {
		c = getopt_long(argc, argv, optstring, long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage();
			return 0;

		case 'V':
			printf(SNDKIT_VERSION);
			return 0;

		case 'W':
			convert_raw_to_wav = 1;
			break;

		case 'T':
			is_strip_wav = 1;
			break;

		case 'D':
			if (strlen(optarg) > 127) {
				printf("file name is too long: %s\n", optarg);
				return 127;
			}
			strncpy(snd_dev, optarg, strlen(optarg));
			break;
		case 'm':
			record_set_dmic = 1;
			set_dmic_num = atoi(optarg);
			if (set_dmic_num == 0) {
				printf("set dmic num error: %d\n", set_dmic_num);
				return 127;
			}

			break;

		case 't':
			if (!strcmp("wav", optarg))
				file_type = FILE_TYPE_WAV;
			else if (!strcmp("raw", optarg))
				file_type = FILE_TYPE_RAW;
			else {
				printf("unrecognized file type: %s\n", optarg);
				return 127;
			}
			break;

		case 'c':
			channels = atoi(optarg);
			channels_user = 1;
			break;

		case 'f':
			format = fmtstr_to_fmt(optarg);
			if (format < 0) {
				printf("unrecognized format: %s\n", optarg);
				return 127;
			}
			format_user = 1;
			break;

		case 'r':
			speed = atoi(optarg);
			if (!rate_is_supported(speed)) {
				printf("unrecognized sample rate: %d\n", speed);
				return 127;
			}
			speed_user = 1;
			break;

		case 'F':
			sine_freq = atoi(optarg);
			if (sine_freq == 0) {
				printf("sine wave frequency can not be 0.\n");
				return 127;
			}
			break;

		case 'd':
			duration = atoi(optarg);
			if (duration == 0) {
				printf("duration is 0, what do you want to do?\n");
				return 127;
			}
			break;

		case 'B':
			buffer_size = atoi(optarg);
			break;

		case 'v':
			debug_level++;
			break;

		case 'P':
			replay_enable = 1;
			break;

		case 'p':
			if (strlen(optarg) > 127) {
				printf("file name is too long: %s\n", optarg);
				return 127;
			}
			strncpy(replay_file, optarg, strlen(optarg));
			break;

		case 'S':
			save_replay = 1;
			break;

		case 's':
			if (strlen(optarg) > 127) {
				printf("file name is too long: %s\n", optarg);
				return 127;
			}
			strncpy(save_file, optarg, strlen(optarg));
			break;


		case 'R':
			record_enable = 1;
			break;

		case 'w':
			if (strlen(optarg) > 127) {
				printf("file name is too long: %s\n", optarg);
				return 127;
			}
			strncpy(record_file, optarg, strlen(optarg));
			break;

		case 'o':
			vol = atoi(optarg);
			if ((vol < 0) || (vol > 100)) {
				printf("volume must be in 0~100.\n");
				return 127;
			}
			wakeup_enable = 1;
			break;

		default:
			/* do nothing */
			printf("Unknown option '%c'\n", c);
			usage();
			return 127;
		}
	}

	if (convert_raw_to_wav)
		return makewav(record_file, replay_file, channels, speed, format);

	if (is_strip_wav)
		return strip_wav(record_file, replay_file);

	if (snd_dev[0] == '\0') {
		fprintf(stderr, "device is not specified!\n");
		return 127;
	}

	if (replay_enable && (replay_file[0] == '\0')) {
		sndkit_dbg("replay file is not specified! stdin will be used!\n");
		//return 127;
	}

	if (record_enable  && (record_file[0] == '\0')) {
		fprintf(stderr, "record file is not specified!\n");
		return 127;
	}

	/* adjust the replay volume */
	{
		int mixer_fd = -1;
		if ((mixer_fd = open("/dev/snd/mixer", O_RDWR)) < 0) {
			perror("open /dev/mixer failed--------------------.");
			exit(1);
		}
		if(replay_enable){
			/*set route*/
			write(mixer_fd, "5", 1);
			if (ioctl(mixer_fd, SOUND_MIXER_WRITE_VOLUME, &vol) == -1) {
				perror("set replay volume failed!\n");
			}
		}
		if(record_enable){
			//if (ioctl(mixer_fd, SOUND_MIXER_WRITE_MIC, &vol) == -1) {
			//perror("set record volume failed!\n");
			//}
		}
	}

	/* open device */
	if (replay_enable && record_enable) {
		sndkit_dbg("Play & Record\n");
		dev_fd = open_device(snd_dev, O_RDWR, &is_dev_reg_file);
	} else if (replay_enable) {
		sndkit_dbg("Play\n");
		dev_fd = open_device(snd_dev, O_WRONLY, &is_dev_reg_file);
	} else if (record_enable) {
		sndkit_dbg("Record\n");
		dev_fd = open_device(snd_dev, O_RDONLY, &is_dev_reg_file);
	}

	if ( (record_enable || replay_enable) && (dev_fd < 0)) {
		fprintf(stderr, "open device failed.\n");
		return -1;
	}

	/* open replay sound file */
	if (replay_enable &&  (replay_file[0] != '\0')) {
		replay_fd = open_file(replay_file, O_RDONLY);
		if (replay_fd < 0)
			return -1;
	}

	if (replay_enable &&  (replay_file[0] == '\0')) {
		replay_fd = fileno(stdin);
	}

//	printf("dao dao dao dao\n");
	/* if replay & wav, check if the file is wav file */
	if (replay_enable && (file_type == FILE_TYPE_WAV)) {
		int head_len = 0;
		int len = 0;
		int is_wav_file = 0;
		unsigned int snd_chn = 0;
		unsigned int snd_spd = 0;
		unsigned int snd_fmt = 0;
		unsigned char buff[MAX_WAV_HEAD_SIZE];

		/* the file must be at least MAX_WAV_HEAD_SIZE bytes */
		len = read(replay_fd, buff, MAX_WAV_HEAD_SIZE);
		if (len != MAX_WAV_HEAD_SIZE) {
			fprintf(stderr, "WAV file must larger than %d bytes.\n", MAX_WAV_HEAD_SIZE);
			return -1;
		} else {
			is_wav_file = parse_wav_header(buff, &head_len,
						       &snd_chn, &snd_spd, &snd_fmt);

			if (is_wav_file == 0) {
				fprintf(stderr, "the specified file is not a WAV file.\n");
				return -1;
			}

			if (format_user == 0) {
				switch (snd_fmt) {
				case 8:
					format = FMT8BITS;
					break;
				case 16:
					format = FMT16BITS;
					break;
				case 24:
					format = FMT24BITS;
					break;
				default:
					break;
				}
			}

			if (channels_user == 0)
				channels = snd_chn;

			if (speed_user == 0)
				speed = snd_spd;

			lseek(replay_fd, head_len, SEEK_SET);

			replay_data_len = is_wav_file;
		}
	}

	sndkit_dbg("%s: format = ",
	       (file_type == FILE_TYPE_WAV) ? "WAV" : "RAW"
	       );
	switch (format) {
	case FMT8BITS:
		format_bits = 8;
		break;
	case FMT16BITS:
		format_bits = 16;
		break;
	case FMT24BITS:
		format_bits = 24;
		break;
	default:
		break;
	}
	sndkit_dbg("%d", format_bits);

	sndkit_dbg(" channels = %d, speed = %d\n",
	       channels, speed);

	/* open record file */
	if (record_enable) {
		record_fd = open_file(record_file, O_WRONLY | O_CREAT);
		if (record_fd < 0)
			return -1;
	}

	/* prepare wav header */
	if (record_enable && (file_type == FILE_TYPE_WAV)) {
		int hi;
		int hn;
		unsigned char wav_head[WAV_HEAD_SIZE];
		unsigned char *wav_head2 = wav_head;

		int record_len = format_bits * channels * speed / 8 * duration;

		init_wav_header(wav_head, record_len, channels, speed, format_bits);
		hi = WAV_HEAD_SIZE;
		while(hi > 0) {
			if ((hn = write(record_fd, wav_head2, hi)) < 0) {
				perror("write sound data file failed");
				return -1;
			}
			hi -= hn;
			wav_head2 += hn;
		}
	}

	/*printf("format = %d,channels = %d,speed = %d\n",format,channels,speed);*/
	/*set sound format */
	if ( (record_enable || replay_enable) && (!is_dev_reg_file)) {
		if (config_device(dev_fd, format, channels, speed) < 0)
			return -1;
	}

	/* Now everything is ok, do real stuffs */
	if (replay_enable) {
		replay_params.dev_fd = dev_fd;
		replay_params.replay_fd = replay_fd;
		replay_params.buffer_size = buffer_size;
		replay_params.replay_data_len = replay_data_len;
		replay_params.format = format;
		replay_params.channels = channels;
		replay_params.speed = speed;
		replay_params.file_type = file_type;

		replaying = pthread_create(&replay_pid, NULL, replay_thread, &replay_params);
		if (replaying != 0) {
			perror("PLAY: start replay thread failed.");
		}
	}

	if (record_enable) {
		if (replay_enable)
			sleep(1);

		record_params.dev_fd = dev_fd;
		record_params.record_fd = record_fd;
		record_params.buffer_size = buffer_size;
		record_params.duration = duration;
		record_params.format = format;
		record_params.channels = channels;
		record_params.speed = speed;
		record_params.file_type = file_type;
		record_params.record_set_dmic = record_set_dmic;
		record_params.set_dmic_num = set_dmic_num;

		recording = pthread_create(&record_pid, NULL, record_thread, &record_params);
		if (recording != 0) {
			perror("REC: start record thread failed.");
		}
	}


	int fd_res, fd_sys_set, fd_sys_ctl, fd_wakeup_reading;
	fd_res = open(WAKEUP_SRC_FILE, O_RDWR);
	if(fd_res < 0) {
		perror("open fd res faild\n");
		return -1;
	}
	fd_sys_set = open(SYSFS_SET_RES_NODE, O_RDWR);
	if(fd_sys_set < 0) {
		perror("open sys set failed \n");
		close(fd_res);
		return -1;
	}
	fd_sys_ctl = open(SYSFS_CTL_NODE, O_RDWR);
	if(fd_sys_ctl < 0) {
		perror("open sys ctl failed\n");
		close(fd_res);
		close(fd_sys_set);
		return -1;
	}

	wakeup_params.fd_res = fd_res;
	wakeup_params.fd_sys_set = fd_sys_set;
	wakeup_params.fd_sys_ctl = fd_sys_ctl;
	wakeup_params.fd_wakeup_reading = fd_wakeup_reading;

	//wakeup_parmas.
	//wakeup_enable = 0;
	if (wakeup_enable) {

		wakingup = pthread_create(&wakeup_pid, NULL, wakeup_thread, &wakeup_params);
		if( wakingup != 0) {
			perror("WAKUP: start wakeup thread failed.");
		}

	}


	if (recording == 0) {
		pthread_join(record_pid, NULL);
	}
	/*
	 * disable wakeup module if enabled;
	 * */
	if (wakeup_enable) {
		printf("disable wakeup module!\n");
		write(fd_sys_ctl, SYSFS_CTL_DISABEL_WAKEUP, 1);
	}
	printf("join record\n");
	if (replaying == 0) {
		pthread_join(replay_pid, NULL);
	}
	printf("join replay\n");

	if (wakingup == 0) {
		pthread_join(wakeup_pid, NULL);
	}
	close(fd_res);
	close(fd_sys_set);
	close(fd_sys_ctl);
	close(fd_wakeup_reading);
	printf("join wakeup\n");

	if (record_enable) {
		ret = close(dev_fd);
		if (ret)
			printf("rdev errno: %d\n", ret);
		else
			printf("rdev success: %d\n", ret);
	}

	return ret;
}
