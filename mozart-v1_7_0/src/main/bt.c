#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "utils_interface.h"
#include "bluetooth_interface.h"
#include "sharememory_interface.h"
#include "mozart_config.h"
#include "resample_interface.h"

#if (SUPPORT_WEBRTC == 1)
#include "webrtc_aec.h"
bt_aec_callback bt_ac;
#endif

#define SUPPORT_BSA_BLE				0
#define SUPPORT_BSA_BLE_SERVER			0
#define SUPPORT_BSA_BLE_CLIENT			0

#define SUPPORT_BSA_HS_RESAMPLE			0
#define SUPPORT_BSA_HS_RESAMPLE_8K_to_48K	0

#define SUPPORT_BSA_AVK_RESAMPLE		0
#define SUPPORT_BSA_AVK_RESAMPLE_44K_to_48K	0
#define SUPPORT_BSA_AVK_RESAMPLE_2CHANNEL_to_1CHANNEL 0

#define SUPPORT_AEC_RESAMPLE			0
#define SUPPORT_AEC_RESAMPLE_48K_TO_8K		0

#if (SUPPORT_BSA_HS_RESAMPLE == 1)
af_resample_t* hs_resample_s;

hs_sample_init_data hs_sample_data = {0};
hs_resample_init_data hs_resample_data = {
	.resample_rate = 0,
	.resample_bits = 0,
	.resample_channel = 0,
	.resample_enable = 0,
	.mozart_hs_resample_data = NULL,
};
#endif

#if (SUPPORT_BSA_AVK_RESAMPLE == 1)
af_resample_t* avk_resample_s;

avk_resample_init_data avk_resample_data = {
	.resample_rate = 0,
	.resample_bits = 0,
	.resample_channel = 0,
	.resample_enable = 0,
	.mozart_avk_resample_data = NULL,
};
#endif

#if (SUPPORT_AEC_RESAMPLE == 1)
af_resample_t* aec_resample_s;

bt_aec_sample_init_data bt_aec_sdata = {0};
bt_aec_resample_init_data bt_aec_rdata = {
	.resample_rate = 0,
	.resample_bits = 0,
	.resample_channel = 0,
	.resample_enable = 0,
	.aec_resample_data = NULL,
};
#endif

#if (SUPPORT_BSA_BLE == 1)
extern int bsa_ble_start_regular_enable;
extern int ble_service_create_enable;
extern int ble_service_add_char_enable;
extern int ble_service_start_enable;
extern int ble_client_open_enable;
extern int ble_client_read_enable;
extern int ble_client_write_enable;

#if (SUPPORT_BSA_BLE_SERVER == 1)
int ble_service_and_char_create()
{
	int i = 0;
	int state = 0;
	int server_num = 0;
	ble_create_service_data ble_create_service;
	ble_add_char_data ble_add_char;
	ble_config_adv_data ble_config_adv;
	ble_start_service_data ble_start_service;

	memset(&ble_create_service, 0, sizeof(ble_create_service_data));
	memset(&ble_add_char, 0, sizeof(ble_config_adv_data));
	memset(&ble_config_adv, 0, sizeof(ble_config_adv_data));
	memset(&ble_start_service, 0, sizeof(ble_start_service_data));


	server_num = mozart_bluetooth_ble_server_register(APP_BLE_MAIN_DEFAULT_APPL_UUID);
	if (server_num < 0) {
		printf("mozart_bluetooth_ble_server_register failed, state = %d.\n", state);
		return -1;
	}

	ble_create_service.server_num = server_num;
	ble_create_service.service_uuid = 0x180d;
	ble_create_service.num_handle = 4;
	ble_create_service.is_primary = 1;
	state = mozart_bluetooth_ble_create_service(&ble_create_service);
	if (state) {
		printf("mozart_bluetooth_ble_create_service failed, state = %d.\n", state);
		return -1;
	}
	for (i = 0; i < 30; i++) {
		if(ble_service_create_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_create_enable == 1)
		ble_service_create_enable = 0;
	else {
		printf("Error: not recived BSA_BLE_SE_CREATE_EVT, please checked!\n");
		return -1;
	}

	ble_add_char.server_num = server_num;
	ble_add_char.srvc_attr_num = ble_create_service.attr_num;
	ble_add_char.char_uuid = 0x2a37;
	ble_add_char.is_descript = 0;
	ble_add_char.attribute_permission = 0x11;
	ble_add_char.characteristic_property = 0x3a;
	state = mozart_bluetooth_ble_add_character(&ble_add_char);
	if (state) {
		printf("mozart_bluetooth_ble_add_character failed, state = %d.\n", state);
		return -1;
	}
	for (i = 0; i < 30; i++) {
		if(ble_service_add_char_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_add_char_enable == 1) {
		ble_service_add_char_enable = 0;
	} else {
		printf("Error: not recived BSA_BLE_SE_ADDCHAR_EVT, please checked!\n");
		return -1;
	}

	ble_config_adv.appearance_data = BLE_ADV_APPEARANCE_DATA;
	ble_config_adv.number_of_services = 1;
	ble_config_adv.services_uuid[0] = 0x180d;
	ble_config_adv.is_scan_rsp = 0;
	state = mozart_bluetooth_ble_configure_adv_data(&ble_config_adv);
	if (state) {
		printf("mozart_bluetooth_ble_configure_adv_data failed, state = %d.\n", state);
		return -1;
	}

	ble_start_service.server_num = server_num;
	ble_start_service.attr_num = ble_create_service.attr_num;
	state = mozart_bluetooth_ble_start_service(&ble_start_service);
	if (state) {
		printf("mozart_bluetooth_ble_start_service failed, state = %d.\n", state);
		return -1;
	}
	for (i = 0; i < 30; i++) {
		if(ble_service_start_enable != 1)
			usleep(300*1000);
		else
			break;
	}
	if (ble_service_start_enable == 1) {
		ble_service_start_enable = 0;
	} else {
		printf("Error: not recived BSA_BLE_SE_START_EVT, please checked!\n");
		return -1;
	}

	return 0;
}
#endif

#if (SUPPORT_BSA_BLE_CLIENT == 1)
int ble_client_create()
{
	int i = 0;
	int state = 0;
	int client_num = 0;
	int disc_index = -1;
	ble_client_connect_data cl_connect_data;
	ble_client_read_data client_rdata;
	ble_client_write_data cl_wdata;
	unsigned char *connect_ble_name = "Ingenic_BSA_BLE";
	UINT16 client_uuid = 0x4000;

	/* register ble client */
	client_num = state = mozart_bluetooth_ble_client_register(client_uuid);
	if (state != 0) {
		printf("ERROR: mozart_bluetooth_ble_client_register failed !\n");
		return -1;
	}
	for (i = 0; i < 5; i++) {
		/* discovery ble devices */
		state = mozart_bluetooth_ble_start_regular();
		if (state != 0) {
			printf("ERROR: mozart_bluetooth_ble_start_regular failed !\n");
			return -1;
		}

		while (bsa_ble_start_regular_enable != 1) {
			usleep(300*1000);
		}
		bsa_ble_start_regular_enable = 0;

		/* find connect_ble_name from discovered ble devices */
		disc_index = mozart_bluetooth_find_disc_device_name(connect_ble_name);
		if (disc_index != -1) {
			printf("mozart_bluetooth_find_disc_device_name %s success!\n", connect_ble_name);
			break;
		} else {
			printf("not find ble device: %s, rediscovery ble device!\n", connect_ble_name);
		}
	}
	if ((disc_index == -1) && (i >= 6)) {
		printf("ERROR: mozart_bluetooth_find_disc_device_name failed!\n");
		return -1;
	}

	/* connect ble server device */
	cl_connect_data.device_index = 1;	/*use Device found in last discovery */
	cl_connect_data.ble_name = connect_ble_name;
	cl_connect_data.client_num = client_num;
	cl_connect_data.direct = 1;
	mozart_bluetooth_ble_client_connect_server(&cl_connect_data);
	for (i = 0; i < 30; i++) {
		if(ble_client_open_enable != 1)
			usleep(300*1000);
		else {
			ble_client_open_enable = 0;
			break;
		}
	}
	if (i > 30) {
		printf("ble_client_open_enable failed!\n");
		return -1;
	}

	/* read connected ble device */
	client_rdata.service_uuid = 0x180d;
	client_rdata.char_uuid = 0x2a37;
	client_rdata.client_num = client_num;
	client_rdata.is_primary = 1;
	client_rdata.descr_id = 0;
	client_rdata.is_descript = 0;
	client_rdata.ser_inst_id = 0;
	client_rdata.char_inst_id = 0;
	state = mozart_bluetooth_ble_client_read(&client_rdata);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_read failed !\n");
		return -1;
	}
	for (i = 0; i < 30; i++) {
		if(ble_client_read_enable != 1)
			usleep(300*1000);
		else {
			ble_client_read_enable = 0;
			break;
		}
	}
	if (i > 30) {
		printf("ble_client_read_enable failed!\n");
		return -1;
	}

	/* write connected ble device */
	cl_wdata.write_len = 4;
	cl_wdata.service_uuid = 0x180d;
	cl_wdata.char_uuid = 0x2a37;
	cl_wdata.write_data[0] = 0x11;
	cl_wdata.write_data[1] = 0x22;
	cl_wdata.write_data[2] = 0x33;
	cl_wdata.write_data[3] = 0x44;

	cl_wdata.client_num = client_num;
	cl_wdata.is_primary = 1;
	cl_wdata.ser_inst_id = 0;
	cl_wdata.char_inst_id = 0;
	cl_wdata.is_descript = 0;
	cl_wdata.descr_id = 0;
	cl_wdata.desc_value = 0;
	cl_wdata.write_type = 1;
	state = mozart_bluetooth_ble_client_write(&cl_wdata);
	if (state != 0) {
		printf("mozart_bluetooth_ble_client_write failed !\n");
		return -1;
	}
	for (i = 0; i < 30; i++) {
		if(ble_client_write_enable != 1)
			usleep(300*1000);
		else {
			ble_client_write_enable = 0;
			break;
		}
	}
	if (i > 30) {
		printf("ble_client_write_enable failed!\n");
		return -1;
	}

	return 0;
}
#endif
#endif

void bt_info_init(bt_init_info *bt_info, char *bt_name)
{
	bt_info->bt_name = bt_name;
	bt_info->bt_ble_name = "Ingenic_BSA_BLE";
	bt_info->discoverable = 0;
	bt_info->connectable = 0;
	memset(bt_info->out_bd_addr, 0, sizeof(bt_info->out_bd_addr));
}


#if ((SUPPORT_AEC_RESAMPLE == 1) && (SUPPORT_AEC_RESAMPLE_48K_TO_8K == 1))
int bt_aec_resample_outlen_max = 0;
void mozart_bt_aec_resample_data_callback(bt_aec_resample_msg *bt_aec_rmsg)
{
	int resample_outlen =mozart_resample_get_outlen(bt_aec_rmsg->in_len, bt_aec_rdata.resample_rate, bt_aec_rdata.resample_channel, bt_aec_rdata.resample_bits, bt_aec_sdata.sample_rate);

	if (bt_aec_rmsg->out_buffer == NULL) {
		printf("bt_aec_rmsg->out_buffer == NULL !\n");
		bt_aec_resample_outlen_max = resample_outlen;
		bt_aec_rmsg->out_buffer = malloc(resample_outlen);
		bt_aec_rmsg->out_len = resample_outlen;
	} else {
		if (resample_outlen > bt_aec_resample_outlen_max) {
			printf("realloc !\n");
			bt_aec_resample_outlen_max = resample_outlen;
			bt_aec_rmsg->out_buffer = realloc(bt_aec_rmsg->out_buffer, resample_outlen);
			bt_aec_rmsg->out_len = resample_outlen;
		} else {
			bt_aec_rmsg->out_len = resample_outlen;
		}
	}
	bt_aec_rmsg->out_len = mozart_resample(aec_resample_s, bt_aec_rdata.resample_channel, bt_aec_sdata.sample_bits,
					    bt_aec_rmsg->in_buffer, bt_aec_rmsg->in_len, bt_aec_rmsg->out_buffer);
}
#endif

#if (SUPPORT_BSA_HS_RESAMPLE == 1)
int hs_resample_outlen_max = 0;
void mozart_hs_resample_data_callback(hs_resample_msg *hs_rmsg)
{
#if (SUPPORT_BSA_HS_RESAMPLE_8K_to_48K == 1)
	int resample_outlen = mozart_resample_get_outlen(hs_rmsg->in_len, hs_sample_data.sample_rate, hs_sample_data.sample_channel, hs_sample_data.sample_bits, hs_resample_data.resample_rate);

	if (hs_rmsg->out_buffer == NULL) {
		printf("hs_rmsg->out_buffer == NULL !\n");
		hs_resample_outlen_max = resample_outlen;
		hs_rmsg->out_buffer = malloc(resample_outlen);
		hs_rmsg->out_len = resample_outlen;
	} else {
		if (resample_outlen > hs_resample_outlen_max) {
			printf("realloc !\n");
			hs_resample_outlen_max = resample_outlen;
			hs_rmsg->out_buffer = realloc(hs_rmsg->out_buffer, resample_outlen);
			hs_rmsg->out_len = resample_outlen;
		} else {
			hs_rmsg->out_len = resample_outlen;
		}
	}
	hs_rmsg->out_len = mozart_resample(hs_resample_s, hs_sample_data.sample_channel, hs_sample_data.sample_bits,
					    hs_rmsg->in_buffer, hs_rmsg->in_len, hs_rmsg->out_buffer);
#else
	hs_rmsg->out_len = hs_rmsg->in_len;
	hs_rmsg->out_buffer = hs_rmsg->in_buffer;
#endif
}
#endif

#if (SUPPORT_BSA_AVK_RESAMPLE == 1)
int avk_resample_outlen_max = 0;
void mozart_avk_resample_data_callback(avk_callback_msg *avk_msg)
{
#if (SUPPORT_BSA_AVK_RESAMPLE_44K_to_48K == 1)
	int resample_outlen =mozart_resample_get_outlen(avk_msg->in_len, avk_msg->sample_rate, avk_msg->sample_channel, avk_msg->sample_bits, avk_resample_data.resample_rate);

	if (avk_msg->sample_rate == 48000) {
		avk_msg->out_len = avk_msg->in_len;
		avk_msg->out_buffer = avk_msg->in_buffer;
		return;
	}
	if (avk_msg->out_buffer == NULL) {
		printf("avk_msg->out_buffer == NULL !\n");
		avk_resample_outlen_max = resample_outlen;
		avk_msg->out_buffer = malloc(resample_outlen);
		avk_msg->out_len = resample_outlen;
	} else {
		if (resample_outlen > avk_resample_outlen_max) {
			printf("realloc !\n");
			avk_resample_outlen_max = resample_outlen;
			avk_msg->out_buffer = realloc(avk_msg->out_buffer, resample_outlen);
			avk_msg->out_len = resample_outlen;
		} else {
			avk_msg->out_len = resample_outlen;
		}
	}
	avk_msg->out_len = mozart_resample(avk_resample_s, avk_msg->sample_channel, avk_msg->sample_bits, avk_msg->in_buffer, avk_msg->in_len, avk_msg->out_buffer);

#elif (SUPPORT_BSA_AVK_RESAMPLE_2CHANNEL_to_1CHANNEL == 1)
	if (avk_msg->sample_channel == 1) {
		avk_msg->out_len = avk_msg->in_len;
		avk_msg->out_buffer = avk_msg->in_buffer;
		return;
	}
	int resample_outlen = avk_msg->in_len / 2;
	if (avk_msg->out_buffer == NULL) {
		printf("avk_msg->out_buffer == NULL !\n");
		avk_resample_outlen_max = resample_outlen;
		avk_msg->out_buffer = malloc(resample_outlen);
		avk_msg->out_len = resample_outlen;
	} else {
		if (resample_outlen > avk_resample_outlen_max) {
			printf("realloc !\n");
			avk_resample_outlen_max = resample_outlen;
			avk_msg->out_buffer = realloc(avk_msg->out_buffer, resample_outlen);
			avk_msg->out_len = resample_outlen;
		} else {
			avk_msg->out_len = resample_outlen;
		}
	}

	int16_t* tin = (int16_t *)avk_msg->in_buffer;
	int i = avk_msg->in_len / 4;
	int16_t* tout = (int16_t *)avk_msg->out_buffer;
	while (i--) {
		*tout = *tin/2 + *(tin+1)/2;
		tin += 2;
		tout += 1;
	}
#else
	avk_msg->out_len = avk_msg->in_len;
	avk_msg->out_buffer = avk_msg->in_buffer;
#endif
}
#endif

void *thr_fn(void *args)
{
	int i = 0;
	bt_init_info bt_info;
	char mac[] = "00:11:22:33:44:55";
	char bt_name[64] = {};
	char bt_avk_name[25] = "/var/run/bt-avk-fifo";
	char bt_socket_name[30] = "/var/run/bt-daemon-socket";
	BOOLEAN discoverable = true;
	BOOLEAN connectable = true;

	for(i = 0; i < 100; i++) {
		if(!access(bt_socket_name, 0) && !access(bt_avk_name, 0)) {
			break;
		} else {
			usleep(50000);
		}
	}

	if(access(bt_socket_name, 0) || access(bt_avk_name, 0)) {
		printf("%s or %s not exists, please check !!\n",
				bt_avk_name, bt_socket_name);
		goto err_exit;
	}

	memset(mac, 0, sizeof(mac));
	memset(bt_name, 0, sizeof(bt_name));
	get_mac_addr("wlan0", mac, "");
	strcat(bt_name, "SmartAudio-");
	strcat(bt_name, mac+4);
	bt_info_init(&bt_info, bt_name);
#if (SUPPORT_BT == BT_RTK)
	system("bt_enable &");
#elif (SUPPORT_BT == BT_BCM)
	printf("Bluetooth name is: %s\n", bt_name);
	if (mozart_bluetooth_init(&bt_info)) {
		printf("bluetooth init failed.\n");
		goto err_exit;
	}

	if (mozart_bluetooth_hs_start_service()) {
		printf("hs service start failed.\n");
		goto err_exit;
	}

	if (mozart_bluetooth_avk_start_service()) {
		printf("avk service start failed.\n");
		goto err_exit;
	}

	mozart_bluetooth_set_visibility(discoverable, connectable);
#if (SUPPORT_BSA_BLE == 1)
	int state = 0;
	usleep(500*1000);

	state = mozart_bluetooth_ble_start();
	if (state) {
		printf("mozart_bluetooth_ble_start failed, state = %d.\n", state);
		return -1;
	}
#if (SUPPORT_BSA_BLE_SERVER == 1)
	/* create ble service */
	state = ble_service_and_char_create();
	if (state != 0) {
		printf("ERROR: ble_service_and_char_create failed !\n");
		goto err_exit;
	}
#endif	/* SUPPORT_BSA_BLE_SERVER */
#if (SUPPORT_BSA_BLE_CLIENT == 1)
	/* create ble client */
	state = ble_client_create();
	if (state != 0) {
		printf("ERROR: ble_client_create failed !\n");
		goto err_exit;
	}
#endif /* SUPPORT_BSA_BLE_CLIENT */
	mozart_bluetooth_ble_set_visibility(discoverable, connectable);
#endif /* SUPPORT_BSA_BLE */

#if (SUPPORT_BSA_HS_RESAMPLE == 1)
	mozart_get_hs_default_sampledata(&hs_sample_data);
#if (SUPPORT_BSA_HS_RESAMPLE_8K_to_48K == 1)
	hs_resample_data.resample_rate = 48000;
	hs_resample_s = mozart_resample_init(hs_sample_data.sample_rate, hs_sample_data.sample_channel, hs_sample_data.sample_bits>>3, hs_resample_data.resample_rate);
	if(hs_resample_s == NULL){
		printf("ERROR: mozart resample inti failed\n");
		goto err_exit;
	}
#else
	hs_resample_data.resample_rate = hs_sample_data.sample_rate;
#endif /* SUPPORT_BSA_HS_RESAMPLE_8K_to_48K */
	hs_resample_data.resample_bits = hs_sample_data.sample_bits;
	hs_resample_data.resample_channel = hs_sample_data.sample_channel;
	hs_resample_data.resample_enable = 1;
	hs_resample_data.mozart_hs_resample_data = mozart_hs_resample_data_callback;

	mozart_set_hs_sampledata_callback(&hs_resample_data);
#endif /* SUPPORT_BSA_HS_RESAMPLE */

#if (SUPPORT_BSA_AVK_RESAMPLE == 1)
#if (SUPPORT_BSA_AVK_RESAMPLE_44K_to_48K == 1)
	avk_resample_data.resample_rate = 48000;
	avk_resample_data.resample_channel = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_bits = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_enable = 1;
	avk_resample_data.mozart_avk_resample_data = mozart_avk_resample_data_callback;

	avk_resample_s = mozart_resample_init(44100, 2, 2, avk_resample_data.resample_rate);
	if(avk_resample_s == NULL){
		printf("ERROR: mozart resample inti failed\n");
		goto err_exit;
	}
#elif (SUPPORT_BSA_AVK_RESAMPLE_2CHANNEL_to_1CHANNEL == 1)
	avk_resample_data.resample_channel = 1;
	avk_resample_data.resample_rate = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_bits = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_enable = 1;
	avk_resample_data.mozart_avk_resample_data = mozart_avk_resample_data_callback;
#else
	avk_resample_data.resample_rate = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_channel = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_bits = BSA_AVK_ORI_SAMPLE;
	avk_resample_data.resample_enable = 1;
	avk_resample_data.mozart_avk_resample_data = mozart_avk_resample_data_callback;
#endif

	mozart_set_avk_sampledata_callback(&avk_resample_data);
#endif /* SUPPORT_BSA_AVK_RESAMPLE */

#if ((SUPPORT_AEC_RESAMPLE == 1) && (SUPPORT_AEC_RESAMPLE_48K_TO_8K == 1))
	mozart_get_bt_aec_default_sampledata(&bt_aec_sdata);
	bt_aec_rdata.resample_rate = hs_resample_data.resample_rate;
	bt_aec_rdata.resample_bits = hs_resample_data.resample_bits;
	bt_aec_rdata.resample_channel = hs_resample_data.resample_channel;

	aec_resample_s = mozart_resample_init(hs_sample_data.sample_rate, hs_sample_data.sample_channel, hs_sample_data.sample_bits>>3, hs_resample_data.resample_rate);
	if(aec_resample_s == NULL){
		printf("ERROR: mozart resample inti failed\n");
		goto err_exit;
	}

	bt_aec_rdata.resample_enable = 1;
	bt_aec_rdata.aec_resample_data = mozart_bt_aec_resample_data_callback;
	mozart_set_bt_aec_sampledata_callback(&bt_aec_rdata);
#endif /* SUPPORT_AEC_RESAMPLE */

#elif (SUPPORT_BT == BT_NULL)
	printf("Bt funcs are closed.\n");
	goto err_exit;
#else
#error "Not supported bt module found."
#endif /* SUPPORT_BT */

err_exit:

	return NULL;
}

int start_bt(void)
{
	int err;
	pthread_t p_tid;

	system("/usr/fs/etc/init.d/S04bsa.sh start");

#if (SUPPORT_WEBRTC == 1)
	bt_ac.aec_init = ingenic_apm_init;
	bt_ac.aec_destroy = ingenic_apm_destroy;
	bt_ac.aec_enable = webrtc_aec_enable;
	bt_ac.aec_get_buffer_length = webrtc_aec_get_buffer_length;
	bt_ac.aec_calculate = webrtc_aec_calculate;

	mozart_aec_callback(&bt_ac);
#else
	mozart_aec_callback(NULL);
#endif
	err = pthread_create(&p_tid, NULL, thr_fn, NULL);
	if (err != 0)
		printf("can't create thread: %s\n", strerror(err));

	pthread_detach(p_tid);

	return 0;
}

int stop_bt(void)
{
	mozart_bluetooth_disconnect(USE_HS_AVK);
	mozart_bluetooth_hs_stop_service();
	mozart_bluetooth_avk_stop_service();
	mozart_bluetooth_uninit();
#if ((SUPPORT_BSA_HS_RESAMPLE == 1) && (SUPPORT_BSA_HS_RESAMPLE_8K_to_48K == 1))
	mozart_resample_uninit(hs_resample_s);
#endif
#if ((SUPPORT_AEC_RESAMPLE == 1) && (SUPPORT_AEC_RESAMPLE_48K_TO_8K == 1))
	mozart_resample_uninit(aec_resample_s);
#endif
#if ((SUPPORT_BSA_AVK_RESAMPLE == 1) && (SUPPORT_BSA_AVK_RESAMPLE_44K_to_48K == 1))
	mozart_resample_uninit(avk_resample_s);
#endif

	system("/usr/fs/etc/init.d/S04bsa.sh stop");

	return 0;
}
