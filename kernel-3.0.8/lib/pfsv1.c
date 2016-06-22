/*
 *  Copyright (C) 2013 Fighter Sun <wanmyqawdr@126.com>
 *  implement of parameterized firmware specification v1
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/pfsv1.h>

struct pfsv1_datatype_offset
{
    PFSV1_SWORD size_param_desc;
    PFSV1_SWORD offset_param_desc_name;
    PFSV1_SWORD offset_param_desc_alias;
    PFSV1_SWORD offset_param_desc_type;
    PFSV1_SWORD offset_param_desc_addr;
    PFSV1_SWORD offset_param_desc_size;
    PFSV1_SWORD offset_param_desc_perm;
    PFSV1_SWORD offset_param_desc_brief;

    PFSV1_SWORD size_help_desc;
    PFSV1_SWORD offset_help_desc_name;
    PFSV1_SWORD offset_help_desc_text;
};

static struct pfsv1_param_desc dummy_param[2];
static struct pfsv1_help_utf8_desc dummy_help_utf8[2];

__attribute__((__used__))
__attribute__ ((__aligned__(sizeof (long))))
__attribute__ ((__section__(PFSV1_SECT_DATATYPE_OFFSET)))
static struct pfsv1_datatype_offset offset = {
    sizeof(dummy_param[0]),
    (PFSV1_SWORD) &dummy_param[0].name - (PFSV1_SWORD) &dummy_param[0],
    (PFSV1_SWORD) &dummy_param[0].alias - (PFSV1_SWORD) &dummy_param[0],
    (PFSV1_SWORD) &dummy_param[0].type - (PFSV1_SWORD) &dummy_param[0],
    (PFSV1_SWORD) &dummy_param[0].addr - (PFSV1_SWORD) &dummy_param[0],
    (PFSV1_SWORD) &dummy_param[0].size - (PFSV1_SWORD) &dummy_param[0],
    (PFSV1_SWORD) &dummy_param[0].perm - (PFSV1_SWORD) &dummy_param[0],
    (PFSV1_SWORD) &dummy_param[0].brief - (PFSV1_SWORD) &dummy_param[0],

    sizeof(dummy_help_utf8[0]),
    (PFSV1_SWORD) &dummy_help_utf8[0].name - (PFSV1_SWORD) &dummy_help_utf8[0],
    (PFSV1_SWORD) &dummy_help_utf8[0].text - (PFSV1_SWORD) &dummy_help_utf8[0]
};
