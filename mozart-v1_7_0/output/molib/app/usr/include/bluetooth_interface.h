/**
 * @file bluetooth_interface.h
 * @brief For the operation of the Bluetooth API
 * @author <zhe.wu@ingenic.com>
 * @version 1.0.0
 * @date 2015-06-15
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *
 * The program is not free, Ingenic without permission,
 * no one shall be arbitrarily (including but not limited
 * to: copy, to the illegal way of communication, display,
 * mirror, upload, download) use, or by unconventional
 * methods (such as: malicious intervention Ingenic data)
 * Ingenic's normal service, no one shall be arbitrarily by
 * software the program automatically get Ingenic data
 * Otherwise, Ingenic will be investigated for legal responsibility
 * according to law.
 */

#ifndef __BLUETOOTH_INTERFACE_H_
#define __BLUETOOTH_INTERFACE_H_

#define BSA_BLE_CL_WRITE_MAX		100
#define GATT_MAX_ATTR_LEN		600
#define BSA_BLE_MAX_ATTR_LEN		GATT_MAX_ATTR_LEN
#define BSA_DISC_VID_PID_MAX		1
#define HCI_EXT_INQ_RESPONSE_LEN        240
#define BD_ADDR_LEN			6
#define BD_NAME_LEN			248
#define DEV_CLASS_LEN			3
#define BSA_EIR_DATA_LENGTH		HCI_EXT_INQ_RESPONSE_LEN
#define APP_BLE_MAIN_DEFAULT_APPL_UUID	9000
#define BSA_DM_BLE_AD_UUID_MAX		6   /*Max number of Service UUID the device can advertise*/
#define BLE_ADV_APPEARANCE_DATA         0x1122
#define BSA_AVK_ORI_SAMPLE		0
#define BSA_HS_ORI_SAMPLE		0

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef unsigned int    UINT32;
typedef unsigned char   BOOLEAN;

typedef UINT8		tBT_DEVICE_TYPE;
typedef UINT8		BD_ADDR[BD_ADDR_LEN];
typedef UINT8		BD_NAME[BD_NAME_LEN + 1];	/* Device name */
typedef UINT8		DEV_CLASS[DEV_CLASS_LEN];	/* Device class */
typedef UINT16		tBSA_STATUS;
typedef UINT32		tBTA_SERVICE_MASK;
typedef tBTA_SERVICE_MASK	tBSA_SERVICE_MASK;

enum {
	/* means not currently in call set up */
	CALLSETUP_STATE_NO_CALL_SETUP = 0,
	/* means an incoming call process ongoing */
	CALLSETUP_STATE_INCOMMING_CALL,
	/* means an outgoing call set up is ongoing */
	CALLSETUP_STATE_OUTGOING_CALL,
	/* means remote party being alerted in an outgoing call */
	CALLSETUP_STATE_REMOTE_BEING_ALERTED_IN_OUTGOING_CALL,
	/* means a call is waiting */
	CALLSETUP_STATE_WAITING_CALL,

	/* means there are no calls in progress */
	CALL_STATE_NO_CALLS_ONGOING,
	/* means at least one call is in progress */
	CALL_STATE_LEAST_ONE_CALL_ONGOING,

	/* No calls held */
	CALLHELD_STATE_NO_CALL_HELD,
	/* Call is placed on hold or active/held calls swapped */
	CALLHELD_STATE_PLACED_ON_HOLD_OR_SWAPPED,
	/* Call on hold, no active call */
	CALLHELD_STATE_CALL_ON_HOLD_AND_NO_ACTIVE_CALL

} BSA_CALL_STATE;

typedef enum {
	BT_LINK_DISCONNECTING,
	BT_LINK_DISCONNECTED,
	BT_LINK_CONNECTING,
	BT_LINK_CONNECTED,
} bsa_bt_link_status;

typedef enum {
	BTHF_CHLD_TYPE_RELEASEHELD,
	BTHF_CHLD_TYPE_RELEASEACTIVE_ACCEPTHELD,
	BTHF_CHLD_TYPE_HOLDACTIVE_ACCEPTHELD,
	BTHF_CHLD_TYPE_ADDHELDTOCONF
} tBSA_BTHF_CHLD_TYPE_T;

enum {
	USE_HS_AVK = 0,
	USE_HS_ONLY,
	USE_AVK_ONLY
};

typedef enum {
	BTHF_VOLUME_TYPE_SPK = 0,	/* Update speaker volume */
	BTHF_VOLUME_TYPE_MIC = 1	/* Update microphone volume */
} tBSA_BTHF_VOLUME_TYPE_T;

typedef struct bt_last_device_info {
	BD_NAME bd_name;
	BD_ADDR bd_addr;
} bd_last_info;

/* Discovery callback events */
typedef enum
{
	BSA_DISC_NEW_EVT, 		/* a New Device has been discovered */
	BSA_DISC_CMPL_EVT, 		/* End of Discovery */
	BSA_DISC_DEV_INFO_EVT, 		/* Device Info discovery event */
	BSA_DISC_REMOTE_NAME_EVT  	/* Read remote device name event */
} tBSA_DISC_EVT;

/* Vendor and Product Identification
 * of the peer device
 * */
typedef struct
{
	BOOLEAN valid; 			/* TRUE if this entry is valid */
	UINT16 vendor_id_source; 	/* Indicate if the vendor field is BT or USB */
	UINT16 vendor; 			/* Vendor Id of the peer device */
	UINT16 product; 		/* Product Id of the peer device */
	UINT16 version; 		/* Version of the peer device */
} tBSA_DISC_VID_PID;

typedef struct
{
	BD_ADDR bd_addr; 		/* BD address peer device. */
	DEV_CLASS class_of_device; 	/* Class of Device */
	BD_NAME name; 			/* Name of peer device. */
	int rssi; 			/* The rssi value */
	tBSA_SERVICE_MASK services; 	/* Service discovery discovered */
	tBSA_DISC_VID_PID eir_vid_pid[BSA_DISC_VID_PID_MAX];
	UINT8 eir_data[BSA_EIR_DATA_LENGTH];  /* Full EIR data */
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
	UINT8 inq_result_type;
	UINT8 ble_addr_type;
	tBT_DEVICE_TYPE device_type;
#endif
} tBSA_DISC_REMOTE_DEV;

typedef struct
{
	tBSA_STATUS status;		/* Status of the request */
	BD_ADDR bd_addr;		/* BD address peer device. */
	UINT8 index;
	UINT16 spec_id;
	BOOLEAN primary;
	UINT16 vendor;
	UINT16 vendor_id_source;
	UINT16 product;
	UINT16 version;
} tBSA_DISC_DEV_INFO_MSG;

typedef struct
{
	UINT16      status;
	BD_ADDR     bd_addr;
	UINT16      length;
	BD_NAME     remote_bd_name;
} tBSA_DISC_READ_REMOTE_NAME_MSG;

/* Structure associated with BSA_DISC_NEW_EVT */
typedef tBSA_DISC_REMOTE_DEV tBSA_DISC_NEW_MSG;

/* Union of all Discovery callback structures */
typedef union
{
	tBSA_DISC_NEW_MSG disc_new;			/* a New Device has been discovered */
	tBSA_DISC_DEV_INFO_MSG dev_info;		/* Device Info of a device */
	tBSA_DISC_READ_REMOTE_NAME_MSG remote_name; 	/* Name of Remote device */
} tBSA_DISC_MSG;

typedef struct {
	UINT8	*in_buffer;
	UINT8	*out_buffer;
	UINT16	in_len;
	UINT16	out_len;
	int 	sample_rate;
	int 	sample_channel;
	int 	sample_bits;
} avk_callback_msg;

typedef void (tBSA_DISC_CBACK)(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data);
typedef unsigned int (*aec_func_t)(void);
typedef void *(*aec_calculate_t)(void *buf_mic, void *buf_ref, void *buf_result, unsigned int size);

typedef struct {
	char *in_buffer;
	char *out_buffer;
	unsigned int in_len;
	unsigned int out_len;
} bt_aec_resample_msg;

typedef void *(*bt_aec_resample_t)(bt_aec_resample_msg *bt_aec_rmsg);

typedef struct {
	char *in_buffer;
	char *out_buffer;
	unsigned int in_len;
	unsigned int out_len;
} hs_resample_msg;

typedef void *(*hs_resample_t)(hs_resample_msg *hs_msg);

typedef struct bt_aec_callback_interface {
	aec_func_t aec_enable;
	aec_func_t aec_get_buffer_length;
	aec_calculate_t aec_calculate;
	aec_func_t aec_init;
	aec_func_t aec_destroy;
} bt_aec_callback;

typedef struct {
	int sample_rate;
	int sample_channel;
	int sample_bits;
} hs_sample_init_data;

typedef struct {
	int sample_rate;
	int sample_channel;
	int sample_bits;
} bt_aec_sample_init_data;

typedef struct {
	int resample_rate;
	int resample_channel;
	int resample_bits;
	int resample_enable;
	bt_aec_resample_t aec_resample_data;
} bt_aec_resample_init_data;

typedef struct {
	int resample_rate;
	int resample_channel;
	int resample_bits;
	int resample_enable;
	hs_resample_t mozart_hs_resample_data;
} hs_resample_init_data;

typedef struct {
	int sample_rate;
	int sample_channel;
	int sample_bits;
} avk_sample_init_data;

typedef void *(*avk_resample_t)(avk_callback_msg *avk_msg);
typedef struct {
	int resample_rate;
	int resample_channel;
	int resample_bits;
	int resample_enable;
	avk_resample_t mozart_avk_resample_data;
} avk_resample_init_data;

typedef struct bt_manager_init_info
{
	char *bt_name;
	char *bt_ble_name;
	int discoverable;
	int connectable;
	unsigned char out_bd_addr[BD_ADDR_LEN];
} bt_init_info;

typedef struct bsa_ble_create_service_data {
	int	server_num;	/* server number which is distributed */
	int	attr_num;
	UINT16	service_uuid;	/*service uuid */
	UINT16  num_handle;
	BOOLEAN	is_primary;
} ble_create_service_data;

typedef struct bsa_ble_config_advertisement_data {
	UINT16	appearance_data;	/* Device appearance data, Eg:0x1122 */
	UINT16	number_of_services;	/* create services number */
	UINT16  services_uuid[BSA_DM_BLE_AD_UUID_MAX]; /*servies uuid value, eg:0xA108 */
	BOOLEAN	is_scan_rsp;		/* is the data scan response or adv data */
} ble_config_adv_data;

typedef struct bsa_ble_start_service_data {
	int server_num;
	int attr_num;
} ble_start_service_data;

typedef struct bsa_ble_add_character_data {
	int server_num;
	int srvc_attr_num;
	int char_attr_num;
	int is_descript;
	int attribute_permission;
	int characteristic_property;
	UINT16	char_uuid;
} ble_add_char_data;

typedef struct {
	UINT16 service_uuid;	/* Enter Service UUID to read(eg. x1800) */
	UINT16 char_uuid;	/* Enter Char UUID to read(eg. x2a00) */
	UINT16 descr_id;	/* Enter Descriptor type UUID to read(eg. x2902) */
	int client_num;
	int is_descript;	/* Select descriptor? (yes=1 or no=0),default 0 */
	int ser_inst_id;	/* Enter Instance ID for Service UUID(eg. 0,1,2..), default 0 */
	int char_inst_id;	/* Enter Instance ID for Char UUID(eg. 0,1,2..) */
	int is_primary;		/* Enter Is_primary value(eg:0,1) */
} ble_client_read_data;

typedef struct {
	UINT16 service_uuid;	/* Service UUID to write */
	UINT16 char_uuid;	/* Char UUID to write */
	UINT16 descr_id;	/* Descriptor type UUID to write(eg. x2902) */
	UINT8 desc_value;	/* Descriptor value to write(eg. x01) */
	UINT8 write_data[BSA_BLE_CL_WRITE_MAX];		/* write data */
	UINT16 write_len;	/* write length: bytes */
	UINT8 write_type;	/* 1-GATT_WRITE_NO_RSP 2-GATT_WRITE */
	int client_num;
	int is_descript;	/* select descriptor? (yes=1 or no=0) */
	int ser_inst_id;	/* Instance ID for Service UUID, default 0 */
	int char_inst_id;	/* Instance ID for Char UUID(eg. 0,1,2..) */
	int is_primary;		/* Is_primary value(eg:0,1) */
} ble_client_write_data;

typedef struct {
	int device_index;	/* 0 Device from XML database (already paired), 1 Device found in last discovery */
	unsigned char *ble_name; 	/* The ble device you want to connect */
	int client_num;
	int direct;		/* Direct connection:1, Background connection:0 */
} ble_client_connect_data;

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_init
 **
 ** Description      Initial bluetooth function.
 **
 ** Parameters	     bt_name:       bluetooth device name
 **		     discoverable:  0 is no discovery, 1 is discoverable.
 ** 		     connectable:   0 is disconnect,   1 is connectable.
 **		     out_bd_addr:   0 is use bt controler bd_addr, other is user-defined.
 **
 ** Returns          0 is ok, -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_init(bt_init_info *bt_info);
extern int mozart_bluetooth_uninit(void);

/************************************ HS servie *********************************/
/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_start_service
 **
 ** Description      Start bluetooth HS service
 **
 ** Parameters	     void
 **
 ** Returns          0 is ok, -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_hs_start_service(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_stop_service
 **
 ** Description      Stop bluetooth HS service
 **
 ** Parameters	     void
 **
 ** Returns          0 is ok, -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_hs_stop_service(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_get_call_state
 **
 ** Description      Access to bluetooth calls state
 **
 ** Parameters	     void
 **
 ** Returns          CALL_STATE_NO_CALL_SETUP: no call
 **	     	     CALL_STATE_IS_ON_GOING: on going
 **		     CALL_STATE_IS_COMMING: is comming
 **
 *******************************************************************************/
extern int mozart_bluetooth_hs_get_call_state(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_answer_call
 **
 ** Description      Bluetooth answer the call
 **
 ** Parameters	     void
 **
 ** Returns          0 if OK, -1 otherwise.
 **
 *******************************************************************************/
extern int mozart_bluetooth_hs_answer_call(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_hangup
 **
 ** Description      Bluetooth hang up the call
 **
 ** Parameters	     void
 **
 ** Returns          0 if OK, -1 otherwise.
 **
 *******************************************************************************/
extern int mozart_bluetooth_hs_hangup(void);

/******************************** avk servie ***********************************/
/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_start_service
 **
 ** Description      Start bluetooth A2DP service
 **
 ** Parameters	     void
 **
 ** Returns          0 if OK, -1 otherwise.
 **
 *******************************************************************************/
extern int mozart_bluetooth_avk_start_service(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_stop_service
 **
 ** Description      Stop bluetooth A2DP service
 **
 ** Parameters	     void
 **
 ** Returns          0 if OK, -1 otherwise.
 **
 *******************************************************************************/
extern int mozart_bluetooth_avk_stop_service(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_start_play
 **
 ** Description      Bluetooth start and resume play
 **
 ** Parameters	     void
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_start_play(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_stop_play
 **
 ** Description      Bluetooth stop the current play
 **
 ** Parameters	     void
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_stop_play(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_play_pause
 **
 ** Description      Bluetooth execute play and pause command switch
 **
 ** Parameters	     void
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_play_pause(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_next_music
 **
 ** Description      Bluetooth play the next music
 **
 ** Parameters	     void
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_next_music(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_prev_music
 **
 ** Description      Bluetooth play the previous music
 **
 ** Parameters	     void
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_prev_music(void);

/*******************************************************************************
 **
 ** Function         mozart_blutooth_hs_set_volume
 **
 ** Description      Send volume AT Command
 **
 ** Parameters       type: speaker or microphone, volume: speaker or microphone volume value
 **
 ** Returns          0 if successful execution, error code else
 **
 *******************************************************************************/
extern int mozart_blutooth_hs_set_volume(tBSA_BTHF_VOLUME_TYPE_T type, int volume);

/*******************************************************************************
 **
 ** Function         mozart_aec_callback
 **
 ** Description      Bluetooth eliminate echo callback
 **
 ** Parameters       bt_aec_callback struct
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_aec_callback(bt_aec_callback *bt_ac);

/*******************************************************************************
 **
 ** Function         mozart_get_hs_default_sampledata
 **
 ** Description      mozart get hs default sample data
 **
 ** Parameters       struct hs_sample_init_data
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_get_hs_default_sampledata(hs_sample_init_data *sample_data);

/*******************************************************************************
 **
 ** Function         mozart_set_hs_sampledata_callback
 **
 ** Description      mozart set hs resample_data to bsa, bsa set resample_data to codec controler.
 **
 ** Parameters       struct hs_resample_init_data
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_set_hs_sampledata_callback(hs_resample_init_data *resample_data);
extern void mozart_get_bt_aec_default_sampledata(bt_aec_resample_init_data *sample_data);
extern void mozart_set_bt_aec_sampledata_callback(bt_aec_resample_init_data *resample_data);

extern void mozart_get_avk_default_sampledata(avk_sample_init_data *sample_data);
extern void mozart_set_avk_sampledata_callback(avk_resample_init_data *resample_data);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_read_last_device_info
 **
 ** Description      Read the last connected devices
 **
 ** Parameters       bd_last_info struct
 **
 ** Returns          1 if successful, error code otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_read_last_device_info(bd_last_info *bd_last_info);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_auto_reconnect
 **
 ** Description      Auto reconnect the last connected BT device
 **
 ** Parameters       type: USE_HS_AVK is use HFP and A2DP, USE_HS_ONLY is only use HFP, USE_AVK_ONLY is only use A2DP.
 **
 ** Returns          0 if successful
 **		     -1 is connect failed
 **		     -2 is bt_devices.xml not existed, no connected device
 **
 *******************************************************************************/
extern int mozart_bluetooth_auto_reconnect(int type);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_disconnect
 **
 ** Description      Bluetooth disconnect connection with bt device
 **
 ** Parameters       type: USE_HS_AVK is use HFP and A2DP, USE_HS_ONLY is only use HFP, USE_AVK_ONLY is only use A2DP.
 **
 ** Returns          0 if successful
 **		     -1 is disconnect failed
 **
 *******************************************************************************/
extern int mozart_bluetooth_disconnect(int type);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_set_link_status
 **
 ** Description      Bluetooth set current link status
 **
 ** Parameters       void
 **
 ** Returns          0: success, -1: FALSE
 **
 *******************************************************************************/
extern int mozart_bluetooth_set_link_status(bsa_bt_link_status bt_status);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_get_link_status
 **
 ** Description      Bluetooth get current link status
 **
 ** Parameters       void
 **
 ** Returns          linked : TRUE, unlink : FALSE
 **
 *******************************************************************************/
extern int mozart_bluetooth_get_link_status(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_disc_start_regular
 **
 ** Description      Start Device discovery
 **
 ** Returns          0 is ok, other is failed
 **
 *******************************************************************************/
extern int mozart_bluetooth_disc_start_regular(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_set_non_discoverable
 **
 ** Description      Set the device non discoverable
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_set_non_discoverable(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_set_discoverable
 **
 ** Description      Set the device discoverable for a specific time
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_set_discoverable(void);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_set_visibility
 **
 ** Description     Set the Device Visibility and connectable.
 **
 ** Parameters      discoverable: FALSE if not discoverable, TRUE is discoverable
 **                 connectable:  FALSE if not connectable, TRUE is connectable
 **
 ** Returns         0 if success, -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_set_visibility(BOOLEAN discoverable, BOOLEAN connectable);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_disc_get_new_device_number
 **
 ** Description     Get bluetooth search bt devices number
 **
 ** Parameters      void
 **
 ** Returns         > 0 : bt devices number, == 0 : no bt devices,
 **
 *******************************************************************************/
extern int mozart_bluetooth_disc_get_new_device_number(void);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_get_avk_play_state
 **
 ** Description     get avk plat status
 **
 ** Parameters      void
 **
 ** Returns          TRUE: play   FALSE: stop
 **
 *******************************************************************************/
extern int mozart_bluetooth_get_avk_play_state(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_ops_start
 **
 ** Description      start OPP Server application
 **
 ** Parameters       void
 **
 ** Returns          BSA_SUCCESS success, error code for failure
 **
 *******************************************************************************/
extern tBSA_STATUS mozart_bluetooth_ops_start(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_ops_stop
 **
 ** Description      stop OPP Server application
 **
 ** Parameters       void
 **
 ** Returns          BSA_SUCCESS success, error code for failure
 **
 *******************************************************************************/
extern tBSA_STATUS mozart_bluetooth_ops_stop(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_ops_auto_accept
 **
 ** Description      set ops auto accept object
 **
 ** Parameters       void
 **
 ** Returns          BSA_SUCCESS success, error code for failure
 **
 *******************************************************************************/
extern tBSA_STATUS mozart_bluetooth_ops_auto_accept(void);

/*******************************************************************************
 **
 ** function         mozart_bluetooth_opc_start
 **
 ** description      start opp client application
 **
 ** parameters       void
 **
 ** returns          bsa_success success, error code for failure
 **
 *******************************************************************************/
extern tBSA_STATUS mozart_bluetooth_opc_start(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_opc_stop
 **
 ** Description      stop OPP Client application
 **
 ** Parameters       void
 **
 ** Returns          BSA_SUCCESS success, error code for failure
 **
 *******************************************************************************/
extern tBSA_STATUS mozart_bluetooth_opc_stop(void);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_opc_set_key
 **
 ** Description      Select key to set opc Function
 **
 ** Parameters       APP_OPC_KEY_XXX
 **
 ** Returns          BSA_SUCCESS success, error code for failure
 **
 *******************************************************************************/
extern tBSA_STATUS mozart_bluetooth_opc_set_key(int choice);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_start
 **
 ** Description     start BSA BLE Function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_start();

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_close
 **
 ** Description     Close BSA BLE Function
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_close();

/*
 * BLE Server functions
 */
/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_server_register
 **
 ** Description     Register server app
 **
 ** Parameters      uuid
 **
 ** Returns         status: server_num if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_server_register(UINT16 uuid);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_server_deregister
 **
 ** Description     Deregister server app
 **
 ** Parameters      server_num: server number
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_server_deregister(int server_num);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_create_service
 **
 ** Description     create GATT service
 **
 ** Parameters      struct ble_service_info
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_create_service(ble_create_service_data *ble_create_service_data);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_configure_advertisement_data
 **
 ** Description     start BLE advertising
 **
 ** Parameters      struct bsa_ble_config_advertisement_data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_configure_adv_data(ble_config_adv_data *ble_config_adv_data);

/*******************************************************************************
 **
 ** Function       mozart_bluetooth_ble_start_service
 **
 ** Description     Start Service
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_start_service(ble_start_service_data *ble_start_service_data);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_add_character
 **
 ** Description     Add character to service
 **
 ** Parameters      struct bsa_ble_add_character
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_add_character(ble_add_char_data *ble_add_char_data);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_ble_server_display
 **
 ** Description      display BLE server
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
extern void mozart_bluetooth_ble_server_display();

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_set_visibility
 **
 ** Description     Set the Device BLE Visibility parameters
 **
 ** Parameters      discoverable: FALSE if not discoverable
 **                 connectable: FALSE if not connectable
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_set_visibility(BOOLEAN discoverable, BOOLEAN connectable);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_hold_call
 **
 ** Description      Hold active call
 **
 ** Parameters       void
 **
 ** Returns          0 if successful execution, error code else
 **
 *******************************************************************************/
extern int mozart_bluetooth_hs_hold_call(tBSA_BTHF_CHLD_TYPE_T type);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_audio_open
 **
 ** Description      Open the SCO connection alone
 ** 		     switch the phone call between the phone to the units
 **
 ** Parameter        None
 **
 ** Returns          0 if success, -1 if failure
 *******************************************************************************/
extern int mozart_bluetooth_hs_audio_open();

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_hs_audio_close
 **
 ** Description      Close the SCO connection alone
 ** 		     switch the phone call between the units to the phone
 **
 ** Parameter        None
 **
 ** Returns          0 if success, -1 if failure
 *******************************************************************************/
extern int mozart_bluetooth_hs_audio_close();

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_get_server_char_value
 **
 ** Description      Get BLE character value
 **
 ** Parameter        server_num: service num. character_num: character num. value: character value.
 **
 ** Returns          character value length if service enabled, -1 if service not enabled
 *******************************************************************************/
extern int mozart_bluetooth_get_server_char_value(int server_num, int character_num, UINT8 *value);


/* ---------------- ble client ------------------------------- */

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_client_register
 **
 ** Description     This is the ble client register command
 **
 ** Parameters      UINT16 uuid
 **
 ** Returns         status: client_num if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_client_register(UINT16 uuid);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_client_deregister
 **
 ** Description     This is the ble deregister app
 **
 ** Parameters      int client_num
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_client_deregister(int client_num);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_ble_start_regular
 **
 ** Description      Start BLE Device discovery
 **
 ** Returns          0 if success, -1 if failed
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_start_regular();

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_find_disc_device_name
 **
 ** Description      Find a device in the list of discovered devices
 **
 ** Parameters       index_device
 **
 ** Returns          index_device if successful, -1 is failed.
 **
 *******************************************************************************/
extern int mozart_bluetooth_find_disc_device_name(unsigned char *name);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_client_connect_server
 **
 ** Description     This is the ble open connection to ble server
 **
 ** Parameters      struct ble_client_connect_data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_client_connect_server(ble_client_connect_data *cl_connect_data);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_client_disconnect_server
 **
 ** Description     This is the ble close connection
 **
 ** Parameters      client_num
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_client_disconnect_server(int client_num);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_client_read
 **
 ** Description     This is the read function to read a characteristec or characteristic descriptor from BLE server
 **
 ** Parameters      struct ble_client_read_data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_client_read(ble_client_read_data *client_data);

/*******************************************************************************
 **
 ** Function        mozart_bluetooth_ble_client_write
 **
 ** Description     This is the write function to write a characteristic or characteristic discriptor to BLE server.
 **
 ** Parameters      ble_client_write_data
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
extern int mozart_bluetooth_ble_client_write(ble_client_write_data *cl_write_data);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_get_link_down_data
 **
 ** Description      get link down status
 **
 **
 ** Parameter        None
 **
 ** Reason code 19: Mobile terminal to close Bluetooth。Reason code 8: connection timeout
 *******************************************************************************/
extern UINT8 mozart_bluetooth_get_link_down_data();

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_set_volume_up
 **
 ** Description      This function sends absolute volume up
 **
 ** Returns          None
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_set_volume_up(UINT8 volume);

/*******************************************************************************
 **
 ** Function         mozart_bluetooth_avk_set_volume_down
 **
 ** Description      This function sends absolute volume down
 **
 ** Returns          None
 **
 *******************************************************************************/
extern void mozart_bluetooth_avk_set_volume_down(UINT8 volume);

#endif
