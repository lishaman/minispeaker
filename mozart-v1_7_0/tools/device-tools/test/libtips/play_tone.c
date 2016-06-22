/*
	play a tone with mp3 format.

	play_tone <tone file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tips_interface.h"

struct playtone_args {
	char *file;

	enum {
		O_PLAY = 0,
		O_STOP,
	} type;
	int sync;

	int retry;
	int r_time;
	int rt_set;
};

const char usage_str[] = {
	"Usage: playtone [-s|--sync][-r|--retry][-t|--time TIME] -f FILE\n"
	"    or playtone [-s|--sync][-r|--retry][-t|--time TIME] -u URL\n"
	"    or playtone [-s|--sync] -k|--stop \n"
	"    or playtone [-h|--help]\n\n"
	"Options:\n"
	"  -f, --file    [FILE]tone file path.\n"
	"  -u, --url     [URL]tone remote url.\n"
	"  -s, --sync    playtone wait play finished(default: nosync).\n"
	"  -k, --stop    Stop tone play\n"
	"  -r, --retry   try to connect server at TIME seconds.\n"
	"  -t, --time    [TIME] retry time length(default 3s).\n"
	"  -h, --help    print usage\n\n"
};

static char *appName = NULL;

static void usage_print(void)
{
	printf(usage_str);
}

static int parse_argument(struct playtone_args *args, int argc, char *argv[])
{
	struct option longopts[] = {
		{"file", required_argument, NULL, 'f'},
		{"url", required_argument, NULL, 'u'},
		{"sync", no_argument, NULL, 's'},
		{"stop", no_argument, NULL, 'k'},
		{"retry", no_argument, NULL, 'r'},
		{"time", required_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0},
	};
	int op;

	while((op = getopt_long(argc, argv, "f:u:skrt:h", longopts, NULL)) != -1) {
		switch (op) {
		case 'f':
			if (args->file) {
				printf("%s. %s : %s at sametime\n", appName, optarg, args->file);
				usage_print();
				exit(1);
			}

			args->file = realpath(optarg, NULL);
			if (!args->file) {
				printf("%s. %s: %s\n", appName, optarg, strerror(errno));
				exit(1);
			}
			break;

		case 'u':
			if (args->file) {
				printf("%s. %s : %s at sametime\n", appName, optarg, args->file);
				usage_print();
				exit(1);
			}

			args->file = strdup(optarg);
			if (!args->file) {
				printf("%s. %s: %s\n", appName, optarg, strerror(errno));
				exit(1);
			}
			break;

		case 's':
			args->sync = 1;
			break;

		case 'k':
			args->type = O_STOP;
			break;

		case 'r':
			args->retry = 1;
			if (!args->rt_set)
				args->r_time = 3 * 1000;
			break;

		case 't':
			args->r_time = atoi(optarg) * 1000;
			args->rt_set = 1;
			break;

		case 'h':
		default:
			usage_print();
			exit(1);
		}
	}

	/* Export no detect args */
	if (optind < argc) {
		int i;
		for (i = optind; i < argc; i++)
			fprintf(stderr, "%s: ignoring extra argument -- %s\n", appName, argv[i]);
	}

	return 0;
}

#define msec	200
static int tone_play(struct playtone_args *args)
{
	int err;

	if (!args->file) {
		printf("%s. file or url is NULL\n", appName);
		usage_print();
		err = -1;
		goto err_check_file;
	}

	if (args->sync)
		printf("%s. synchronized method\n", appName);
	else
		printf("%s. nonsynchronized method\n", appName);

	do {
		if (args->sync)
			err = mozart_play_tone_sync(args->file);
		else
			err = mozart_play_tone(args->file);

		if (!err || !args->retry)
			break;

		args->r_time -= msec;
		usleep(msec * 1000);
	} while (args->r_time > 0);

err_check_file:
	if (args->file)
		free(args->file);

	return err;
}

static int tone_stop(struct playtone_args *args)
{
	int err;

	if (args->file)
		free(args->file);

	if (args->sync)
		printf("%s. synchronized method\n", appName);
	else
		printf("%s. nonsynchronized method\n", appName);

	if (args->sync)
		err = mozart_stop_tone_sync();
	else
		err = mozart_stop_tone();

	return err;
}

int main(int argc, char **argv)
{
	struct playtone_args args;

	/* Get basename */
	appName = basename(argv[0]);

	/* perse arguments */
	memset(&args, 0, sizeof(args));
	if (parse_argument(&args, argc, argv) < 0) {
		usage_print();
		return -1;
	}

	switch (args.type) {
	case O_PLAY:
		return tone_play(&args);
	case O_STOP:
		return tone_stop(&args);
	default:
		printf("%s. Unknown type\n", appName);
	}

	return -1;
}
