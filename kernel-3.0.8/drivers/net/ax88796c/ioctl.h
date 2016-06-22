/*
 *********************************************************************************
 *     Copyright (c) 2005	ASIX Electronic Corporation      All rights reserved.
 *
 *     This is unpublished proprietary source code of ASIX Electronic Corporation
 *
 *     The copyright notice above does not evidence any actual or intended
 *     publication of such source code.
 *********************************************************************************
 */
 
#ifndef ioctl_h
#define ioctl_h

/* INCLUDE FILE DECLARATIONS */
#include "command.h"

/* NAMING CONSTANT DECLARATIONS */

struct ax_command_info { 
	int inet_sock;
	struct ifreq *ifr;
	int argc;
	char **argv;
	unsigned short ioctl_cmd;
	const char *help_ins;
	const char *help_desc;
};

const char help_str1[] =
"./ioctl help [command]\n"
"    -- command description\n";
const char help_str2[] =
"        [command] - Display usage of specified command\n";

const char genpkt_str1[] =
"./ioctl genpkt [type] [speed] [length] [pattern]\n"
"    -- Configure AX88796C packet generator\n";
const char genpkt_str2[] =
"        [type]      - 0: Disable  1:Random Pattern  2:Fixed Pattern\n"
"        [speed]     - 10: Force link at 10M  100:Force link at 100M\n"
"        [length]    - Length of packet\n"
"        [pattern]   - Pattern data (HEX)\n";

/* EXPORTED SUBPROGRAM SPECIFICATIONS */
void help_func (struct ax_command_info *info);
void genpkt_func (struct ax_command_info *info);
/* TYPE DECLARATIONS */

typedef void (*MENU_FUNC)(struct ax_command_info *info);

struct {
	char *cmd;
	unsigned short ioctl_cmd;
	MENU_FUNC OptFunc;
	const char *help_ins;
	const char *help_desc;
} command_list[] = {
	{"help",	0,			help_func,		help_str1,	help_str2},
	{"genpkt",	AX_CFG_TEST_PKT,	genpkt_func,		genpkt_str1,	genpkt_str2},
	{NULL},
};

#endif /* End of console_debug_h */
