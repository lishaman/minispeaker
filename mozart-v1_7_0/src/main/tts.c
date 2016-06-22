#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define BAIDU_TTS_URL "http://tts.baidu.com/text2audio?cuid=48-5A-B6-47-0A-BB&lan=zh&ctp=1&pdt=90&tex="

#define BAIDU 1
#define SPEECH 2

#define TTS_PROVIDER BAIDU
#define TTS_DEBUG

static char *encode_url(char *url)
{
	int i = 0;
	char *dst = NULL;

	if (!url)
		return NULL;

	dst = malloc(strlen(url) * 3 + 1);
	if (!dst)
		return NULL;
	dst[0] = '\0';

	for (i = 0; url[i]; i++) {
		switch (url[i]) {
		case ' ':
			strcat(dst, "%20");
			break;
		default:
			strncat(dst, url + i, 1);
			break;
		}
	}

	return dst;
}

static char *get_baidu_tts_url(const char *word)
{
	char *url = NULL;
	char *url_encoded = NULL;

	url = malloc(strlen(BAIDU_TTS_URL) + strlen(word) + 1);
	if (!url) {
		printf("malloc for baidu-tts-url error: %s.\n", strerror(errno));
		return NULL;
	}

	sprintf(url, "%s%s", BAIDU_TTS_URL, word);

	url_encoded = encode_url(url);

	free(url);
	url= NULL;

	return url_encoded;
}

static void _mozart_tts(const char *word, bool sync)
{
	char *url = NULL;

	if (word && *word) {
		switch (TTS_PROVIDER) {
		case BAIDU:
			url = get_baidu_tts_url(word);
			break;
		case SPEECH:
			break;
		}
#ifdef TTS_DEBUG
		if (url) {
			printf("%s\n", url);
		} else {
			printf("tts error.\n", url);
			return;
		}
#endif
		if (sync)
			mozart_play_tone_sync(url);
		else
			mozart_play_tone(url);

		free(url);
		url = NULL;
	}
}

void mozart_tts(const char *word)
{
	_mozart_tts(word, false);
}

void mozart_tts_sync(const char *word)
{
	_mozart_tts(word, true);
}

void mozart_tts_wait(void)
{
	mozart_play_tone_sync(NULL);
}
