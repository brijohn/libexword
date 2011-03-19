/* util.h - several utility functions used in exword
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

#ifndef _UTIL_H
#define _UTIL_H

#if defined(__MINGW32__)
# define mkdir(path, mode) _mkdir(path)
#endif

void * xmalloc(size_t n);
int write_file(const char* filename, char *buffer, int len);
int read_file(const char* filename, char **buffer, int *len);
const char * get_data_dir();
char * mkpath(const char *id, const char *filename);

#endif
