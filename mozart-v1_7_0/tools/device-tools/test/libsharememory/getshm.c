#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "sharememory_interface.h"

int main(int argc, char *argv[])
{
	int i = 0;
	module_status status = STATUS_UNKNOWN;

	share_mem_init();

	printf("Trace sharememory:\n");
	for (i = 0; i < get_domain_cnt(); i++) {
		if (memory_domain_str[i] && strcmp(memory_domain_str[i], "UNKNOWN_DOMAIN")) {
			if (!share_mem_get(i, &status))
				printf("%20s:\t%s\n", memory_domain_str[i], module_status_str[status]);
			else
				printf("get %s's current status error.\n", memory_domain_str[i]);
		}
	}

	return 0;
}
