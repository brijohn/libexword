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


typedef struct _exword exword_t;

typedef struct {
	uint16_t vid;
	uint16_t pid;
	char manufacturer[20];
	char product[20];
} exword_device_t;

#define SD_CARD		"\\_SD_00"
#define INTERNAL_MEM	"\\_INTERNAL_00"
#define ROOT		""

#pragma pack(1)
typedef struct {
	uint16_t size;   //size of structure
	uint8_t  flags;  //flags 0 = file 1 = directory
	uint8_t  name[13]; //name of entry 8.3 file names
} directory_entry_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	uint32_t total;
	uint32_t used;
} exword_capacity_t;
#pragma pack()

int exword_scan_devices(exword_device_t **devices);
void exword_free_devices(exword_device_t *devices);
void exword_set_debug(exword_t *self, int level);
exword_t * exword_open(exword_device_t *device);
exword_t * exword_open_by_vid_pid(uint16_t vid, uint16_t pid);
void exword_close(exword_t *self);
int exword_connect(exword_t *self);
int exword_send_file(exword_t *self, char* filename, char *buffer, int len);
int exword_remove_file(exword_t *self, char* filename);
int exword_get_model(exword_t *self, uint8_t * model, uint16_t *count);
int exword_get_capacity(exword_t *self, exword_capacity_t *cap);
int exword_setpath(exword_t *self, uint8_t *path);
int exword_list(exword_t *self, directory_entry_t **entries, uint16_t *count);
int exword_disconnect(exword_t *self);
#endif
