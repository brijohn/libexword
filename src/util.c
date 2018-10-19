/* util.c - several utility functions used in exword
 *
 * Copyright (C) 2010-2018 - Brian Johnson <brijohn@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#include "exword.h"
#include "util.h"

#if defined(__MINGW32__)
# include <shlwapi.h>
# include <shlobj.h>
#elif defined(__APPLE__) && defined(__MACH__)
# include <Carbon/Carbon.h>
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

char valid_sfn_chars[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
	0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01,
};

void * xmalloc (size_t n)
{
	void *p = malloc(n);
	if (!p && n != 0) {
		fprintf(stderr, "abort: Out of Memory\n");
		abort();
	}
	memset(p, 0, n);
	return p;
}

void * xrealloc (void *ptr, size_t n)
{
	void *p = realloc(ptr, n);
	if (!p && n != 0) {
		fprintf(stderr, "abort: Out of Memory\n");
		abort();
	}
	return p;
}

void dev_list_clear(struct list_head *head)
{
	struct device_map * _dev;
	while (!list_empty(head)) {
		_dev = list_entry(head->next, struct device_map, list);
		list_del(&(_dev->list));
		free(_dev->dev);
		free(_dev->root);
		free(_dev);
	}
}

void dev_list_scan(struct list_head *head, exword_dirent_t *entries)
{
	int i;
	int len;
	for (i = 0; entries[i].name != NULL; ++i) {
		fflush(stdout);
		struct device_map * _dev = xmalloc(sizeof(struct device_map));
		memset(_dev, 0, sizeof(struct device_map));
		len = strlen(entries[i].name);
		_dev->root = xmalloc(len + 2);
		_dev->dev = xmalloc(5);
		_dev->root[0] = '\\';
		strcpy(_dev->root + 1, entries[i].name);
		if (entries[i].name[1] == 'S')
			strcpy(_dev->dev, "crd");
		if (entries[i].name[1] == 'I')
			strcpy(_dev->dev, "drv");
		_dev->dev[3] = entries[i].name[len - 1];
		_dev->dev[4] = 0;
		list_add_tail(&(_dev->list), head);
	}
}

struct device_map * dev_list_search(struct list_head *head, char * dev)
{
	struct device_map *tmp;
	list_for_each_entry(tmp, head, list)
		if (strcmp(tmp->dev, dev) == 0)
			return tmp;
	return NULL;
}

const char * get_data_dir()
{
	static char data_dir[PATH_MAX];
#if defined(__MINGW32__)
	if(SUCCEEDED(SHGetFolderPath(NULL,
		     CSIDL_APPDATA|CSIDL_FLAG_CREATE,
		     NULL,
		     0,
		     data_dir))) {
		PathAppend(data_dir, "exword");
	} else {
		return NULL;
	}
#elif defined(__APPLE__) && defined(__MACH__)
	int length;
	FSRef foundRef;
	OSStatus validPath;
	OSErr err = FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &foundRef);
	if (err != noErr)
		return NULL;
	validPath = FSRefMakePath(&foundRef, data_dir, PATH_MAX);
        if (validPath != noErr)
		return NULL;
	length = strlen(data_dir) + strlen("/exword");
	if (length >= PATH_MAX)
		return NULL;
	strcat(data_dir, "/exword");
#else
	int length = 0;
	const char * xdg_data_home;
	xdg_data_home = getenv("XDG_DATA_HOME");
	if (xdg_data_home == NULL) {
		xdg_data_home = getenv("HOME");
		if (xdg_data_home == NULL)
			return NULL;
		length = strlen(xdg_data_home);
		if (length >= PATH_MAX)
			return NULL;
		strcpy(data_dir, xdg_data_home);
		length += strlen("/.local/share/exword");
		if (length >= PATH_MAX)
			return NULL;
		strcat(data_dir, "/.local/share/exword");
	} else {
		length = strlen(xdg_data_home);
		if (length >= PATH_MAX)
			return NULL;
		strcpy(data_dir, xdg_data_home);
		length += strlen("/exword");
		if (length >= PATH_MAX)
			return NULL;
		strcat(data_dir, "/exword");
	}
#endif
	return data_dir;
}

char * region_id2locale(int id)
{
	switch(id) {
	case EXWORD_REGION_JA:
		return "SHIFT_JIS";
	case EXWORD_REGION_CN:
		return "GB2312";
	case EXWORD_REGION_KR:
		return "EUC-KR";
	default:
		return "UTF-8";
	}
}

char * region_id2str(int id)
{
	switch(id) {
	case EXWORD_REGION_JA:
		return "ja";
	case EXWORD_REGION_CN:
		return "cn";
	case EXWORD_REGION_IN:
		return "in";
	case EXWORD_REGION_KR:
		return "kr";
	case EXWORD_REGION_IT:
		return "it";
	case EXWORD_REGION_DE:
		return "de";
	case EXWORD_REGION_ES:
		return "es";
	case EXWORD_REGION_FR:
		return "fr";
	case EXWORD_REGION_RU:
		return "ru";
	default:
		return NULL;
	}
}

char * mode_id2str(int id)
{
	switch(id) {
	case EXWORD_MODE_TEXT:
		return "text";
	case EXWORD_MODE_CD:
		return "cd";
	case EXWORD_MODE_LIBRARY:
		return "library";
	default:
		return NULL;
	}
}

int is_valid_sfn(char * filename)
{
	int i;
	size_t base_len = 0;
	size_t ext_len = 0;
	unsigned char *fname = strdup(filename);
	unsigned char *ext = strrchr(fname, '.');
	int valid = 0;
	if (ext != NULL) {
		*ext++ = '\0';
		ext_len = strlen(ext);
	}
	base_len = strlen(fname);
	if (base_len > 8 || base_len == 0 || ext_len > 3)
		goto done;
	for (i = 0; i < base_len; ++i) {
		if (valid_sfn_chars[fname[i]] == 0)
			goto done;
	}
	for (i = 0; i < ext_len; ++i) {
		if (valid_sfn_chars[ext[i]] == 0)
			goto done;
	}
	valid = 1;
done:
	free(fname);
	return valid;
}

char * mkpath(const char* separator, const char *base, ...)
{
	char *path, *part;
	unsigned int current_size = strlen(base) + 1;
	unsigned int new_size;
	path = xmalloc(strlen(base) + 1);
	strcpy(path, base);
	va_list list;
	va_start(list, base);
	part = va_arg(list, char*);
	while (part != NULL)  {
		new_size = strlen(part) + strlen(path) + 2;
		if (new_size > current_size) {
			path = xrealloc(path, new_size);
		}
		strcat(path, separator);
		strcat(path, part);
		part = va_arg(list, char*);
	}
	va_end(list);
	return path;
}

int read_file(const char* filename, char **buffer, int *len)
{
	int fd, err;
	struct stat buf;
	*buffer = NULL;
	*len = 0;
	fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0)
		return -1;
	fstat(fd, &buf);
	*buffer = xmalloc(buf.st_size);
	*len = read(fd, *buffer, buf.st_size);
	if (*len < 0) {
		err = errno;
		free(*buffer);
		*buffer = NULL;
		*len = 0;
		close(fd);
		errno = err;
		return -1;
	}
	close(fd);
	return 0;
}

int write_file(const char* filename, char *buffer, int len)
{
	int fd, ret, err;
	struct stat buf;
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return -1;
	ret = write(fd, buffer, len);
	if (ret < 0) {
		err = errno;
		close(fd);
		errno = err;
		return -1;
	}
	close(fd);
	return 0;
}
