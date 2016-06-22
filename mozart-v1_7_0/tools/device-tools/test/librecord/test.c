#include <stdio.h>

#include "record_interface.h"

/********以下是wave格式文件的文件头格式说明******/
/*------------------------------------------------
|             RIFF WAVE Chunk                  |
|             ID = 'RIFF'                     |
|             RiffType = 'WAVE'                |
------------------------------------------------
|             Format Chunk                     |
|             ID = 'fmt '                      |
------------------------------------------------
|             Fact Chunk(optional)             |
|             ID = 'fact'                      |
------------------------------------------------
|             Data Chunk                       |
|             ID = 'data'                      |
------------------------------------------------*/
/**********以上是wave文件格式头格式说明***********/
/*wave 文件一共有四个Chunk组成，其中第三个Chunk可以省略，每个Chunk有标示（ID）,大小（size,就是本Chunk的内容部分长度）,内容三部分组成*/
typedef struct waveheader
{
	/****RIFF WAVE CHUNK*/
	unsigned char a[4];//四个字节存放'R','I','F','F'
	long int b;        //整个文件的长度-8;每个Chunk的size字段，都是表示除了本Chunk的ID和SIZE字段外的长度;
	unsigned char c[4];//四个字节存放'W','A','V','E'
	/****Format CHUNK*/
	unsigned char d[4];//四个字节存放'f','m','t',''
	long int e;       //16后没有附加消息，18后有附加消息；一般为16，其他格式转来的话为18
	short int f;       //编码方式，一般为0x0001;
	short int g;       //声道数目，1单声道，2双声道;
	long int h;        //采样频率;
	long int i;        //每秒所需字节数;
	short int j;       //每个采样需要多少字节，若声道是双，则两个一起考虑;
	short int k;       //即量化位数
	/***Data Chunk**/
        unsigned char p[4];//四个字节存放'd','a','t','a'
	long int q;        //语音数据部分长度，不包括文件头的任何部分
} waveheader;//定义WAVE文件的文件头结构体


waveheader *get_waveheader(int bits, int rates, int channels, unsigned long size)
{
        waveheader *header = malloc(sizeof(*header));

	header->a[0] = 'R';
	header->a[1] = 'I';
	header->a[2] = 'F';
	header->a[3] = 'F';
	header->b = size - 8;
	header->c[0] = 'W';
	header->c[1] = 'A';
	header->c[2] = 'V';
	header->c[3] = 'E';
	header->d[0] = 'f';
	header->d[1] = 'm';
	header->d[2] = 't';
	header->d[3] = ' ';
	header->e = 16;
	header->f = 1;
	header->g = channels;
	header->h = rates;
	header->i = size / (rates * channels * bits / 8);
	header->j = channels * bits / 8;
	header->k = bits;
	header->p[0] = 'd';
	header->p[1] = 'a';
	header->p[2] = 't';
	header->p[3] = 'a';
	header->q = size;

        return header;
}

static void Usage(char *name)
{
        printf("%s - librecord test program.\n"
               "Usage:\n"
               " %s <-b bits> <-r rates> <-c channels> <-v volume> <-t seconds> <-f /tmp/record.wav> <-h>\n"
               "Example:\n"
               " %s -b 16 -r 16000 -c 1 -v 100 -t 10 -f /tmp/record.wav\n", name, name, name);

        return;
}

int main(int argc, char **argv)
{
        unsigned long size = 0;
        unsigned long len = 0;
        char *buf = NULL;
        char *p = NULL;
        int fd = -1;

        unsigned long left = 0;
        unsigned long cnt = 0;
        int c = 0;

        int time = -1;
        char *file = NULL;
        record_param param = {-1, -1, -1, -1};

        for (;;) {
                c = getopt(argc, argv, "b:r:c:v:t:f:h");
                if (c < 0)
                        break;
                switch (c) {
                case 'b':
                        param.bits = atoi(optarg);
                        break;
                case 'r':
                        param.rates = atoi(optarg);
                        break;
                case 'c':
                        param.channels = atoi(optarg);
                        break;
                case 'v':
                        param.volume = atoi(optarg);
                        break;
                case 't':
                        time = atoi(optarg);
                        break;
                case 'f':
                        file = optarg;
                        break;
                case 'h':
                        Usage(argv[0]);
                        return 0;
                default:
                        Usage(argv[0]);
                        return -1;
                }
        }

        if (param.bits == -1 || param.rates == -1 || param.channels == -1 || param.volume == -1 || time == -1) {
                Usage(argv[0]);
                return -1;
        }

        if (file)
                file = "/tmp/record.wav";

        // init record device.
        mic_record *record = mozart_soundcard_init(param);
        if (!record) {
                printf("mozart_sound_card_init error, Cann't record.\n");
                return -1;
        }

        // dump record format.
        printf("record param: bits: %d, rates: %u, channels: %u, volume: %d, time: %ds, file: %s\n",
               record->param.bits, record->param.rates, record->param.channels, record->param.volume, time, file);

        // alloc memory for <time> seconds's record data.
        size = 10 * (record->param.bits / 8 ) * record->param.channels * record->param.rates;
        buf = malloc(size);

        // record.
        len = mozart_record(record, buf, size);

        // make the wav file header.
        waveheader *header = get_waveheader(16, 16000, 1, len);

        // create wav file.
        fd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
        if (fd < 0) {
                printf("Cann't create %s: %s.\n", file, strerror(errno));
                return -1;
        }

        // write wav file header.
        write(fd, header, sizeof(*header));
        free(header);
        header = NULL;

        // write pcm data.
        left = len;
        p = buf;
        while (left > 0) {
                cnt = left > 10240 ? 10240 : left;
                cnt = write(fd, p, cnt);
                if (cnt < 0) {
                        printf("write error, retry.\n");
                        usleep(100 * 10000);
                }

                left -= cnt;
                p += cnt;
        }

        close(fd);
        free(buf);

        printf("Saved record data to %s, ByeBye~~~\n", file);

        return 0;
}
