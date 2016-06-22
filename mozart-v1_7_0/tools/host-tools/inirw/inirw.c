#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ini_interface.h"

#define INI_UNKONW 0
#define INI_READ 1
#define INI_WRITE 2

static void usage(const char *name)
{
    printf("%s - ini file read/write util.\n\n"
           "\nOptions:\n"
           "  -f </path/to/file> -w/-r -s <section> -k <key> [-v <value>]\n"
           "\nDetails:\n"
           "  -f: ini file path.\n"
           "  -w/-r: write/read action.\n"
           "  -s: specfy section name.\n"
           "  -k: specfy key name.\n"
           "  -v: specfy value name.(ONLY used on write action.)\n"
           "\nExamples:\n"
           "  %s -f /usr/data/system.ini -r -s volume -k music\n"
           "  %s -f /usr/data/system.ini -w -s volume -k music -v 80\n", name, name, name);
}

int main(int argc, char **argv)
{
    /*Get command line parameters*/
    int c;
    char action = INI_UNKONW;
    char *filepath = NULL;
    char *section = NULL;
    char *key = NULL;
    char *value = NULL;

    for (;;) {
        c = getopt(argc, argv, "rwf:s:k:v:");
        if (c < 0)
            break;
        switch (c) {
        case 'w':
            action = INI_WRITE;
            break;
        case 'r':
            action = INI_READ;
            break;
        case 'f':
            filepath = optarg;
            break;
        case 's':
            section = optarg;
            break;
        case 'k':
            key = optarg;
            break;
        case 'v':
            value = optarg;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

#if 0
    printf("action is %d, filepath: %s, section: %s, key: %s, value: %s.\n",
           action, filepath, section, key, value);
#endif

    if (action == INI_UNKONW) {
        usage(argv[0]);
        return -1;
    } else if (action == INI_WRITE) {
        if (!filepath || !section || !key || !value) {
            usage(argv[0]);
            return -1;
        } else {
            if (mozart_ini_setkey(filepath, section, key, value)) {
                //printf("Could not write %s=%s below %s in %s.\n", key, value, section, filepath);
                return -1;
            }
        }
    } else if (action == INI_READ) {
        if (!filepath || !section || !key) {
            usage(argv[0]);
            return -1;
        } else {
            value = malloc(512);
            memset(value, 0, 512);
            if (mozart_ini_getkey(filepath, section, key, value)) {
                //printf("Could not get %s's value below %s in %s.\n", key, section, filepath);
                free(value);
                value = NULL;
                return -1;
            }
            puts(value);
            free(value);
            value = NULL;
        }
    }

    return 0;
}
