/* include/linux/input/remote.h
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 *	Sun Jiwei<jwsun@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef	__REMOTE_H
#define __REMOTE_H

struct jz_remote_board_data {
	short gpio;
	unsigned int *key_maps;
	int (*init)(void *);
};

#endif	/*__REMOTE_H*/
