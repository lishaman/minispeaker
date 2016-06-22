#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "volume_interface.h"

static void usage(const char *name)
{
	printf("%s [-w <value>] [-r]\n"
	       " -r = get current volume\n"
	       " -w = set volume to value\n", name);
}

int test_get_volume(void)
{
	printf("current volume is %d.\n", mozart_volume_get());

	return 0;
}

int test_set_volume(int volume)
{
	return mozart_volume_set(volume, MUSIC_VOLUME);
}

int main(int argc, char **argv)
{
	/*Get command line parameters*/
	int c;
	for (;;) {
		c = getopt(argc, argv, "w:r");
		if (c < 0)
			break;
		switch (c) {
		case 'w':
			return test_set_volume(atoi(optarg));
		case 'r':
			return test_get_volume();
		default:
			usage(argv[0]);
			return -1;
		}
	}

	usage(argv[0]);
	return 0;
}
