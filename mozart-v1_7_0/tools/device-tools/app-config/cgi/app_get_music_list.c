#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#include "utils_interface.h"
#include "cgi.h"
#include "cJSON.h"


#define TRUE 1
#define FALSE 0
#define MAXPATH 1024

typedef enum {
	NO_STORAGE = 0,
	SDCARD,
	UDISK,
} storage_type;

typedef struct music_item_s {
	char *musicUrl;
	storage_type storage_type;
	struct music_item_s *next;
	struct music_item_s *prev;
} music_item;

typedef struct dir_item_s {
	char dir_name[1024];
	struct dir_item_s *next;
} dir_item;
typedef music_item *music_list;
typedef dir_item *dir_list;

/**
 * @brief udisk playlist.
 */
static music_list udisk_music_list = NULL;
/**
 * @brief sdcard playlist.
 */
static music_list sdcard_music_list = NULL;
/**
 * @brief The currently playing song.
 */
static music_list current_item = NULL;


/**
 * @brief Local supported audio formats.
 */
static char *audios[] =
    { ".3gp", ".aac", ".ac3", ".aif", ".aiff", ".mp1", ".mp2",
	".mp3", ".mp4", ".m4a", ".ogg", ".wav",
	".wma", ".mka", ".ra", ".rm", "flac", ".ape", NULL
};

static const char *ep;




/**
 * @brief To determine whether the specified file is an audio file.
 *
 * @param fileName Specified file.
 *
 * @return Returns true if the specified file is an audio file, and false
 * indicates that the specified file is not an audio file.
 */
int is_audio_file(char *fileName)
{
	int i = 0, len = 0;
	for (i = 0; audios[i]; i++) {
		len = strlen(fileName) - strlen(audios[i]);
		if (len < 0) {
			return FALSE;
		}
		// 比较扩展名
		if (strcmp(fileName + len, audios[i]) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @brief Empty the specified playlist.
 *
 * @param dest specified playlist.
 */
void remove_play_list(music_list * dest)
{
	if (*dest == NULL)
		return;
	music_item *p1 = NULL, *tmp = NULL;
	p1 = *dest;
	do {
		tmp = p1;
		p1 = p1->next;
		if (tmp->musicUrl) {
			free(tmp->musicUrl);
		}
		free(tmp);
	} while (p1 != *dest);
	*dest = NULL;
}

/**
 * @brief Recursively get all the audio files in the specified directory, all
 * the supported audio files stored in the specified circular list.
 *
 * @param dir Specified directory.
 * @param stoType Storage device type.
 */
void scan_dir(char *dir, storage_type stoType)
{
	// 创建JSON Array
	 cJSON* pArray = cJSON_CreateArray();

	DIR *dp = NULL;
	struct dirent *entry;
	struct stat statbuff;
	// p1指向当前找到的项即最后一个要插入的
	// p2指向链表的头
	music_item *p2, *p1;
	p2 = NULL;
	dir_item *tmpDir, *first, *last;
	int count = 0;
	int hasError = 0;
	music_list *dest = NULL;

	if (SDCARD == stoType)
		dest = &sdcard_music_list;
	else if (UDISK == stoType)
		dest = &udisk_music_list;
	else {
		printf("Unknow storage_type.");
		return;
	}

	first = (dir_item *) malloc(sizeof(dir_item));
	memset(first, 0, sizeof(dir_item));
	strcpy(first->dir_name, dir);
	first->next = NULL;
	tmpDir = last = first;

	while (first) {
		chdir(first->dir_name);
		if (!(dp = opendir(first->dir_name))) {
			printf("Can't open dir:%s.", first->dir_name);
			hasError = 1;
			break;
		}

		char path_buff[MAXPATH];
		strcpy(path_buff, first->dir_name);
		strcat(path_buff, "/");

		while ((entry = readdir(dp)) != NULL) {
			int err;
			err = lstat(entry->d_name, &statbuff);	// 获取下一级成员属性
			if (err < 0) {
				perror("Get lstat");
				hasError = 1;
				break;
			}

			//忽略隐藏文件.和..
			if (strcmp(".", entry->d_name) == 0
			    || strcmp("..", entry->d_name) == 0)
				continue;

			if (S_IFDIR & statbuff.st_mode) {
				tmpDir = (dir_item *) malloc(sizeof(dir_item));
				memset(tmpDir, 0, sizeof(dir_item));
				strcpy(tmpDir->dir_name, path_buff);
				strcpy(tmpDir->dir_name +
				       strlen(tmpDir->dir_name), entry->d_name);
				tmpDir->next = NULL;
				last->next = tmpDir;
				last = tmpDir;
			} else {
				if (is_audio_file(entry->d_name)) {
					p1 = (music_item *)
					    malloc(sizeof(music_item));
					memset(p1, 0, sizeof(music_item));
					p1->musicUrl =
					    (char *)malloc(strlen(entry->d_name)
							   + strlen(path_buff) +
							   1);
					memset(p1->musicUrl, 0,
					       (strlen(entry->d_name) +
						strlen(path_buff) + 1));
					memcpy(p1->musicUrl, path_buff,
					       strlen(path_buff));
					memcpy(p1->musicUrl +
					       strlen(p1->musicUrl),
					       entry->d_name,
					       strlen(entry->d_name));
					p1->storage_type = stoType;

					p1->next = 0;
					p1->prev = 0;
					count++;

					cJSON *pItem = cJSON_CreateObject();
					cJSON_AddStringToObject(pItem,"name",entry->d_name);
					cJSON_AddStringToObject(pItem,"url",p1->musicUrl);
					cJSON_AddItemToArray(pArray, pItem);

					if (count == 1) {
						*dest = p2 = p1;
					} else {
						p2->next = p1;
						p1->prev = p2;
					}
					p1->next = *dest;
					(*dest)->prev = p1;
					p2 = p1;
				}
			}
		}
		tmpDir = first;
		first = first->next;
		free(tmpDir);
		closedir(dp);
	}

// 打印JSON数据包
    char *out = cJSON_Print(pArray);
 	printf("%s\n",out);
	// 释放内存
    cJSON_Delete(pArray);
    free(out);

	chdir("/");
	if (hasError) {
		while (first) {
			tmpDir = first;
			first = first->next;
			free(tmpDir);
		}
		remove_play_list(dest);
	}
	//printf("the current dir have Audio file is %d.", count);
}

int main(void)
{
	cgi_init();
	cgi_process_form();
	cgi_init_headers();

	if(mozart_path_is_mount("/mnt/sdcard"))
   		scan_dir("/mnt/sdcard", SDCARD);
	else
		printf("sdout");
	cgi_end();

	return 0;
}
