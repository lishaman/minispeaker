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
/*==========================================================================
 * Module Name : console_debug.c
 * Purpose     : 
 * Author      : 
 * Date        : 
 * Notes       :
 * $Log$
 *==========================================================================
 */
 
/* INCLUDE FILE DECLARATIONS */
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/sockios.h>

#include "ioctl.h"
#include "command.h"

/* STATIC VARIABLE DECLARATIONS */
#define AX88796C_IOCTL_VERSION		"AX88796C IOCTL version 0.1.5"

/* LOCAL SUBPROGRAM DECLARATIONS */
static unsigned long STR_TO_U32(const char *cp,char **endp,unsigned int base);


/* LOCAL SUBPROGRAM BODIES */
static void show_usage(void)
{
	int i;
	printf ("\n%s\n",AX88796C_IOCTL_VERSION);
	printf ("Usage:\n");
	for (i = 0; command_list[i].cmd != NULL; i++)
		printf ("%s\n", command_list[i].help_ins);
}

static unsigned long STR_TO_U32(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;

	return result;
}

void help_func (struct ax_command_info *info)
{
	int i;

	if (info->argv[2] == NULL) {
		for(i=0; command_list[i].cmd != NULL; i++) {
			printf ("%s%s\n", command_list[i].help_ins, command_list[i].help_desc);
		}

		return;
	}

	for(i=0; command_list[i].cmd != NULL; i++)
	{
		if (strncmp(info->argv[2], command_list[i].cmd, strlen(command_list[i].cmd)) == 0 ) {
			printf ("%s%s\n", command_list[i].help_ins, command_list[i].help_desc);
			return;
		}
	}

}

void genpkt_func(struct ax_command_info *info)
{
	unsigned long type, pattern = 0, length = 0, speed = 0;
	int ret;
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	AX_IOCTL_COMMAND ioctl_cmd;

	if (info->argc < 3) {
		printf ("Usage\n");
		printf ("%s%s\n", info->help_ins, info->help_desc);
		return;
	}

	type = STR_TO_U32(info->argv[2], NULL, 10);
	if (type > 2) {
		printf ("Wrong type!!\n");
		printf ("Usage\n");
		printf ("%s%s\n", info->help_ins, info->help_desc);
		return;
	}

	if (type == 1) {		/* random pattern */
		if (info->argc < 5) {
			printf ("Usage\n");
			printf ("%s%s\n", info->help_ins, info->help_desc);
			return;
		}

		speed = STR_TO_U32(info->argv[3], NULL, 10);
		length = STR_TO_U32(info->argv[4], NULL, 10);
	} else if (type == 2) {		/* Fixed pattern */
		if (info->argc < 6) {
			printf ("Usage\n");
			printf ("%s%s\n", info->help_ins, info->help_desc);
			return;
		}

		speed = STR_TO_U32(info->argv[3], NULL, 10);
		length = STR_TO_U32(info->argv[4], NULL, 10);
		pattern = STR_TO_U32(info->argv[5], NULL, 16);
	}

	ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
	ioctl_cmd.type = type;
	ioctl_cmd.speed = speed;
	ioctl_cmd.pattern = pattern;
	ioctl_cmd.length = length;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0)
		perror("ioctl");
	else {
		printf ("AX88796C packet generator configured\n");
	}
	return;
}

/* EXPORTED SUBPROGRAM BODIES */
int main(int argc, char **argv)
{
	int inet_sock;
	struct ifreq ifr;
	char buf[0xff];
	struct ax_command_info info;
	unsigned char i;
	char	input;
	unsigned char count = 0;
	int fd, console;
	const unsigned char length = sizeof(char);
	AX_IOCTL_COMMAND ioctl_cmd;

	if (argc < 2) {
		show_usage();
		return 0;
	}

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

	/*  */
	for (i = 0; i < 255; i++) {

		memset (&ioctl_cmd, 0, sizeof (AX_IOCTL_COMMAND));
		ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

		sprintf (ifr.ifr_name, "eth%d", i);
		ifr.ifr_data = (caddr_t)&ioctl_cmd;

		if (ioctl (inet_sock, AX_PRIVATE, &ifr) < 0) {
			continue;
		}

		if (strncmp (ioctl_cmd.sig, AX88796C_SIGNATURE, strlen(AX88796C_SIGNATURE)) == 0 ) {
			break;
		}

	}

	if (i == 255) {
		printf ("No %s found\n", AX88796C_SIGNATURE);
		return 0;
	}

	for(i=0; command_list[i].cmd != NULL; i++)
	{
		if (strncmp(argv[1], command_list[i].cmd, strlen(command_list[i].cmd)) == 0 ) {
			info.help_ins = command_list[i].help_ins;
			info.help_desc = command_list[i].help_desc;
			info.ifr = &ifr;
			info.argc = argc;
			info.argv = argv;
			info.inet_sock = inet_sock;
			info.ioctl_cmd = command_list[i].ioctl_cmd;
			(command_list[i].OptFunc)(&info);
			return 0;
		}
	}

	printf ("Wrong command\n");

	return 0;
}

