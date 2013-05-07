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


/** @def ENTRY_IS_UNICODE
 * @ingroup cmd
 * Test if list entry is stored as a unicode string.
 * @param entry pointer to an \ref exword_dirent_t structure
 * @returns returns true if entry name is stored in UTF16 format false otherwise
 */
#define ENTRY_IS_UNICODE(entry) (((entry)->flags & 2) != 0)
/** @def ENTRY_IS_DIRECTORY
 * @ingroup cmd
 * Test if list entry is a directory.
 * @param entry pointer to an \ref exword_dirent_t structure
 * @returns returns true if entry is a directory false otherwise
 */
#define ENTRY_IS_DIRECTORY(entry) (((entry)->flags & 1) != 0)

/** @ingroup device
 * EX-word regions.
 * One of these regions must be specified when calling \ref exword_connect.
 */
enum exword_region {
	/** Japanese Region */
	EXWORD_REGION_JA = 0x20,

	/** Korean Region */
	EXWORD_REGION_KR = 0x40,

	/** Italian Region */
	EXWORD_REGION_IT = 0x48,

	/** Chinese Region */
	EXWORD_REGION_CN = 0x60,

	/** Indian Region */
	EXWORD_REGION_IN = 0x68,

	/** German Region */
	EXWORD_REGION_DE = 0x80,

	/** Spanish Region */
	EXWORD_REGION_ES = 0xa0,

	/** French Region */
	EXWORD_REGION_FR = 0xc0,

	/** Russian Region */
	EXWORD_REGION_RU = 0xe0,
};

/** @ingroup device
 * EX-word modes.
 * One of these modes must be specified when calling \ref exword_connect.
 */
enum exword_mode {
	/** This mode is used to install and remove add-on dictionaries.*/
	EXWORD_MODE_LIBRARY = 0x0100,

	/** This mode is used to upload and delete text files. */
	EXWORD_MODE_TEXT    = 0x0200,

	/** This mode is used to upload cd audio. */
	EXWORD_MODE_CD      = 0x0400,
};

/** @ingroup cmd
 * EX-word capabilities.
 * Capability bitmasks returned by \ref exword_get_model.
 */
enum exword_capability {
	/** SW capability */
	CAP_SW  = (1 << 0),

	/** P capability */
	CAP_P   = (1 << 1),

	/** F capability */
	CAP_F   = (1 << 2),

	/** C capability */
	CAP_C   = (1 << 3),

	/** C2 capability */
	CAP_C2  = (1 << 4),

	/** ST capability */
	CAP_ST	= (1 << 5),

	/** T capability */
	CAP_T	= (1 << 6),

	/** C3 capability */
	CAP_C3	= (1 << 7),

	/** Device contains extended model information */
	CAP_EXT = (1 << 15),
};

/** @ingroup misc
 * Error codes.
 * Most libexword functions return 0 on success or one of these error codes.
 */
enum exword_error {
	/** Success (no error) */
	EXWORD_SUCCESS = 0,

	/** Access denied */
	EXWORD_ERROR_FORBIDDEN,

	/** File not found */
	EXWORD_ERROR_NOT_FOUND,

	/** Internal Error */
	EXWORD_ERROR_INTERNAL,

	/** Insufficient memory */
	EXWORD_ERROR_NO_MEM,

	/** Other error */
	EXWORD_ERROR_OTHER,
};

/** @ingroup device
 * Disconnect codes.
 * These codes sent to the application by the disconnect notification handler.
 */
enum exword_disconnect {
	/** Normal disconnect */
	EXWORD_DISCONNECT_NORMAL = 1,

	/** Disconnect due to cable being unplugged */
	EXWORD_DISCONNECT_UNPLUGGED = 2,

	/** Disconnect due to internal server error */
	EXWORD_DISCONNECT_ERROR = 4,
};


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
	uint64_t total;
	/** space available (in bytes) */
	uint64_t free;
} exword_capacity_t;
#pragma pack()

/**
 * Structure representing the device model.
 */
#pragma pack(1)
typedef struct {
	/** Main model string */
	char model[14];
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
	unsigned char key[16];
	/** Generated XOR Encryption key */
	unsigned char xorkey[16];
} exword_cryptkey_t;
#pragma pack()

/** @ingroup misc
 * File transfer callback function.
 * @param filename name of file currently being transferred
 * @param transferred number of bytes transferred so far
 * @param length total length of file
 * @param user_data data pointer specified in \ref exword_register_xfer_callbacks
 * @see exword_register_xfer_callbacks
 */
typedef void (*file_cb)(char *filename, uint32_t transferred, uint32_t length, void *user_data);

/** @ingroup device
 * Disconnect notification function.
 * @param reason reason for disconnection
 * @param user_data data pointer specified in \ref exword_register_disconnect_callback
 * @see exword_register_disconnect_callback
 */
typedef void (*disconnect_cb)(int reason, void *user_data);

#ifdef __cplusplus
extern "C" {
#endif

char * convert_to_locale(char *fmt, char **dst, int *dstsz, const char *src, int srcsz);
char * convert_from_locale(char *fmt, char **dst, int *dstsz, const char *src, int srcsz);

void get_xor_key(char *key, long size, char *xorkey);
void crypt_data(char *data, int size, char *key);

char * exword_error_to_string(int code);

exword_t * exword_init();
void exword_deinit(exword_t *self);
int exword_is_connected(exword_t *self);
void exword_set_debug(exword_t *self, int level);
int exword_get_debug(exword_t *self);
void exword_register_xfer_callbacks(exword_t *self, file_cb get, void *get_data, file_cb put, void *put_data);
void exword_register_xfer_get_callback(exword_t *self, file_cb callback, void *userdata);
void exword_register_xfer_put_callback(exword_t *self, file_cb callback, void *userdata);
void exword_register_disconnect_callback(exword_t *self, disconnect_cb disconnect, void *userdata);
void exword_poll_disconnect(exword_t *self);

int exword_connect(exword_t *self, uint16_t options);
int exword_disconnect(exword_t *self);
int exword_send_file(exword_t *self, char* filename, char *buffer, int len);
int exword_get_file(exword_t *self, char* filename, char **buffer, int *len);
int exword_remove_file(exword_t *self, char* filename, int convert_to_unicode);
int exword_get_model(exword_t *self, exword_model_t * model);
int exword_get_capacity(exword_t *self, exword_capacity_t *cap);
int exword_sd_format(exword_t *self);
int exword_setpath(exword_t *self, uint8_t *path, uint8_t mkdir);
int exword_list(exword_t *self, exword_dirent_t **entries, uint16_t *count);
void exword_free_list(exword_dirent_t *entries);
int exword_userid(exword_t *self, exword_userid_t id);
int exword_cryptkey(exword_t *self, exword_cryptkey_t *key);
int exword_cname(exword_t *self, char *name, char* dir);
int exword_unlock(exword_t *self);
int exword_lock(exword_t *self);
int exword_authchallenge(exword_t *self, exword_authchallenge_t challenge);
int exword_authinfo(exword_t *self, exword_authinfo_t *info);

#ifdef __cplusplus
}
#endif

#endif
