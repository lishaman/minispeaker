/*
 *  Copyright (C) 2013 Fighter Sun <wanmyqawdr@126.com>
 *  helpers for implement of parameterized firmware specification v1
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
 *
 *  // Example begin
 *  struct child
 *  {
 *      int b;
 *      int c;
 *  };
 *
 *  struct parent
 *  {
 *      char name[10];
 *      int a;
 *      struct child child_var;
 *  };
 *
 *  // Must not be allocated at .bss section
 *  struct parent parent_var;
 *
 *  // Define parameter descriptor, will be at "pfsv1.param.desc" section
 *  PFSV1_DEF_PARAM_BEGIN(example)
 *      PFSV1_DEF_PARAM_BLOCK_BEGIN(parent_var, "parent", struct parent,
 *                                      "Parent variable block begin")
 *          PFSV1_DEF_PARAM(parent_var.name, "name", char [10], "ro",
 *                              "text: Parent variable.name, read only\n"
 *                              "value: ["string0", "string1"]")
 *          PFSV1_DEF_PARAM(parent_var.a, "a", int, "rw",
 *                              "text: Parent variable.a, read write\n"
 *                              "value: [1, 2, 3]")
 *
 *          PFSV1_DEF_PARAM_BLOCK_BEGIN(parent_var.child_var, "child", struct child,
 *                                          "Child variable block begin")
 *              PFSV1_DEF_PARAM(parent_var.child_var.b, "b", int, "rw",
 *                                              "text: Child variable.b\n"
 *                                              "value: [1:2:10]")
 *              PFSV1_DEF_PARAM(parent_var.child_var.c, "c", int, "rw",
 *                                              "text: Child variable.c\n"
 *                                              "value: [1:10], [12, 17]")
 *          PFSV1_DEF_PARAM_BLOCK_END(parent_var.child_var)
 *      PFSV1_DEF_PARAM_BLOCK_END(parent_var)
 *  PFSV1_DEF_PARAM_END(example)
 *
 *  // Define parameter detail help text in English
 *  // will be at "pfsv1.help.en_us.utf8" section
 *  PFSV1_DEF_HELP_EN_BEGIN(example)
 *      PFSV1_DEF_HELP_BLOCK_BEGIN(parent_var, "help text for this")
 *          PFSV1_DEF_HELP(parent_var.a, "help text for this")
 *          PFSV1_DEF_HELP(parent_var.name, "help text for this")
 *
 *          PFSV1_DEF_HELP_BLOCK_BEGIN(parent_var.child_var,
 *                                                  "help text for this")
 *              PFSV1_DEF_HELP(parent_var.child_var.b, "help text for this")
 *              PFSV1_DEF_HELP(parent_var.child_var.c, "help text for this")
 *          PFSV1_DEF_HELP_BLOCK_END(parent_var.child_var)
 *      PFSV1_DEF_HELP_BLOCK_END(parent_var)
 *  PFSV1_DEF_HELP_END(example)
 *
 *  // Define parameter detail help text in Chinese,
 *  // will be at "pfsv1.help.zh_cn.utf8" section
 *  PFSV1_DEF_HELP_ZH_BEGIN(example)
 *      PFSV1_DEF_HELP_BLOCK_BEGIN(parent_var, "填写详细帮助信息")
 *          PFSV1_DEF_HELP(parent_var.a, "填写详细帮助信息")
 *          PFSV1_DEF_HELP(parent_var.name, "填写详细帮助信息")
 *
 *          PFSV1_DEF_HELP_BLOCK_BEGIN(parent_var.child_var, "填写详细帮助信息")
 *              PFSV1_DEF_HELP(parent_var.child_var.b, "填写详细帮助信息")
 *              PFSV1_DEF_HELP(parent_var.child_var.c, "填写详细帮助信息")
 *          PFSV1_DEF_HELP_BLOCK_END(parent_var.child_var)
 *      PFSV1_DEF_HELP_BLOCK_END(parent_var
 *  PFSV1_DEF_HELP_END(example)
 *  // Example end
 */

#ifndef PFSV1_H
#define PFSV1_H

#ifdef CONFIG_PFSV1

#ifndef __GNUC__
#error "Please use GNU C complier."
#endif

typedef long            PFSV1_WORD;
typedef unsigned long   PFSV1_SWORD;
typedef PFSV1_SWORD     PFSV1_ADDR;
typedef char            PFSV1_BYTE;

#define PFSV1_PARAM_TYPE_LENGTH_MAX                 256
#define PFSV1_PARAM_NAME_LENGTH_MAX                 512
#define PFSV1_PARAM_ALIAS_LEGNTH_MAX                64
#define PFSV1_PARAM_NAME_BRIEF_LENGTH_MAX           2048

#define PFSV1_PARAM_HELP_TEXT_LENGTH_MAX            1024 * 8

#define PFSV1_PERM_LENGTH_MAX                       3

#define PFSV1_SECT_DATATYPE_OFFSET       "pfsv1.datatype.offset"
#define PFSV1_SECT_PARAM_DESC            "pfsv1.param.desc"
#define PFSV1_SECT_NAME_HELP_ZH_CN       "pfsv1.help.zh_cn.utf8"
#define PFSV1_SECT_NAME_HELP_EN_US       "pfsv1.help.en_us.utf8"

struct pfsv1_param_desc
{
    PFSV1_BYTE name[PFSV1_PARAM_NAME_LENGTH_MAX];
    PFSV1_BYTE alias[PFSV1_PARAM_ALIAS_LEGNTH_MAX];
    PFSV1_BYTE type[PFSV1_PARAM_TYPE_LENGTH_MAX];
    PFSV1_ADDR addr;
    PFSV1_SWORD size;
    PFSV1_BYTE perm[PFSV1_PERM_LENGTH_MAX];
    PFSV1_BYTE brief[PFSV1_PARAM_NAME_BRIEF_LENGTH_MAX];
};

struct pfsv1_help_utf8_desc
{
    PFSV1_BYTE name[PFSV1_PARAM_NAME_LENGTH_MAX];
    PFSV1_BYTE text[PFSV1_PARAM_HELP_TEXT_LENGTH_MAX];
};

#define PFSV1_DEF_PARAM_BEGIN(_name_, _alias_, _brief_)                     \
    __attribute__((__used__))                                               \
    __attribute__ ((__aligned__(sizeof (long))))                            \
    __attribute__ ((__section__(PFSV1_SECT_PARAM_DESC)))                    \
        static struct pfsv1_param_desc                                      \
                        pfsv1_param_desc_##_name_[] = {                     \
                            {                                               \
                                .name = { "<"#_name_">" },                  \
                                .alias = { _alias_ },                       \
                                .type = { "" },                             \
                                .addr = 1,                                  \
                                .size = 1,                                  \
                                .perm = { "" },                             \
                                .brief = { _brief_ }                        \
                            },

#define PFSV1_DEF_PARAM_BLOCK_BEGIN(_name_, _alias_, _type_, _brief_)       \
            {                                                               \
                .name = { "<"#_name_">" },                                  \
                .alias = { _alias_ },                                       \
                .type = { "" },                                             \
                .addr = sizeof(_type_),                                     \
                .size = sizeof(_name_),                                     \
                .perm = { "" },                                             \
                .brief = { _brief_ }                                        \
            },

#define PFSV1_DEF_PARAM_BLOCK_END(_name_)                                   \
            {                                                               \
                .name = { "</"#_name_">" },                                 \
                .type = { "" },                                             \
                .addr = 0,                                                  \
                .size = 0,                                                  \
                .perm = { "" },                                             \
                .brief = { "" }                                             \
            },

#define PFSV1_DEF_PARAM(_name_, _alias_, _type_, _perm_, _brief_)           \
            {                                                               \
                .name = { #_name_ },                                        \
                .alias = { _alias_ },                                       \
                .type = { #_type_ },                                        \
                .addr = (PFSV1_ADDR)(&(_name_)),                            \
                .size = sizeof(_name_),                                     \
                .perm = { _perm_ },                                         \
                .brief = { _brief_ }                                        \
            },

#define PFSV1_DEF_PARAM_END(_name_)                                         \
            {                                                               \
                .name = { "</"#_name_">" },                                 \
                .type = { "" },                                             \
                .addr = 0,                                                  \
                .size = 0,                                                  \
                .perm = { "" },                                             \
                .brief = { "" }                                             \
            },                                                              \
        };

#define PFSV1_DEF_HELP_BEGIN(_name_, _sect_)                                \
    __attribute__((__used__))                                               \
    __attribute__ ((__aligned__(sizeof (long))))                            \
    __attribute__ ((__section__(_sect_)))                                   \
        static struct pfsv1_help_utf8_desc                                  \
                        pfsv1_help_utf8_desc_##_name_[] = {

#define PFSV1_DEF_HELP_EN_BEGIN(_name_)                                     \
    PFSV1_DEF_HELP_BEGIN(_name_##_en_US, PFSV1_SECT_NAME_HELP_EN_US)

#define PFSV1_DEF_HELP_ZH_BEGIN(_name_)                                     \
    PFSV1_DEF_HELP_BEGIN(_name_##_zh_CN, PFSV1_SECT_NAME_HELP_ZH_CN)

#define PFSV1_DEF_HELP(_name_, _text_)                                      \
            {                                                               \
                .name = { #_name_ },                                        \
                .text = { _text_ }                                          \
            },

#define PFSV1_DEF_HELP_BLOCK_BEGIN(_name_, _text_)                          \
    PFSV1_DEF_HELP(_name_, _text_)

#define PFSV1_DEF_HELP_BLOCK_END(_name_)

#define PFSV1_DEF_HELP_END(_name_)                                          \
        };

#else

#define PFSV1_DEF_PARAM_BEGIN(_name_, _alias_, _text_)
#define PFSV1_DEF_PARAM_BLOCK_BEGIN(_name_, _alias_, _type_, _brief_)
#define PFSV1_DEF_PARAM_BLOCK_END(_name_)
#define PFSV1_DEF_PARAM(_name_, _alias_, _type_, _perm_, _brief_)
#define PFSV1_DEF_PARAM_END(_name_)

#define PFSV1_DEF_HELP_BEGIN(_name_, _sect_)
#define PFSV1_DEF_HELP_EN_BEGIN(_name_)
#define PFSV1_DEF_HELP_ZH_BEGIN(_name_)
#define PFSV1_DEF_HELP_BLOCK_BEGIN(_name_, _text_)
#define PFSV1_DEF_HELP_BLOCK_END(_name_)
#define PFSV1_DEF_HELP(_name_, _text_)
#define PFSV1_DEF_HELP_END(_name_)

#endif

#endif
