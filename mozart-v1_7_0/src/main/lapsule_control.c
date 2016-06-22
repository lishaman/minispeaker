#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include "utils_interface.h"
#include "sharememory_interface.h"
#if (SUPPORT_DMR == 1)
#include "render_interface.h"
#endif
#include "lapsule_control.h"

#define DEBUG(x,y...)	{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}
#define ERROR(x,y...)	{printf("[ %s : %s : %d] ",__FILE__, __func__, __LINE__); printf(x,##y); printf("\n");}

#define LAPSULE_SOCK_ADDR "127.0.0.1"
#define LAPSULE_SOCK_PORT 10098

#define LAPSULE_PLAY "{\"command\": \"play\"}"
#define LAPSULE_STOP "{\"commands\": [{\"command\": \"set_play_next\",\"is_play_next\": false},{\"command\": \"stop\"}]}"
#define LAPSULE_WAKEUP "{\"commands\": [{\"command\": \"set_play_next\",\"is_play_next\": true},{\"command\": \"play\"}]}"
#define LAPSULE_PAUSE "{\"command\": \"pause\"}"
#define LAPSULE_TOGGLE "{\"command\": \"toggle\"}"
#define LAPSULE_SKIP "{\"command\": \"skip\"}"
#define LAPSULE_PREV "{\"command\": \"prefix\"}"
#define LAPSULE_SET_VOLUME "{\"command\": \"set_volume\", \"volume\": %d}"
#define LAPSULE_GET_VOLUME "{\"command\": \"get_volume\"}"
#define LAPSULE_SEARCH_SONG "{\"commands\": [{\"command\": \"set_play_next\",\"is_play_next\": true},{\"command\": \"search\", \"keyword\": \"%s\"}]}"
#define LAPSULE_MUSIC_LIST "{\"command\": \"play_default_channel\",\"index\": %d}"
#define LAPSULE_NOTIFY_LINEIN_ON "{\"command\": \"set_linein_mode\",\"mode\": 1}"
#define LAPSULE_NOTIFY_LINEIN_OFF "{\"command\": \"set_linein_mode\",\"mode\": 0}"
#define RESULT_SIZE 128

#define LAPSULE_PLAY_FM "{\"command\":\"replace_queue\",\"songs\":[{\"song_id\":\"43645\",\"ssid\":\"\",\"song_name\":\"姊佽緣璇存硶\",\"songurl\":\"http:\/\/live.xmcdn.com\/live\/61\/64.m3u8\",\"artists_name\":\"姊佽緣璇存硶\",\"albumsname\":\"姊佽緣璇存硶\",\"picture\":\"http:\/\/fdfs.xmcdn.com\/group6\/M04\/88\/9E\/wKgDg1T_oZ3gW19IAADOaANCnpQ836_mobile_large.jpg\",\"like\":0,\"duration\":0}],\"index\":0}"


static int lapsule_socket = -1;

static void close_lapsule_socket(void)
{
	if(lapsule_socket > 0){
		close(lapsule_socket);
		lapsule_socket = -1;
	}
}

static void *lapsule_autoplay(void *args)
{
	int i = 0;
	module_status status;

	sleep(3);

	share_mem_get(SDCARD_DOMAIN, &status);
	if (status == STATUS_EXTRACT) {
		for (i = 0; i < 10; i++) {
			if (!lapsule_do_wakeup())
				break;
			else
				usleep(100 * 1000);
		}
		if (i == 10) {
			printf("start network playback error.\n");
		} else {
			printf("No sdcard found, start network playback.\n");
		}
	} else if (status == STATUS_INSERT) {
		printf("sdcard found, will not play network music.\n");
	}

	return NULL; // make compile happy.
}

int start_lapsule_app(void)
{
	pthread_t autoplay_thread;

	if (mozart_system("lapsule -k ead13802422e11e5828bb8f6b1234567 -s 12313802422e11e5828bb8f6b1137a5d > /dev/null &")) {
		return errno;
	}
#if 0
	if (pthread_create(&autoplay_thread, NULL, lapsule_autoplay, NULL)) {
		return errno;
	}
	pthread_detach(autoplay_thread);
#endif
	return 0;
}

void stop_lapsule_app(void)
{
#if (SUPPORT_DMR == 1)
	control_point_info *info = NULL;
	memory_domain domain;
#endif

#if (SUPPORT_DMR == 1)
	ret = share_mem_get_active_domain(&domain);
	if(0 == ret && RENDER_DOMAIN == domain) {
		info = mozart_render_get_controler();
		if(strcmp(LAPSULE_PROVIDER, info->music_provider) == 0){		//is lapsule in control
			lapsule_do_stop_play();
		}
	}
#endif
	close_lapsule_socket();

	mozart_system("killall lapsule");
	mozart_system("rm -f /mnt/vendor/lapsule_dist/db/is_restart");
}

static int connect_to_lapsule(void)
{
	int sockfd = -1;
	struct sockaddr_in lapsule_addr;

	if(lapsule_socket > -1){
		return lapsule_socket;
	}

	if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))){
		return -1;
	}

	lapsule_addr.sin_family = AF_INET;
	lapsule_addr.sin_port = htons(LAPSULE_SOCK_PORT);
	lapsule_addr.sin_addr.s_addr = inet_addr(LAPSULE_SOCK_ADDR);
	if (-1 == connect(sockfd, (struct sockaddr *)&lapsule_addr, sizeof(lapsule_addr))){
		close(sockfd);
		return -1;
	}

	return sockfd;
}

static int send_command(char *command)
{
	int rs = 0;
	rs = send(lapsule_socket, command, strlen(command) + 1, 0);
	if(rs == strlen(command) + 1){
		return 0;
	}
	ERROR("send:%s", strerror(errno));
	if(EBADF == errno || ENOTCONN == errno || ENOTSOCK == errno){
		return -2;
	}
	return -1;
}

int recv_result(char *result)
{
	if(-1 == recv(lapsule_socket, result, RESULT_SIZE, 0)){
		ERROR("recv:%s", strerror(errno));
		return -1;
	}
	return 0;
}

int lapsule_do_play(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_PLAY)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_pause(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_PAUSE)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_prev_song()
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_PREV)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_next_song(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_SKIP)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_toggle(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}
	if(0 != send_command(LAPSULE_TOGGLE)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_stop_play(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_STOP)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_wakeup(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_WAKEUP)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_set_vol(int vol)
{
	char cmd[128] = {0};

	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	sprintf(cmd, LAPSULE_SET_VOLUME, vol);
	if(0 != send_command(cmd)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_search_song(char *song)
{
	char * cmd = NULL;
	int len = 0;
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}
	len = strlen(song) + strlen(LAPSULE_SEARCH_SONG);
	cmd = (char *)malloc(len);
	memset(cmd,0,len);
	sprintf(cmd, LAPSULE_SEARCH_SONG, song);
	if(0 != send_command(cmd)){
		close_lapsule_socket();
		if(cmd != NULL){
			free(cmd);
			cmd = NULL;
		}
		return -1;
	}
	if(cmd != NULL){
		free(cmd);
		cmd = NULL;
	}
	return 0;
}
 #define MUSIC_FILE_PATH "/usr/data/musiclist"

int lapsule_play_music_list(int index){

	char filepath[128] = {0};
	        FILE *fd;
		int i;
	memset(filepath,0,128);
	sprintf(filepath,"%s%d", MUSIC_FILE_PATH, index);
	printf("file path is %s\n",filepath);

		if ((fd = fopen(filepath, "r")) == NULL) {
			printf("error open file:.\n");
			return;
		}
		int file_size;
		fseek(fd,0,SEEK_END);
		file_size = ftell(fd);
		fseek(fd,0,SEEK_SET);

		printf("file size id %d\n",file_size);

		char *buff = malloc(file_size);
		memset(buff,0,file_size);
		fread(buff,file_size,1,fd);
//		printf("read file ok! %s\n",buff);

		struct json_object *play_command = NULL;
		play_command = json_tokener_parse(buff);
		struct json_object *tmp = NULL;
		json_object_object_get_ex(play_command, "playcommand", &tmp);
		char * result = json_object_get_string(tmp);
//		printf("playcommand is %s\n",result);

		if(0 != send_command(result)){
			printf("error send command!\n");
			close_lapsule_socket();
			json_object_put(play_command);
			result = NULL;
			free(buff);
			fclose(fd);
			return -1;
		}

		json_object_put(play_command);
		result = NULL;
		free(buff);
		fclose(fd);
		return 0;
}


void lapsule_play_music_list1(){

	FILE *fd;
	int i;
	if ((fd = fopen(MUSIC_FILE_PATH, "r")) == NULL) {
		printf("error open file: %s.\n", strerror(errno));
		return;
	}
	int file_size;
	fseek(fd,0,SEEK_END);
	file_size = ftell(fd);
	fseek(fd,0,SEEK_SET);


	printf("file size id %d\n",file_size);

	char *buff = malloc(file_size);
	memset(buff,0,file_size);
	fread(buff,file_size,1,fd);
	printf("read file ok! %s\n",buff);

	struct json_object *play_command = NULL;

	play_command = json_tokener_parse(buff);

	struct json_object *tmp = NULL;
        json_object_object_get_ex(play_command, "mCoverUrl", &tmp);
	char * result = json_object_get_string(tmp);
	printf("mCoverUrl is %s\n",result);

	json_object_object_get_ex(play_command, "mType", &tmp);
        result = json_object_get_string(tmp);
	printf("type is %s\n",result);

	      json_object_object_get_ex(play_command, "mQulickListSongs", &tmp);
	      result = json_object_get_string(tmp);
	      printf("mQulickListSongs is %s\n",result);

	      struct json_object *obj, *temp_obj;

	      obj = json_tokener_parse(result);
		int count = json_object_array_length(obj);
	printf("song list number is %d\n",count);

	for(i=0 ; i<json_object_array_length(obj) ; i++ ){
		temp_obj = json_object_array_get_idx(obj, i );
		json_object_object_get_ex(temp_obj, "mCoverUrl", &tmp);
		result = json_object_get_string(tmp);
		printf("song mCoverUrl is i %d %s\n",i,result);

		json_object_object_get_ex(temp_obj, "mPlayUrl", &tmp);
		result = json_object_get_string(tmp);
		printf("song mPlayUrl is i %d %s\n",i,result);

	}
	json_object_put(play_command);
	free(buff);
	fclose(fd);
}

int lapsule_do_music_list(int keyIndex)
{
	char cmd[128] = {0};

	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	printf("jim gao indexkey is %d\n",keyIndex);
	if(keyIndex == 6){
		printf("start play FM!\n");
		    if(0 != send_command(LAPSULE_PLAY_FM)){
			                     close_lapsule_socket();
			                     return -1;
			         }
	}
	else{
	//	sprintf(cmd, LAPSULE_MUSIC_LIST, keyIndex);
	//	printf("cmd is %s\n",cmd);
	//	if(0 != send_command(cmd)){
	//		close_lapsule_socket();
	//		return -1;
	//	}
		int index = keyIndex + 1;
		lapsule_play_music_list(index);
	}
	return 0;
}

int lapsule_do_linein_on(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_NOTIFY_LINEIN_ON)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}

int lapsule_do_linein_off(void)
{
	lapsule_socket = connect_to_lapsule();
	if(lapsule_socket < 0){
		ERROR("connect_to_lapsule failure.");
		return -1;
	}

	if(0 != send_command(LAPSULE_NOTIFY_LINEIN_OFF)){
		close_lapsule_socket();
		return -1;
	}

	return 0;
}
