#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sharememory_interface.h"

static char d_buf[512] = {};
static char s_buf[512] = {};

static const char *dumpdomain(void)
{
    int i = 0;

    for (i = 0; i < get_domain_cnt(); i++) {
        strcat(d_buf, memory_domain_str[i]);
        strcat(d_buf, "\n      ");
    }

    return d_buf;
}

static const char *dumpstatus(void)
{
    int i = 0;

    for (i = 0; i < get_status_cnt(); i++) {
        strcat(s_buf, module_status_str[i]);
        strcat(s_buf, "\n      ");
    }

    return s_buf;
}

static void usage(const char *name)
{
    printf("%s - sharemem write util.\n\n"
           "\nOptions:\n"
           "  -d domain -s <status>\n"
           "\nDetails:\n"
           "  -d: domain name\n"
           "    avaible domain:\n"
           "      %s\n"
           "  -s: domain status.\n"
           "    avaible status:\n"
           "      %s\n"
           "\nExamples:\n"
           "  %s -d AIRPLAY_DOMAIN -s STATUS_STOPPING\n"
           "  %s -d VR_DOMAIN -s STATUS_SHUTDOWN\n", name, dumpdomain(), dumpstatus(), name, name);
}

static memory_domain getdomain(char *str)
{
    int i = 0;

    if (!str)
        return UNKNOWN_DOMAIN;

    for (i = 0; i < get_domain_cnt(); i++) {
        if (!strcmp(str, memory_domain_str[i]))
            return i;
    }

    return UNKNOWN_DOMAIN;
}

static module_status getstatus(char *str)
{
    int i = 0;

    if (!str)
        return STATUS_UNKNOWN;

    for (i = 0; i < get_status_cnt(); i++) {
        if (!strcmp(str, module_status_str[i]))
            return i;
    }

    return STATUS_UNKNOWN;
}

int main(int argc, char *argv[])
{
    int c = -1;
	module_status status = STATUS_UNKNOWN;
	memory_domain domain= UNKNOWN_DOMAIN;
    char *d = NULL;
    char *s = NULL;

    for (;;) {
        c = getopt(argc, argv, "d:s:");
        if (c < 0)
            break;
        switch (c) {
        case 'd':
            d = optarg;
            break;
        case 's':
            s = optarg;
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (!d || !s) {
        usage(argv[0]);
        return -1;
    }

	share_mem_init();

    domain = getdomain(d);
    if (domain == UNKNOWN_DOMAIN) {
        printf("parse domain name error, exit...\n");
        goto exit;
    }

    status = getstatus(s);
    if (status == STATUS_UNKNOWN) {
        printf("parse status error, exit...\n");
        goto exit;
    }

    if (share_mem_set(domain, status)) {
		printf("domain %s's status to %s error, exit...\n", d, s);
        goto exit;
    }

exit:
	share_mem_destory();

	return 0;
}
