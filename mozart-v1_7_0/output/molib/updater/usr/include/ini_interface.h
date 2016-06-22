/**
 * @file ini_interface.h
 * @brief ini file parser interface
 * @author ylguo <yonglei.guo@ingenic.com>
 * @version 1.0
 * @date 2015-03-23
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

#ifndef __INI_INTERACE_H__
#define __INI_INTERACE_H__

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @brief OK
 */
#define CFG_OK 0
/**
 * @brief section not found.
 */
#define CFG_SECTION_NOT_FOUND -1
/**
 * @brief key not found.
 */
#define CFG_KEY_NOT_FOUND -2
/**
 * @brief cfg error.
 */
#define CFG_ERR -10
/**
 * @brief cfg file error
 */
#define CFG_ERR_FILE -10
/**
 * @brief open file error.
 */
#define CFG_ERR_OPEN_FILE -10
/**
 * @brief create file error.
 */
#define CFG_ERR_CREATE_FILE -11
/**
 * @brief read file error.
 */
#define CFG_ERR_READ_FILE -12
/**
 * @brief section not found.
 */
#define CFG_ERR_WRITE_FILE -13
/**
 * @brief file format error.
 */
#define CFG_ERR_FILE_FORMAT -14
/**
 * @brief system error.
 */
#define CFG_ERR_SYSTEM -20
/**
 * @brief system call invoke error.
 */
#define CFG_ERR_SYSTEM_CALL -20
/**
 * @brief internal error
 */
#define CFG_ERR_INTERNAL -21
/**
 * @brief execed buf size error.
 */
#define CFG_ERR_EXCEED_BUF_SIZE -22


/**
 * @brief section not found.
 */
#define COPYF_OK 0
/**
 * @brief open file error.
 */
#define COPYF_ERR_OPEN_FILE -10
/**
 * @brief create file error.
 */
#define COPYF_ERR_CREATE_FILE -11
/**
 * @brief readl file error.
 */
#define COPYF_ERR_READ_FILE -12
/**
 * @brief write file error.
 */
#define COPYF_ERR_WRITE_FILE -13


/**
 * @brief ok.
 */
#define TXTF_OK 0
/**
 * @brief open file error.
 */
#define TXTF_ERR_OPEN_FILE -1
/**
 * @brief read file error.
 */
#define TXTF_ERR_READ_FILE -2
/**
 * @brief write file error.
 */
#define TXTF_ERR_WRITE_FILE -3
/**
 * @brief delete file error.
 */
#define TXTF_ERR_DELETE_FILE -4
/**
 * @brief not found.
 */
#define TXTF_ERR_NOT_FOUND -5

/**
 * @brief get all sections in ini_file
 *
 * @param ini_file [in] ini file path
 * @param sections [out] sections pointer array, need allocate memory.
 *
 * @return return 0 On success, else error.
 */
extern int mozart_ini_getsections(char *ini_file, char **sections);

/**
 * @brief similar to mozart_ini_getsections(...), but alloc memory internal for sections.
 *
 * @param ini_file [in] ini file path
 * @param sections [out] all sections cnt.
 *
 * @return all sections, need free after used done.
 */
extern char **mozart_ini_getsections1(char *ini_file, int *n_sections);

/**
 * @brief get all keys in sections in ini_file
 *
 * @param ini_file [in] ini file path
 * @param section [in] section name
 * @param keys [out] keys pointer array, need allocate memory.
 *
 * @return return 0 On success, else error.
 */
extern int mozart_ini_getkeys(char *ini_file, char *section, char *keys[]);

/**
 * @brief get key's value in section in ini_file
 *
 * @param ini_file [in] ini file path
 * @param section [in] section name
 * @param key [in] key name
 * @param value [out] value name of key
 *
 * @return return 0 On success, else error.
 */
extern int mozart_ini_getkey(char *ini_file, char *section, char *key, char *value);

/**
 * @brief set key's value in section in ini_file
 *
 * @param ini_file [in] ini file path
 * @param section [in] section name
 * @param key [in] key name
 * @param value [in] value name of key
 *
 * @return return 0 On success, else error.
 */
extern int mozart_ini_setkey(char *ini_file, char *section, char *key, char *value);

#ifdef  __cplusplus
}
#endif

#endif //__INI_INTERACE_H__
