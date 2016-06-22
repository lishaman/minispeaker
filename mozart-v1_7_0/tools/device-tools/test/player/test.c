#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <execinfo.h>

#include "player_interface.h"

char *file = NULL;

static void usage(const char *app_name)
{
	printf("%s [-f file] -h\n"
	       " -h help (show this usage text)\n"
	       " -f file\n", app_name);

	return;
}

static char *signal_str[] = {
	[1] = "SIGHUP",       [2] = "SIGINT",       [3] = "SIGQUIT",      [4] = "SIGILL",      [5] = "SIGTRAP",
	[6] = "SIGABRT",      [7] = "SIGBUS",       [8] = "SIGFPE",       [9] = "SIGKILL",     [10] = "SIGUSR1",
	[11] = "SIGSEGV",     [12] = "SIGUSR2",     [13] = "SIGPIPE",     [14] = "SIGALRM",    [15] = "SIGTERM",
	[16] = "SIGSTKFLT",   [17] = "SIGCHLD",     [18] = "SIGCONT",     [19] = "SIGSTOP",    [20] = "SIGTSTP",
	[21] = "SIGTTIN",     [22] = "SIGTTOU",     [23] = "SIGURG",      [24] = "SIGXCPU",    [25] = "SIGXFSZ",
	[26] = "SIGVTALRM",   [27] = "SIGPROF",     [28] = "SIGWINCH",    [29] = "SIGIO",      [30] = "SIGPWR",
	[31] = "SIGSYS",      [34] = "SIGRTMIN",    [35] = "SIGRTMIN+1",  [36] = "SIGRTMIN+2", [37] = "SIGRTMIN+3",
	[38] = "SIGRTMIN+4",  [39] = "SIGRTMIN+5",  [40] = "SIGRTMIN+6",  [41] = "SIGRTMIN+7", [42] = "SIGRTMIN+8",
	[43] = "SIGRTMIN+9",  [44] = "SIGRTMIN+10", [45] = "SIGRTMIN+11", [46] = "SIGRTMIN+12", [47] = "SIGRTMIN+13",
	[48] = "SIGRTMIN+14", [49] = "SIGRTMIN+15", [50] = "SIGRTMAX-14", [51] = "SIGRTMAX-13", [52] = "SIGRTMAX-12",
	[53] = "SIGRTMAX-11", [54] = "SIGRTMAX-10", [55] = "SIGRTMAX-9",  [56] = "SIGRTMAX-8", [57] = "SIGRTMAX-7",
	[58] = "SIGRTMAX-6",  [59] = "SIGRTMAX-5",  [60] = "SIGRTMAX-4",  [61] = "SIGRTMAX-3", [62] = "SIGRTMAX-2",
	[63] = "SIGRTMAX-1",  [64] = "SIGRTMAX",
};

void sig_handler(int signo)
{
	char cmd[64] = {};
	void *array[10];
	int size = 0;
	char **strings = NULL;
	int i = 0;

	printf("\n\n[%s: %d] mozart crashed by signal %s.\n", __func__, __LINE__, signal_str[signo]);

	printf("Call Trace:\n");
	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);
	if (strings) {
		for (i = 0; i < size; i++)
			printf ("  %s\n", strings[i]);
		free (strings);
	} else {
		printf("Not Found\n\n");
	}

	if (signo == SIGSEGV || signo == SIGBUS ||
	    signo == SIGTRAP || signo == SIGABRT) {
		sprintf(cmd, "cat /proc/%d/maps", getpid());
		printf("Process maps:\n");
		system(cmd);
	}

	exit(-1);
}

int callback(player_snapshot_t *snapshot, struct player_handler *handler, void *param)
{
	printf("player controller uuid: %s, status: %s, pos is %d, duration is %d.\n",
	       snapshot->uuid, player_status_str[snapshot->status], snapshot->position, snapshot->duration);

	switch (snapshot->status) {
	case PLAYER_TRANSPORT:
	case PLAYER_PLAYING:
	case PLAYER_PAUSED:
	case PLAYER_UNKNOWN:
		break;
	case PLAYER_STOPPED:
		if (!strcmp(handler->uuid, snapshot->uuid))
			mozart_player_playurl(handler, file);
		break;
	default:
		break;
	}

	return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;
        player_handler_t *handler = NULL;

        player_status_t status;
	int pos = -1;
	int duration = -1;
	char *url = NULL;
	char *uuid = NULL;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGABRT, sig_handler);

	/* Get command line parameters */
        int c = -1;
        while (1) {
                c = getopt(argc, argv, "f:h");
                if (c < 0)
                        break;
                switch (c) {
                case 'f':
                        file = optarg;
                        break;
                case 'h':
                        usage(argv[0]);
                        return 0;
                default:
                        usage(argv[0]);
                        return -1;
                }
        }

        handler = mozart_player_handler_get("player-test", callback, NULL);
        if (!handler) {
                printf("get player handler failed, do nothing, exit...\n");
                return -2;
        }

        if (file)
                mozart_player_playurl(handler, file);

	int i = 0;
	for (i = 0; i < 3600; i++) {
		uuid = mozart_player_getuuid(handler);
		printf("uuid is %s.\n", uuid);
		free(uuid);
		uuid = NULL;

		pos = mozart_player_getpos(handler);
		printf("pos is %d.\n", pos);

		duration = mozart_player_getduration(handler);
		printf("duration is %d.\n", duration);

		url = mozart_player_geturl(handler);
		printf("url is %s.\n", url);
		free(url);
		url = NULL;

		status = mozart_player_getstatus(handler);
		printf("status is %s.\n", player_status_str[status]);

		sleep(1);
	}

__exit:
        mozart_player_handler_put(handler);

        return ret;
}
