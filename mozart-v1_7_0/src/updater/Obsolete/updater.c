#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include <pthread.h>

#include "ini_interface.h"
#include "event_interface.h"
#include "sharememory_interface.h"
#include "linklist_interface.h"
#include "tips_interface.h"

#include "updater.h"
#include "broadcast.h"
#include "updater_interface.h"

update_status status = UPDATE_DO_NOTHING;
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t key_mutex = PTHREAD_MUTEX_INITIALIZER;

int key_response = 0;

bool exit_now = false;

char *update_status_str[] = {
	[UPDATE_DO_NOTHING] = "UPDATE_DO_NOTHING",
	[UPDATE_DETECTING] = "UPDATE_DETECTING",
	[UPDATE_FOUND] = "UPDATE_FOUND",
	[UPDATE_NOTFOUND] = "UPDATE_NOTFOUND",
	[UPDATE_UPDATING] = "UPDATE_UPDATING",
	[UPDATE_UPDATED_SUCCESS] = "UPDATE_UPDATED_SUCCESS",
	[UPDATE_UPDATED_FAIL] = "UPDATE_UPDATED_FAIL",
};

update_status get_status(void)
{
	update_status ret = UPDATE_DO_NOTHING;

	pthread_mutex_lock(&status_mutex);
	ret = status;
	pthread_mutex_unlock(&status_mutex);

	return ret;
}

update_status set_status(update_status new_status)
{
	update_status old_status = UPDATE_DO_NOTHING;
	char buf[5] = {};

	pthread_mutex_lock(&status_mutex);
	old_status = status;
	status = new_status;

	sprintf(buf, "%d", status);
	mozart_ini_setkey("/tmp/updater_cur_status", "updater", "status", buf);

	pthread_mutex_unlock(&status_mutex);

	return old_status;
}

update_status check_status_on_startup(void)
{
	update_status ret = UPDATE_DO_NOTHING;
	char buf[5] = {};

	pthread_mutex_lock(&status_mutex);

	if (mozart_ini_getkey("/tmp/updater_cur_status", "updater", "status", buf)) {
		ret = UPDATE_DO_NOTHING;
		goto exit;
	} else {
		ret = atoi(buf);
	}

exit:
	pthread_mutex_unlock(&status_mutex);

	return ret;
}

void usage(char *appname)
{
	printf("%s - update support program.\n"
	       "Usage:\n"
	       "  %s -s\n"
	       "      write system startup success flags to spinor.\n"
	       "  %s -h\n"
	       "      this help\n", appname, appname, appname);
}

void check_version(int signum)
{
	update_pkg *pkg = NULL;
	// default alarm time is 5.
	static int min = 5;
	unsigned int update_flag = 0;
#define MIN_MAX 60

	update_status cur_status = get_status();

	switch (cur_status) {
	case UPDATE_DETECTING:
	case UPDATE_UPDATING:
		printf("current status is %s, Please waiting for status done!!!\n",
		       update_status_str[cur_status]);
		goto check_version_exit;
	case UPDATE_FOUND:
		printf("current status is %s, new version found, You can update now!!!\n",
		       update_status_str[cur_status]);
		mozart_play_key("update_found");
		goto check_version_exit;
	case UPDATE_UPDATED_SUCCESS:
		printf("current status is %s, You can reboot now!!!\n",
		       update_status_str[cur_status]);
		goto check_version_exit;
	case UPDATE_UPDATED_FAIL:
		printf("\n\n\n"
		       "********************\n"
		       "current status is %s, Please handle this Fatal error!!!\n"
		       "**********************"
		       "\n\n\n", update_status_str[cur_status]);
		goto check_version_exit;
	default:
		printf("current status is %s, begin to check new version...\n",
		       update_status_str[cur_status]);
		break;
	}

	// begin new version detection.
	set_status(UPDATE_DETECTING);
	if ((pkg = mozart_updater_chkver())) {
		mozart_play_key("update_found");
		set_status(UPDATE_FOUND);
		printf("new version found, current version: %d, lastest version: %d.\n",
		       pkg->machine_version, pkg->lastest_version);
		mozart_updater_destory(pkg);
	} else {
		set_status(UPDATE_NOTFOUND);
		printf("NO new version found.\n");
		goto check_version_exit;
	}

check_version_exit:
	// re-check update after min minutes.
	min = (min + 5 > MIN_MAX) ? min: MIN_MAX;
	alarm(min * 60);

	return;
}

static void key_response_start(void)
{
	pthread_mutex_lock(&key_mutex);
	key_response = 1;
	pthread_mutex_unlock(&key_mutex);
}

static void key_response_stop(void)
{
	pthread_mutex_lock(&key_mutex);
	key_response = 0;
	pthread_mutex_unlock(&key_mutex);
}

static bool key_responsing(void)
{
	bool result = true;

	pthread_mutex_lock(&key_mutex);
	if (key_response == 0)
		result = false;
	else
		result = true;
	pthread_mutex_unlock(&key_mutex);

	return result;
}

void *key_reponser(void *arg)
{
	update_status cur_status = get_status();

	update_pkg *pkg = NULL;

	if (cur_status == UPDATE_UPDATED_SUCCESS || cur_status == UPDATE_UPDATED_FAIL) {
		printf("update done with %s, do nothing.\n", update_status_str[cur_status]);
		goto key_reponser_exit;
	} else if (cur_status == UPDATE_DETECTING) {
		// get the newest package info before update.
		// check_new_version() will ONLY change status to
		// UPDATE_FOUND, UPDATE_NOTFOUND.
		// so use while(1) here.
		update_status p = UPDATE_DO_NOTHING;
		while (1) {
			p = get_status();
			if (p == UPDATE_FOUND) {
				printf("new version found.\n");
				break;
			} else if (p == UPDATE_NOTFOUND) {
				printf("NO new version found, Do nothing.\n");
				goto key_reponser_exit;
			} else {
				usleep(5*1000);
			}
		}
	} else {
		set_status(UPDATE_DETECTING);
		if ((pkg = mozart_updater_chkver())) {
			mozart_play_key("update_found");
			set_status(UPDATE_FOUND);
			printf("new version found.\n");
		} else {
			mozart_play_key("update_not_found");
			set_status(UPDATE_NOTFOUND);
			printf("NO new version found, Do nothing.\n");
			goto key_reponser_exit;
		}
	}

	set_status(UPDATE_UPDATING);
	mozart_play_key("updating");

	// download update package.
	if (mozart_updater_download(pkg)) {
		set_status(UPDATE_UPDATED_FAIL);
		mozart_play_key("update_failed");
		printf("Fatal error occurred while invoking update(), abort!!!\n");
		goto key_reponser_exit;
	}

	// update it.
	if (mozart_updater_apply(pkg)) {
		set_status(UPDATE_UPDATED_FAIL);
		mozart_play_key("update_failed");
		printf("Fatal error occurred while invoking update(), abort!!!\n");
		goto key_reponser_exit;
	}

	printf("Update done successfully!!!\n");
	mozart_play_key("update_successed");
	set_status(UPDATE_UPDATED_SUCCESS);

key_reponser_exit:
	if (pkg)
		mozart_updater_destory(pkg);

#if 0
	if (get_status() == UPDATE_UPDATED_SUCCESS) {
		exit_now = true;
	}
#endif

	key_response_stop();

	return NULL;
}

int start_update_thread(void)
{
	pthread_t update_thread;

	if (pthread_create(&update_thread, NULL, key_reponser, NULL)) {
		printf("pthread_create error: %s\n", strerror(errno));
		return -1;
	}

	if (pthread_detach(update_thread)) {
		printf("pthread_detach error: %s(Ignored!!)\n", strerror(errno));
		return 0;
	}

	return 0;
}

static void key_callback(mozart_event event, void *param)
{
        module_status status;

        switch (event.type) {
        case EVENT_KEY:
                if (event.event.key.key.type == EV_KEY &&
                    event.event.key.key.code == KEY_RECORD && event.event.key.key.value == 1)
                        if (key_responsing()) {
                                printf("Processing the previous update request, ignore!! Please waiting...\n");
                                return;
                        } else {
                                key_response_start();
                        }

                // if bt-call is in or calling, will not response this key event.
                if (share_mem_get(BT_HS_DOMAIN, &status)) {
                        printf("share_mem_get failed, begin update status.\n");
                } else if (status == STATUS_BT_HS_AUD_REQ ||
                           status == STATUS_BT_HS_AUD_RDY ||
                           status == STATUS_BT_HS_AUD_INUSE) {
                        printf("Bluetooth is active, do nothing.\n");
                        break;
                }

                // start a thread to status the update, so we can response the next key event as soon as possible.
                start_update_thread();
                break;
        default:
                // ignore all other key down.
                break;
        }

	return;
}

void sig_handler(int signo)
{
	printf("killed by signal: %d.\n", signo);

	exit_now = true;
}

int main(int argc, char **argv)
{
	int ret = 0;
	int c = -1;
        mozart_handler *handler = NULL;

	while (1) {
		c = getopt(argc, argv, "sh");
		if (c < 0)
			break;
		switch (c) {
		case 's':
			return mozart_updater_startup();
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 0;
		}
	}

#if 1
	// become daemon
	if (daemon(0, 1)) {
		printf("transfer updater status to daemon by invoking daemon() error: %s\n", strerror(errno));
		ret = -1;
		goto main_exit;
	}
#endif

#if 1
	// read the previous update status saved on /tmp/updater_cur_status back.
	if (!access("/tmp/updater_cur_status", 0)) {
		update_status pre_status = UPDATE_DO_NOTHING;
		if ((pre_status = check_status_on_startup()) == UPDATE_DO_NOTHING) {
			set_status(UPDATE_DO_NOTHING);
		} else {
			if (pre_status == UPDATE_DETECTING || pre_status == UPDATE_UPDATING) {
				printf("updater exit while %s, set status to UPDATE_DO_NOTHING.\n",
				       update_status_str[pre_status]);
				set_status(UPDATE_DO_NOTHING);
			} else {
				set_status(pre_status);
			}
		}
	} else {
		set_status(UPDATE_DO_NOTHING);
		mozart_updater_startup();
	}
#endif

	if (start_version_broadcast_thread())
		printf("start_version_respond_thread error, no version broadcast func.\n");

	//share memory init.
	if (share_mem_init()) {
		printf("invoke share_mem_init failed.\n");
		ret = -1;
		goto main_exit;
	}

	// register key event
	handler = mozart_event_handler_get(key_callback, argv[0]);

	if (SIG_ERR == signal(SIGALRM, check_version)) {
		printf("Warning: signal(SIGALRM, check_version) error: %s(will not check new version regularly.).\n",
		       strerror(errno));
	}
	// XXX: After alarm(), DO NOT invoke sleep() any more!!!!!!!!!!!!!!!
	alarm(1);

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGSEGV, sig_handler);
	signal(SIGABRT, sig_handler);

	while (!exit_now)
		usleep(999);

main_exit:
	// cancle alarm.
	alarm(0);

	stop_version_broadcast();

	mozart_event_handler_put(handler);

	share_mem_destory();

	printf("updater exit...\n");

	return ret;
}
