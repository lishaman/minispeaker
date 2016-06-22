#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "utils_interface.h"
#include "sharememory_interface.h"
#include "render_interface.h"

#if 0
#define DDEBUG(fmt, args...) printf("Debug. "fmt, ##args)
#else
#define DDEBUG(fmt, args...)
#endif

struct commands {
	char *cmd;

	void (*fun)(void *para);
	void *para;
};

static int quit = 0;

static char pre_string[] = {
	"Ingenic DLNA utils\n"
};

static char help_string[] = {
	"Function\n"
	"    start    start DLNA function\n"
	"    stop     stop DLNA function\n"
	"    quit     quit from this term\n"
	"    help     print help info\n"
};

static void pre_print(void)
{
	printf("%s", pre_string);
	printf("%s", help_string);
	fflush(stdout);
}

static void term_start(void *para)
{
	/*
	 * this is our default frendlyname rule in librender.so,
	 * and you can create your own frendlyname rule.
	 */
	char deviceName[64] = {};
	char macaddr[] = "255.255.255.255";
	memset(macaddr, 0, sizeof (macaddr));

	//FIXME: replace "wlan0" with other way. such as config file.
	get_mac_addr("wlan0", macaddr, "");
	sprintf(deviceName, "SmartAudio-%s", macaddr + 4);

	//share memory init.
	if(0 != share_mem_init()){
		printf("share_mem_init failure.");
		return;
	}

	if(0 != share_mem_clear()){
		printf("share_mem_clear failure.");
		return;
	}

	mozart_render_start(deviceName);
}

static void term_stop(void *para)
{
	//share memory init.
	if(0 != share_mem_init()){
		printf("share_mem_init failure.");
		return;
	}

	if(0 != share_mem_clear()){
		printf("share_mem_clear failure.");
		return;
	}

	mozart_render_terminate();

	share_mem_destory();
}

static void term_quit(void *para)
{
	term_stop(NULL);
	*(int *)para = 1;
}

static void term_help(void *para)
{
	printf("%s", help_string);
	fflush(stdout);
}

static struct commands cmdv[] = {
	{"start", term_start, NULL},
	{"stop", term_stop, NULL},
	{"quit", term_quit, &quit},
	{"help", term_help, NULL},
	{NULL, NULL, NULL},
};

static int handle_cmd(char *cmd)
{
	int v_size = sizeof(cmdv) / sizeof(struct commands);
	int i;

	for (i = 0; i < v_size; i++) {
		/* Undefined command */
		if (!cmdv[i].cmd)
			return -1;

		/* If command match, run function */
		if (!strcmp(cmd, cmdv[i].cmd)) {
			cmdv[i].fun(cmdv[i].para);
			break;
		}
	}

	return 0;
}

static void stdin_clear(void)
{
	int c;
	while ((c = getchar() != '\n' && c != EOF));
}

int main(int argc, char *argv[])
{
	char cmd[128] = {0};
	int n;
	int ret;

	pre_print();

	while (!quit) {
		printf(">>> ");
		fflush(stdout);

		n = scanf("%[^\n]", cmd);
		stdin_clear();
		if (n < 0) {
			printf("Get command: %s\n", strerror(errno));
			return -1;
		} else if (n == 0) {
			DDEBUG("NULL Input\n");
			continue;
		}

		ret = handle_cmd(cmd);
		if (ret) {
			printf("%s, invalid command\n", cmd);
			printf("%s", help_string);
			fflush(stdout);
		}
	}

	return 0;
}
