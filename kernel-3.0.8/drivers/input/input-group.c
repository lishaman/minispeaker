/*
 * drivers/input/input-group.c
 *
 * Inter-devices notify facility
 *
 * Copyright (C) 2012 Fighter Sun <wanmyqawdr@126.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This provide a easy way to send notifies
 * across members of input devices group
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/input-group.h>

static LIST_HEAD(group_list);
static DEFINE_MUTEX(group_lock);

int input_register_group(struct input_group *group) {
	INIT_LIST_HEAD(&group->members);
	INIT_LIST_HEAD(&group->node);

	mutex_init(&group->lock);

	group->name = group->master->name;

	mutex_lock(&group_lock);
	list_add_tail(&group->node, &group_list);
	mutex_unlock(&group_lock);

	return 0;
}
EXPORT_SYMBOL(input_register_group);

void input_unregister_group(struct input_group *group) {
	struct input_member *member;

	mutex_lock(&group->lock);
	list_for_each_entry(member, &group->members, node) {
		if (member->remove)
			member->remove(member);
	}
	mutex_unlock(&group->lock);

	mutex_lock(&group_lock);
	list_del(&group->node);
	mutex_unlock(&group_lock);
}
EXPORT_SYMBOL(input_unregister_group);

struct input_group *input_find_group(const char *name) {
	struct input_group *group;
	list_for_each_entry(group, &group_list, node)
		if (!strcmp(group->name, name))
			return group;

	return NULL ;
}
EXPORT_SYMBOL(input_find_group);

int input_register_member(struct input_group *group,
		struct input_member *member) {
	mutex_lock(&group->lock);
	list_add_tail(&member->node, &group->members);
	mutex_unlock(&group->lock);

	return 0;
}
EXPORT_SYMBOL(input_register_member);

void input_unregister_member(struct input_group *group,
		struct input_member *member) {
	mutex_lock(&group->lock);
	list_del(&member->node);
	mutex_unlock(&group->lock);
}
EXPORT_SYMBOL(input_unregister_member);

int input_group_lock(struct input_group *group) {
	struct input_member *member;
	int err = 0;

	list_for_each_entry(member, &group->members, node) {
		if (member->lock) {
			err = member->lock(member);
			if (err)
				break;
		}
	}

	return err;
}
EXPORT_SYMBOL(input_group_lock);

int input_group_unlock(struct input_group *group) {
	struct input_member *member;
	int err = 0;

	list_for_each_entry(member, &group->members, node) {
		if (member->unlock) {
			err = member->unlock(member);
			if (err)
				break;
		}
	}

	return err;
}
EXPORT_SYMBOL(input_group_unlock);
