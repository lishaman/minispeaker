/*
	play a keyword in /etc/tone.ini with mp3 format.

	play_key <keyword>
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

#include "ini_interface.h"
#include "tips_interface.h"

struct playtone_args {
	char *key;
	int sync;

	int list;

	int retry;
	int r_time;
	int rt_set;
};

const char usage_str[] = {
	"Usage: playkey [-s|--sync][-r|--retry][-t|--time TIME] -k KEYWORD\n"
	"    or playkey -l\n"
	"    or playkey [-h|--help]\n\n"
	"Options:\n"
	"  -k, --keyword [KEYWORD]tone.ini keyword.\n"
	"  -l, --list    list all Source section key name\n"
	"  -s, --sync    playtone wait play finished(default: nosync).\n"
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
		{"keyword", required_argument, NULL, 'k'},
		{"list", no_argument, NULL, 'l'},
		{"sync", no_argument, NULL, 's'},
		{"retry", no_argument, NULL, 'r'},
		{"time", required_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0},
	};
	int op;

	while((op = getopt_long(argc, argv, "k:lsrt:h", longopts, NULL)) != -1) {
		switch (op) {
		case 'k':
			args->key = optarg;
			break;

		case 'l':
			args->list = 1;
			break;

		case 's':
			args->sync = 1;
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

#define STR_ARRAY_COUNT		64
#define STR_ARRAY_SIZE		64
static int list_keyword(void)
{
	char **keys;
	int retval;
	int i;

	keys = malloc(STR_ARRAY_COUNT * (sizeof(void *) + STR_ARRAY_SIZE));
	if (!keys) {
		printf("%s. Alloc keys: %s\n", appName, strerror(errno));
		return -1;
	}

	/* Map string Array */
	for (i = 0; i < STR_ARRAY_COUNT; i++)
		keys[i] = (char *)keys + STR_ARRAY_COUNT * sizeof(void *) + i * STR_ARRAY_SIZE;

	retval = mozart_ini_getkeys("/etc/tone.ini", "Source", keys);
	if (retval < 0) {
		if (retval == CFG_ERR_OPEN_FILE)
			printf("%s getkeys open '%s' %s\n", appName, "/etc/tone.ini", strerror(errno));
		else
			printf("%s getkeys error: %d\n", appName, retval);

		free(keys);
		return -1;
	}

	printf("%s. List %d valid keyword:\n", appName, retval);
	/* print key list */
	for (i = 0; i < retval; i++)
		printf("[%02d] %s\n", i + 1, keys[i]);

	free(keys);

	return 0;
}

#define msec	200
static int key_play(struct playtone_args *args)
{
	int err;

	if (!args->key) {
		printf("%s. keyword is NULL\n", appName);
		usage_print();
		err = -1;
		goto err_check_key;
	}

	if (args->sync)
		printf("%s. synchronized method\n", appName);
	else
		printf("%s. nonsynchronized method\n", appName);

	do {
		if (args->sync)
			err = mozart_play_key_sync(args->key);
		else
			err = mozart_play_key(args->key);

		if (err < 0 || !args->retry)
			break;

		args->r_time -= msec;
		usleep(msec * 1000);
	} while (args->r_time > 0);

err_check_key:
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

	if (args.list) {
		return list_keyword();
	}

	return key_play(&args);
}
