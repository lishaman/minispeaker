#include <stdio.h>
#include <string.h>

#include "event_interface.h"

int main(void)
{
	/* Disconnect Event notify to event manager */
	mozart_event event = {
		.event = {
			.misc = {
				.name = "airplay",
				.type = "connected",
			},
		},
		.type = EVENT_MISC,
	};

	int ret = mozart_event_send(event);

	if (!ret)
		printf("Send dlna connected event successfully.\n");
	else
		printf("Send dlna connected event unsuccessfully.\n");

	return ret;
}
