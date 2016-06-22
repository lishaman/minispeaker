#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "broadcast.h"
#include "updater.h"
#include "updater_interface.h"

#define PORT 1357
bool stop_broadcast = false;

void *version_broadcast_func(void *arg)
{
	int sockfd = -1;
	char *ret = NULL;

	version_t version;

	int so_broadcast=1;

	struct sockaddr_in sockaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("socket() error: %s\n", strerror(errno));
		ret = NULL;
		goto exit;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast))) {
		printf("setsockopt() error: %s\n", strerror(errno));
		ret = NULL;
		goto exit;
	}

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = ntohs(PORT);
	sockaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	while (!stop_broadcast) {
		version.status = get_status();
		version.machine_version = -1;
		version.lastest_version = -1;
		if (-1 == sendto(sockfd, &version, sizeof(version), 0,
				 (struct sockaddr *)&sockaddr, sizeof(sockaddr))) {
			printf("sendto() error: %s\n", strerror(errno));
			ret = NULL;
			goto exit;
		}
#ifdef DEBUG
		printf("status: %d\n", version.status);
		printf("machine_version: %d\n", version.machine_version);
		printf("lastest_version: %d\n", version.lastest_version);
#endif
		sleep(5);
	}

exit:
	if (sockfd) {
		close(sockfd);
		sockfd = -1;
	}

	return ret;
}

int start_version_broadcast_thread(void)
{
	pthread_t version_broadcast_thread;

	if (pthread_create(&version_broadcast_thread, NULL, version_broadcast_func, NULL)) {
		printf("pthread_create error: %s\n", strerror(errno));
		return -1;
	}

	if (pthread_detach(version_broadcast_thread)) {
		printf("pthread_create error: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

void stop_version_broadcast(void)
{
	stop_broadcast = true;
}
