/*
 * drivers/input/input-group.c
 *
 * Inter-device notify facility
 *
 * Copyright (C) 2012 Fighter Sun <wanmyqawdr@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _INPUT_GROUP_H
#define _INPUT_GROUP_H

#include <linux/time.h>
#include <linux/list.h>
#include <linux/input.h>
#include <linux/spinlock.h>

struct input_group {
	const char *name;

	struct input_dev *master;
	struct list_head members;
	struct mutex lock;

	struct list_head node;
};

struct input_member {
	struct input_dev* input;

	struct list_head node;

	int (*lock)(struct input_member *member);
	int (*unlock)(struct input_member *member);

	int (*remove)(struct input_member *member);
};

int input_register_group(struct input_group *group);
void input_unregister_group(struct input_group *group);

struct input_group *input_find_group(const char *name);

int input_register_member(struct input_group *group, struct input_member *member);
void input_unregister_member(struct input_group *group, struct input_member *member);

int input_group_lock(struct input_group *group);
int input_group_unlock(struct input_group *group);

#endif
