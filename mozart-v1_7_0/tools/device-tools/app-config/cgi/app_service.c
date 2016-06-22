#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "wifi_interface.h"

#define PORT 8723
#define BUFSIZE 512

#define SSIDMAXLEN 32
#define PASSWDMINLEN 8
#define PASSWDMAXLEN 63

#define AP_FILE "/usr/data/ap_name"
#define WPA_FILE "/usr/data/wpa_supplicant.conf"

#define AP_CONF \
	"encrypt=%s\n" \
	"SSID=%s\n" \
	"PASSWD=%s\n"

#define WPA_CONF \
	"ctrl_interface=/var/run/wpa_supplicant\n" \
	"ap_scan=1\n" \
	"network={\n" \
	"\t%skey_mgmt=NONE\n" \
	"\tssid=\"%s\"\n" \
	"\t%spsk=\"%s\"\n" \
	"}\n"

enum{
	OK = 0,
	ILLEGAL_CMD,
	ILLEGAL_SSID,
	ILLEGAL_PASSWD,
	INTERNAL_ERROR,
	SAVEFILE_ERROR,
};

const char *err[] = {
	"OK",
	"ILLEGAL_CMD",
	"ILLEGAL_SSID",
	"ILLEGAL_PASSWD",
	"INTERNAL_ERROR",
	"SAVEFILE_ERROR",
};

/*
 * Format:
 *   CMD SSID PASSWD
 *
 * config sta:
 *   wpa/wpa2 wifi:
 *     STA JZ_Guest #wwwingenic*
 *   open wifi:
 *     STA wifi-audio NULL
 *
 * config ap:
 *   encrypt ap:
 *     AP hehe 12345678
 *   open ap:
 *     AP hehe NULL
 *
 * switch wifi mode:
 *   switch to ap:
 *     AP NULL NULL
 *   switchap to sta:
 *     STA NULL NULL
 *
 * */

int callback_handle_func(const char *p)
{
	printf(">>>>>>Hello,the change process of the network is %s<<<<<<\n",p);
	return 0;
}

void doit(int fd)
{
	int ret = -1;

	char buffer[BUFSIZE+1] = {};
	char *cmd = NULL;
	char *ssid = NULL;
	char *passwd = NULL;

	int errcode = OK;

	ret =recv(fd, buffer, BUFSIZE, 0);
	if(ret == -1 || ret == 0) { /* read failure stop now */
		printf("failed to read() in server: %s\n", strerror(errno));
		errcode = INTERNAL_ERROR;
		goto process_and_exit;
	}

#if 1 // DEBUG
	printf("\n\n---------recieved data---------\n"
	       "'%s'"
	       "\n---------recieved data---------\n\n", buffer);
#endif

	if(ret > 0 && ret < BUFSIZE)
		buffer[ret]='\0';
	else
		buffer[0]='\0';

	cmd = strtok(buffer, " ");
	if(cmd){
		ssid = strtok(NULL, " ");
		if(ssid){
			passwd = strtok(NULL, " ");
			if(!passwd){
				errcode = ILLEGAL_PASSWD;
				goto process_and_exit;
			}
		}
		else{
			errcode = ILLEGAL_SSID;
			goto process_and_exit;
		}
	}
	else{
		printf("recieved unknow request\n");
		errcode = ILLEGAL_CMD;
		goto process_and_exit;
	}

#if 1 // DEBUG
	printf("cmd: '%s'\n"
	       "ssid: '%s'\n"
	       "passwd: '%s'\n", cmd, ssid, passwd);
#endif

	//1. check cmd
	if(!strncmp(cmd, "AP", 2)){
		printf("recieved ap request\n");
	}
	else if(!strncmp(cmd, "STA", 3)){
		printf("recieved sta request\n");
	}
	else{
		printf("recieved unknow request\n");
		errcode = ILLEGAL_CMD;
		goto process_and_exit;
	}

	//2. check ssid
	if(!strcmp("NULL", ssid))
		goto process_and_exit;
	else{
		if(strlen(ssid) > (SSIDMAXLEN-strlen("SmartAudio-"))){
			errcode = ILLEGAL_SSID;
			goto process_and_exit;
		}
	}

	//3. check passwd
	if(strcmp(passwd, "NULL")){
		if((strlen(passwd) < PASSWDMINLEN || strlen(passwd) > PASSWDMAXLEN)){
			errcode = ILLEGAL_PASSWD;
			goto process_and_exit;
		}
	}

	// doit
	if(!strcmp("AP", cmd)){
		int ap_fd = -1;
		char tmp[512] = {};
		char *encrypt = NULL;

		ap_fd = open(AP_FILE, O_CREAT | O_RDWR | O_TRUNC, 0666);
		if(ap_fd < 0){
			printf("open %s fail: %s\n", AP_FILE, strerror(errno));
			errcode = INTERNAL_ERROR;
			goto process_and_exit;
		}

		if(!strcmp(passwd, "NULL")){
			encrypt = "no";
		}
		else{
			encrypt = "yes";
		}
		sprintf(tmp, AP_CONF, encrypt, ssid, passwd); /* useless while encrypt=no */

		write(ap_fd, tmp, strlen(tmp));

		printf("create conf file %s done, you can check it manually\n", AP_FILE);

		close(ap_fd);
	}
	else if(!strcmp("STA", cmd)){
		char tmp[512] = {};
		char *key_mgmt_prefix = "#";
		char *psk_prefix = "";

		int wpa_fd = open(WPA_FILE, O_CREAT | O_RDWR | O_TRUNC, 0666);
		if(wpa_fd < 0){
			printf("open %s fail: %s\n", WPA_FILE, strerror(errno));

			errcode = INTERNAL_ERROR;
			goto process_and_exit;
		}

		if(!strcmp(passwd, "NULL")){
			key_mgmt_prefix = "";
			psk_prefix = "#";
		}else{
			key_mgmt_prefix = "#";
			psk_prefix = "";
		}

		sprintf(tmp, WPA_CONF, key_mgmt_prefix, ssid, psk_prefix, passwd);
		write(wpa_fd, tmp, strlen(tmp));

		printf("create conf file %s done, you can check it manually\n", WPA_FILE);

		close(wpa_fd);
	}

process_and_exit:
	/* write result back */
#if 1 // DEBUG
	printf("send result: '%s' to client\n", err[errcode]);
#endif
	if((ret = send(fd, err[errcode], strlen(err[errcode]), 0)) == -1){
		printf("failed to write() in server: %s\n", strerror(errno));
		exit(-1);
	}
	sleep(1); //waiting for data sending.

	if(errcode != OK)
		return;

#if 1
	bool status = false;
	struct wifi_client_register info;
	wifi_ctl_msg_t mode;

	info.pid = getpid();
	info.reset = 1;
	info.priority = 3;
	strcpy(info.name, "from_app");

	register_to_networkmanager(info, callback_handle_func);

	mode.force = true;
	strcpy(mode.name, "from_app");
	if(!strcmp("AP", cmd))
		mode.cmd = SW_AP;
	else if(!strcmp("STA", cmd))
		mode.cmd = SW_STA;

	status = request_wifi_mode(mode);
	printf("recv status is %d in %s:%s:%d\n", status, __FILE__, __func__, __LINE__);
#endif

	return;
}

int main(int argc, char **argv)
{
	int port = 8723;
	int pid, listenfd, socketfd;
	size_t length;
	struct sockaddr_in cli_addr;
	struct sockaddr_in serv_addr;

	/* Become deamon + unstopable and no zombies children (= no wait()) */
	if(fork() != 0)
		return 0; /* parent returns OK to shell */

	signal(SIGCLD, SIG_IGN); /* ignore child death */
	signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */


	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0){
		printf("socket() error in %s:%d: %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}

	int reuse = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1){
		printf("setsockopt() error in %s:%d: %s\n", __func__, __LINE__, strerror(errno));
		close(listenfd);
		return -1;
	}

	if(argc > 1)
		port = atoi(argv[1]);

	if(port < 0 || port >60000)
		printf("Invalid port number: %d(try 1->60000), force to %d", port, PORT);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
		printf("bind() error in %s:%d: %s\n", __func__, __LINE__, strerror(errno));
		close(listenfd);
		return -1;
	}

	if(listen(listenfd, 64) <0){
		printf("listen() error in %s:%d: %s\n", __func__, __LINE__, strerror(errno));
		close(listenfd);
		return -1;
	}

	printf("Start app service on 0:0:0:0:%d\n", port);

	while(1){
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, (socklen_t*)&length)) < 0){
			printf("accept() error in %s:%d: %s\n", __func__, __LINE__, strerror(errno));
			close(listenfd);
			return -1;
		}

		if((pid = fork()) < 0) {
			printf("fork() error in %s:%d: %s\n", __func__, __LINE__, strerror(errno));
			close(listenfd);
			return -1;
		}
		else {
			if(pid == 0) {  /* child doit */
				doit(socketfd);
				return 0;
			} else {        /* parent accept continue */
				close(socketfd);
			}
		}
	}
}
