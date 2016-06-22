#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#include "ini_interface.h"
#include "ota_interface.h"

struct ota_args {
	char		*url;
	char		*file;
	uint32_t	version;
};

const char usage_str[] = {
	"Usage: otact -f FILE | -u URL\n"
	"    or otact -h\n\n"
	"Options:\n"
	"  -f    [FILE]info file path.\n"
	"  -u    [URL]tone remote url.\n"
	"  -h    print usage\n\n"
};

static char *appName = NULL;

static void usage_print(void)
{
	printf(usage_str);
}

static int parse_argument(struct ota_args *args, int argc, char *argv[])
{
	int op;

	while ((op = getopt(argc, argv, "f:u:h")) != -1) {
		switch (op) {
		case 'f':
			if (args->url) {
				printf("%s. %s : %s at sametime\n", appName, optarg, args->url);
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

			args->url = strdup(optarg);
			if (!args->file) {
				printf("%s. %s: %s\n", appName, optarg, strerror(errno));
				exit(1);
			}
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

int main(int argc, char **argv)
{
	struct ota_args args;
	int fd;
	int err = 0;

	/* Get basename */
	appName = basename(argv[0]);

	/* perse arguments */
	memset(&args, 0, sizeof(args));
	if (parse_argument(&args, argc, argv) < 0) {
		usage_print();
		return -1;
	}

	if (args.file && !args.url) {
		args.url = malloc(512);
		if (!args.url) {
			printf("%s. Alloc url: %s\n", appName, strerror(errno));
			err = -1;
			goto err_alloc_url;
		}

		err = mozart_ini_getkey(args.file, "Update", "remote-url", args.url);
		if (err) {
			printf("%s. Not get remote-url\n", appName);
			err = -1;
			goto err_get_url;
		}
	}

	printf("%s. Remote url: %s\n", appName, args.url);

	fd = mozart_ota_init();
	if (fd < 0) {
		printf("%s. mozart OTA init: %s\n", appName, strerror(errno));
		err = -1;
		goto err_ota_init;
	}

	mozart_ota_seturl(args.url, strlen(args.url), fd);

	mozart_ota_start_update();
        printf("%s. Mozart update OTA standby ...\n", appName);

	mozart_ota_close(fd);

err_ota_init:
err_get_url:
err_alloc_url:
	if (args.file)
		free(args.file);
	if (args.url)
		free(args.url);

	return err;
}
