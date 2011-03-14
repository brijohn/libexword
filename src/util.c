/* util.c - several utility functions used in exword
 *
 * Copyright (C) 2010, 2011 - Brian Johnson <brijohn@gmail.com>
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
#include <fcntl.h>
#include <limits.h>

#include "util.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

void * xmalloc (size_t n)
{
	void *p = malloc(n);
	if (!p && n != 0) {
		fprintf(stderr, "abort: Out of Memory\n");
		abort();
	}
	return p;
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

char * mkpath(const char *id, const char *filename)
{
	char *path = xmalloc(strlen(id) + strlen(filename) + 2);
	strcpy(path, id);
	strcat(path, PATH_SEP);
	strcat(path, filename);
	return path;
}

int read_file(const char* filename, char **buffer, int *len)
{
	int fd;
	struct stat buf;
	*buffer = NULL;
	*len = 0;
	fd = open(filename, O_RDONLY | O_BINARY);
	if (fd < 0)
		return 0x44;
	fstat(fd, &buf);
	*buffer = xmalloc(buf.st_size);
	*len = read(fd, *buffer, buf.st_size);
	if (*len < 0) {
		free(*buffer);
		*buffer = NULL;
		*len = 0;
		close(fd);
		return 0x50;
	}
	close(fd);
	return 0x20;
}

int write_file(const char* filename, char *buffer, int len)
{
	int fd, ret;
	struct stat buf;
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return 0x43;
	ret = write(fd, buffer, len);
	if (ret < 0) {
		close(fd);
		return 0x50;
	}
	close(fd);
	return 0x20;
}
