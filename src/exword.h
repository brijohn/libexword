/* libexword - library for transffering files to Casio-EX-Word dictionaries
 *
 * Copyright (C) 2010 - Brian Johnson <brijohn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 */
#ifndef EXWORD_H
#define EXWORD_H

#include <stdint.h>

typedef struct exword_t exword_t;

#define SD_CARD		"\\_SD_00"
#define INTERNAL_MEM	"\\_INTERNAL_00"
#define ROOT		""

#define LIST_F_DIR     1
#define LIST_F_UNICODE 2

/** @ingroup device
 * Japanese Region
 */
#define LOCALE_JA      0x20
/** @ingroup device
 * Korean Region
 */
#define LOCALE_KR      0x40
/** @ingroup device
 * Chinese Region
 */
#define LOCALE_CN      0x60
/** @ingroup device
 * German Region
 */
#define LOCALE_DE      0x80
/** @ingroup device
 * Spanish Region
 */
#define LOCALE_ES      0xa0
/** @ingroup device
 * French Region
 */
#define LOCALE_FR      0xc0
/** @ingroup device
 * Russian Region
 */
#define LOCALE_RU      0xe0

/** @ingroup device
 *  Opens device in library mode.
 *  This mode is used to install and remove add-on dictionaries.
 */
#define OPEN_LIBRARY   0x0000
/** @ingroup device
 *  Opens device in text mode.
 *  This mode is used to upload and delete text files.
 */
#define OPEN_TEXT      0x0100
/** @ingroup device
 *  Opens device in CD mode.
 *  This mode is used to upload cd audio.
 */
#define OPEN_CD        0x0200

/** @ingroup cmd
 * SW capability
 */
#define CAP_SW         (1 << 0)
/** @ingroup cmd
 * P capability
 */
#define CAP_P          (1 << 1)
/** @ingroup cmd
 * F capability
 */
#define CAP_F          (1 << 2)
/** @ingroup cmd
 * C capability
 */
#define CAP_C          (1 << 3)
/** @ingroup cmd
 * Device contains extended model information
 */
#define CAP_EXT        (1 << 15)

/**
 * Structure representing a directory entry.
 */
#pragma pack(1)
typedef struct {
	/** Size of structure */
	uint16_t size;
	/** Flags.
	 * Field representing type of entry.
	 * - 0 = file
	 * - 1 = directory
	 * - 2 = unicode name
	 */
	uint8_t  flags;
	/** Entry name */
	uint8_t  *name;
} exword_dirent_t;
#pragma pack()

/**
 * Structure representing a device's storage capacity.
 */
#pragma pack(1)
typedef struct {
	/** total space (in bytes) */
	uint32_t total;
	/** space available (in bytes) */
	uint32_t free;
} exword_capacity_t;
#pragma pack()

/**
 * Structure representing the device model.
 */
#pragma pack(1)
typedef struct {
	/** Main model string */
	char model[15];
	/** sub model string */
	char sub_model[6];
	/** extended model string (DP5+) */
	char ext_model[6];
	/** Capability bit mask */
	short capabilities;
} exword_model_t;
#pragma pack()

/**
 * Structure representing user name.
 */
#pragma pack(1)
typedef struct {
	/** User name */
	char name[17];
} exword_userid_t;
#pragma pack()

/**
 * Structure representing an authentication challenge.
 */
#pragma pack(1)
typedef struct {
	/** Challenge key */
	char challenge[20];
} exword_authchallenge_t;
#pragma pack()

/**
 * Structure representing authentication info.
 */
#pragma pack(1)
typedef struct {
	/** Input block 1 */
	unsigned char blk1[16];
	/** Input block 2 */
	unsigned char blk2[24];
	/** Generated challenge key */
	char challenge[20];
} exword_authinfo_t;
#pragma pack()

/**
 * Structure representing a CryptKey
 */
#pragma pack(1)
typedef struct {
	/** Input block 1 */
	unsigned char blk1[16];
	/** Input block 2 */
	unsigned char blk2[12];
	/** Generated CryptKey */
	unsigned char key[12];
} exword_cryptkey_t;
#pragma pack()

/** @ingroup misc
 * File transfer callback function,
 * @param filename name of file currently being transferred
 * @param transferred number of bytes transferred so far
 * @param length total length of file
 * @param user_data data pointer specified in \ref exword_register_callbacks
 * @see exword_register_callbacks
 */
typedef void (*file_cb)(char *filename, uint32_t transferred, uint32_t length, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif

char * convert_to_locale(char *fmt, char **dst, int *dstsz, const char *src, int srcsz);
char * utf16_to_locale(char **dst, int *dstsz, const char *src, int srcsz);
char * locale_to_utf16(char **dst, int *dstsz, const char *src, int srcsz);
char * exword_response_to_string(int rsp);
void exword_set_debug(exword_t *self, int level);
void exword_register_callbacks(exword_t *self, file_cb get, file_cb put, void *userdata);
void exword_free_list(exword_dirent_t *entries);
exword_t * exword_open();
exword_t * exword_open2(uint16_t options);
void exword_close(exword_t *self);
int exword_connect(exword_t *self);
int exword_send_file(exword_t *self, char* filename, char *buffer, int len);
int exword_get_file(exword_t *self, char* filename, char **buffer, int *len);
int exword_remove_file(exword_t *self, char* filename, int convert_to_unicode);
int exword_get_model(exword_t *self, exword_model_t * model);
int exword_get_capacity(exword_t *self, exword_capacity_t *cap);
int exword_sd_format(exword_t *self);
int exword_setpath(exword_t *self, uint8_t *path, uint8_t mkdir);
int exword_list(exword_t *self, exword_dirent_t **entries, uint16_t *count);
int exword_userid(exword_t *self, exword_userid_t id);
int exword_cryptkey(exword_t *self, exword_cryptkey_t *key);
int exword_cname(exword_t *self, char *name, char* dir);
int exword_unlock(exword_t *self);
int exword_lock(exword_t *self);
int exword_authchallenge(exword_t *self, exword_authchallenge_t challenge);
int exword_authinfo(exword_t *self, exword_authinfo_t *info);
int exword_disconnect(exword_t *self);

#ifdef __cplusplus
}
#endif

#endif
