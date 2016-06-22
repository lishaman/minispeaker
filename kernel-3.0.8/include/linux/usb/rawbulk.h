/*
 * Rawbulk Driver from VIA Telecom
 *
 * Copyright (C) 2011 VIA Telecom, Inc.
 * Author: Karfield Chen (kfchen@via-telecom.com)
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RAWBULK_H__
#define __RAWBULK_H__

#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/wakelock.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>

#define INTF_DESC       0
#define BULKIN_DESC     1
#define BULKOUT_DESC    2
#define MAX_DESC_ARRAY  4

#define NAME_BUFFSIZE   64
#define MAX_ATTRIBUTES    10

#define MAX_TTY_RX          4
#define MAX_TTY_RX_PACKAGE  512
#define MAX_TTY_TX          8
#define MAX_TTY_TX_PACKAGE  64

struct rawbulk_function {
	char name[32];
	char longname[32];
	struct device *dev;

	/* Controls */
	spinlock_t lock;
	int enable: 1;
	int activated: 1; /* set when usb enabled */
	int tty_opened: 1;

	struct work_struct activator; /* asynic transaction starter */

	struct wake_lock keep_awake;

	/* USB Gadget related */
	struct usb_function function;
	struct usb_composite_dev *cdev;
	struct usb_ep *bulk_out, *bulk_in;

	int rts_state;          /* Handshaking pins (outputs) */
	int dtr_state;
	int cts_state;                   /* Handshaking pins (inputs) */
	int dsr_state;
	int dcd_state;
	int ri_state;

	/* Transfer Controls */
	int nups;
	int ndowns;
	int upsz;
	int downsz;
	int splitsz;
	int autoreconn;
	int pushable;   /* Set to use push-way for upstream */

	struct rawbulk_transfer *transfer;
	/* Descriptors and Strings */
	struct usb_descriptor_header *fs_descs[MAX_DESC_ARRAY];
	struct usb_descriptor_header *hs_descs[MAX_DESC_ARRAY];
	struct usb_string string_defs[2];
	struct usb_gadget_strings string_table;
	struct usb_gadget_strings *strings[2];
	struct usb_interface_descriptor interface;
	struct usb_endpoint_descriptor fs_bulkin_endpoint;
	struct usb_endpoint_descriptor hs_bulkin_endpoint;
	struct usb_endpoint_descriptor fs_bulkout_endpoint;
	struct usb_endpoint_descriptor hs_bulkout_endpoint;

	/* Sysfs Accesses */
	int max_attrs;
	struct device_attribute attr[MAX_ATTRIBUTES];
	struct list_head list;
	struct class *rawbulk_class;
};

#define RAWBULK_INCEPT_FLAG_ENABLE          0x01
#define RAWBULK_INCEPT_FLAG_PUSH_WAY        0x02
typedef int (*rawbulk_intercept_t)(struct usb_interface *interface, unsigned int flags);
typedef void (*rawbulk_autoreconn_callback_t)(struct rawbulk_function *fn);

struct rawbulk_function *rawbulk_lookup_function(char *name);

/* bind/unbind for gadget */
int rawbulk_bind_transfer(struct rawbulk_transfer *transfer, struct usb_function *function,
		struct usb_ep *bulk_out, struct usb_ep *bulk_in,
		rawbulk_autoreconn_callback_t autoreconn_callback);
void rawbulk_unbind_transfer(struct rawbulk_transfer *transfer);
int rawbulk_check_enable(struct rawbulk_function *fn);
void rawbulk_disable_function(struct rawbulk_function *fn);

/* bind/unbind host interfaces */
struct rawbulk_transfer *rawbulk_bind_host_interface(struct usb_interface *interface,
		rawbulk_intercept_t inceptor,char *name);
void rawbulk_unbind_host_interface(struct usb_interface *interface,
		struct rawbulk_transfer *transfer);

/* operations for transactions */
int rawbulk_start_transactions(struct rawbulk_transfer *transfer, int nups, int ndowns,
		int upsz, int downsz, int splitsz, int pushable);
void rawbulk_stop_transactions(struct rawbulk_transfer *transfer);
int rawbulk_halt_transactions(struct rawbulk_transfer *transfer);
int rawbulk_resume_transactions(struct rawbulk_transfer *transfer);

int rawbulk_suspend_host_interface(struct rawbulk_transfer *transfer, pm_message_t message);
int rawbulk_resume_host_interface(struct rawbulk_transfer *transfer);
int rawbulk_push_upstream_buffer(struct rawbulk_transfer *transfer, const void *buffer,
		unsigned int length);

int rawbulk_forward_ctrlrequest(struct rawbulk_transfer *transfer, const struct usb_ctrlrequest *ctrl);

/* statics and state */
int rawbulk_transfer_statistics(struct rawbulk_transfer *transfer, char *buf);
int rawbulk_transfer_state(struct rawbulk_transfer *transfer);

struct rawbulk_transfer *rawbulk_transfer_get(const char *name);
#endif /* __RAWBULK_HEADER_FILE__ */
