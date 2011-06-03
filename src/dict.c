/* dict.c - code for removing/installing and dumping dictionaries
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exword.h"
#include "util.h"

typedef struct {
	char id[32];
	char key[16];
	char name[132];
} admini_t;

static char key1[16] =
	"\x42\x72\xb7\xb5\x9e\x30\x83\x45\xc3\xb5\x41\x53\x71\xc4\x95\x00";

char *admini_list[] = {
	"admini.inf",
	"adminikr.inf",
	"adminicn.inf",
	"adminide.inf",
	"adminies.inf",
	"adminifr.inf",
	"adminiru.inf",
	NULL
};

int _read_admini(exword_t *device, char **buffer, int *length)
{
	int i, rsp;
	for (i = 0; admini_list[i] != NULL; i++) {
		rsp = exword_get_file(device, admini_list[i], buffer, length);
		if (rsp == EXWORD_SUCCESS && *length > 0)
			break;
		free(*buffer);
		*buffer = NULL;
	}
	return (admini_list[i] == NULL ? -1 : i);
}

int _find(exword_t *device, char *root, char *id, admini_t *ini)
{
	char * buffer;
	int len, i, rsp;

	exword_setpath(device, root, 0);
	rsp = _read_admini(device, &buffer, &len);
	if (rsp < 0)
		return 0;
	for (i = 0; i < len; i += 180) {
		if (strncmp(buffer + i, id, 32) == 0)
			break;
	}
	if (i >= len) {
		free(buffer);
		return 0;
	}
	memcpy(ini, buffer + i, 180);
	free(buffer);
	return 1;
}

int _upload_file(exword_t *device, char *id, char* name, char *key)
{
	int length, rsp;
	char *ext, *buffer;
	char *filename;
	filename = mkpath(PATH_SEP, id, name, NULL);
	rsp = read_file(filename, &buffer, &length);
	if (rsp != 0) {
		free(filename);
		return 0;
	}
	ext = strrchr(filename, '.');
	if (ext != NULL && (strcmp(ext, ".txt") == 0 ||
			    strcmp(ext, ".bmp") == 0 ||
			    strcmp(ext, ".htm") == 0 ||
			    strcmp(ext, ".TXT") == 0 ||
			    strcmp(ext, ".BMP") == 0 ||
			    strcmp(ext, ".HTM") == 0)) {
		crypt_data(buffer, length, key);
	}
	rsp = exword_send_file(device, name, buffer, length);
	free(filename);
	free(buffer);
	return (rsp == EXWORD_SUCCESS);
}

int _download_file(exword_t *device, char *id, char* name, char *key)
{
	char *filename;
	int length, rsp;
	char *buffer, *ext;
	filename = mkpath(PATH_SEP, id, name, NULL);
	ext = strrchr(filename, '.');
	rsp = exword_get_file(device, name, &buffer, &length);
	if (rsp != EXWORD_SUCCESS) {
		free(filename);
		return 0;
	}
	if (ext != NULL && (strcmp(ext, ".htm") == 0 ||
			    strcmp(ext, ".bmp") == 0 ||
			    strcmp(ext, ".txt") == 0 ||
			    strcmp(ext, ".TXT") == 0 ||
			    strcmp(ext, ".BMP") == 0 ||
			    strcmp(ext, ".HTM") == 0)) {
		crypt_data(buffer, length, key);
	}
	rsp = write_file(filename, buffer, length);
	free(filename);
	free(buffer);
	return (rsp == EXWORD_SUCCESS);
}

int _get_size(char *id)
{
	DIR *dhandle;
	struct stat buf;
	struct dirent *entry;
	int size = 0;
	int fd;
	char *filename;
	dhandle = opendir(id);
	if (dhandle == NULL)
		return -1;
	while ((entry = readdir(dhandle)) != NULL) {
		filename = mkpath(PATH_SEP, id, entry->d_name, NULL);
		fd = open(filename, O_RDONLY);
		if (fd >= 0) {
			if (fstat(fd, &buf) == 0 && S_ISREG(buf.st_mode))
				size += buf.st_size;
			close(fd);
		}
		free(filename);
	}
	closedir(dhandle);
	return size;
}

char * _get_name(char *id)
{
	int length, len;
	char *name = NULL;
	char *filename;
	char *buffer, *start, *end;
	struct stat buf;
	filename = mkpath(PATH_SEP, id, "diction.htm", NULL);
	if (read_file(filename, &buffer, &length) != 0) {
		free(filename);
		return NULL;
	}
	start = strstr(buffer, "<title>");
	end = strstr(buffer, "</title>");
	len = end - (start + 7);
	if (start == NULL || end == NULL || len < 0) {
		free(filename);
		free(buffer);
		return NULL;
	}
	name = xmalloc(len + 1);
	memcpy(name, start + 7, len);
	name[len] = '\0';
	free(filename);
	free(buffer);
	return name;
}

int _save_user_key(char *name, char *key)
{
	char *buffer;
	char *file;
	int length, ret, str_len;
	int i = 0;
	const char *dir = get_data_dir();
	if (dir == NULL)
		return 0;
	mkdir(dir, 0770);
	file = mkpath(PATH_SEP, dir, "users.dat", NULL);
	ret = read_file(file, &buffer, &length);
	if (ret == -1 && errno != ENOENT) {
		free(file);
		return 0;
	}
	while (i < length) {
		if (strcmp(name, buffer + i + 1) == 0) {
			free(file);
			free(buffer);
			return 1;
		}
		i += 21 + *(buffer + i);
	}
	str_len = strlen(name) + 1;
	buffer = xrealloc(buffer, length + str_len + 21);
	buffer[length] = str_len;
	memcpy(buffer + length + 1, name, str_len);
	memcpy(buffer + length + 1 + str_len, key, 20);
	ret = write_file(file, buffer, length + str_len + 21);
	free(file);
	free(buffer);
	return (ret == 0);
}

int _load_user_key(char *name, char *key)
{
	char *buffer;
	char *file;
	int length, ret;
	int i = 0;
	const char *dir = get_data_dir();
	if (dir == NULL)
		return 0;
	mkdir(dir, 0770);
	file = mkpath(PATH_SEP, dir, "users.dat", NULL);
	ret = read_file(file, &buffer, &length);
	free(file);
	if (ret != 0)
		return 0;
	while (i < length) {
		if (strcmp(name, buffer + i + 1) == 0) {
			memcpy(key, buffer + i + 1 + *(buffer + i), 20);
			free(buffer);
			return 1;
		}
		i += 21 + *(buffer + i);
	}
	free(buffer);
	return 0;
}

int dict_decrypt(exword_t *device, char *root, char *id)
{
	int i, rsp;
	char *ext;
	char key[16];
	char *path;
	char *dir;
	struct stat buf;
	admini_t info;
	uint16_t count;
	exword_dirent_t *entries;
	path = mkpath("\\", root, id, "_CONTENT", NULL);
	if (!_find(device, root, id, &info)) {
		printf("No dictionary with id %s installed.\n", id);
		return 0;
	}
	rsp = exword_setpath(device, path, 0);
	free(path);
	if (rsp != EXWORD_SUCCESS) {
		printf("Dictionary %s does not exist\n", id);
		return 0;
	}
	dir = mkpath(PATH_SEP, get_data_dir(), id, NULL);
	if (stat(dir, &buf) == 0 && S_ISDIR(buf.st_mode)) {
		printf("Local version of dictionary already exists\n");
		free(dir);
		return 0;
	}
	if (mkdir(dir, 0770) < 0) {
		printf("Failed to create local directory %s\n", id);
		free(dir);
		return 0;
	}
	get_xor_key(info.key, 16, key);
	exword_list(device, &entries, &count);
	for (i = 0; i < count; i++) {
		if (entries[i].flags == 0) {
			ext = strrchr(entries[i].name, '.');
			if (ext != NULL && (strcmp(ext, ".cjs") == 0 ||
					    strcmp(ext, ".CJS") == 0))
				continue;
			printf("Decrypting %s...", entries[i].name);
			if (_download_file(device, dir, entries[i].name, key))
				printf("Done\n");
			else
				printf("Failed\n");
		}
	}
	free(dir);
	exword_free_list(entries);
	return 1;
}

int dict_auth(exword_t *device, char *user, char *auth)
{
	int rsp, i;
	uint16_t count;
	exword_dirent_t *entries;
	exword_authchallenge_t c;
	exword_authinfo_t ai;
	exword_userid_t u;
	if (auth == NULL) {
		rsp = _load_user_key(user, c.challenge);
		if (!rsp)
			return 0;
	} else {
		memcpy(c.challenge, auth, 20);
	}
	memcpy(ai.blk1, "FFFFFFFFFFFFFFFF", 16);
	strncpy(u.name, user, 16);
	strncpy(ai.blk2, user, 24);
	exword_setpath(device, "\\_INTERNAL_00", 0);
	rsp = exword_authchallenge(device, c);
	if (rsp != EXWORD_SUCCESS)
		return 0;
	exword_setpath(device, "", 0);
	exword_list(device, &entries, &count);
	for (i = 0; i < count; i++) {
		if (strcmp(entries[i].name, "_SD_00") == 0) {
			exword_setpath(device, "\\_SD_00", 0);
			rsp = exword_authchallenge(device, c);
			if (rsp != EXWORD_SUCCESS) {
				exword_authinfo(device, &ai);
			}
		}
	}
	exword_free_list(entries);
	exword_userid(device, u);
	return 1;
}

int dict_reset(exword_t *device, char *user)
{
	int i;
	exword_authinfo_t info;
	exword_userid_t u;
	memset(&info, 0, sizeof(exword_authinfo_t));
	memset(&u, 0, sizeof(exword_userid_t));
	memcpy(info.blk1, "FFFFFFFFFFFFFFFF", 16);
	strncpy(info.blk2, user, 24);
	strncpy(u.name, user, 16);
	exword_setpath(device, "\\_INTERNAL_00", 0);
	exword_authinfo(device, &info);
	exword_userid(device, u);
	printf("User %s with key 0x", u.name);
	for (i = 0; i < 20; i++) {
		printf("%02X", info.challenge[i] & 0xff);
	}
	printf(" registered\n");
	if (!_save_user_key(user, info.challenge))
		printf("Warning - Failed to save authentication info!\n");
	return dict_auth(device, user, info.challenge);
}

int dict_list_remote(exword_t *device, char* root)
{
	int rsp, length, i, len;
	char * locale;
	admini_t *info = NULL;
	exword_setpath(device, root, 0);
	rsp = _read_admini(device, (char **)&info, &length);
	if (rsp >= 0) {
		for (i = 0; i < length / 180; i++) {
			locale =  convert_to_locale("SHIFT_JIS", &locale, &len, info[i].name, strlen(info[i].name) + 1);
			printf("%d. %s (%s)\n", i, (locale == NULL ? info[i].name : locale), info[i].id);
			free(locale);
		}
	}
	free(info);
	return 1;
}

int dict_list_local()
{
	DIR *dir;
	char *name, *path;
	char *locale;
	int len, i = 0;
	struct dirent *entry;
	dir = opendir(get_data_dir());
	if (!dir)
		return 0;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] == '.')
			continue;
		path = mkpath(PATH_SEP, get_data_dir(), entry->d_name, NULL);
		name = _get_name(path);
		if (name) {
			locale =  convert_to_locale("SHIFT_JIS", &locale, &len, name, strlen(name) + 1);
			printf("%d. %s (%s)\n", i, (locale == NULL ? name : locale), entry->d_name);
			free(locale);
			free(name);
			++i;
		}
		free(path);
	}
	closedir(dir);
	return 1;
}

int dict_remove(exword_t *device, char *root, char *id)
{
	admini_t info;
	exword_cryptkey_t ck;
	int rsp;
	if (!_find(device, root, id, &info)) {
		printf("No dictionary with id %s installed.\n", id);
		return 0;
	}
	memset(&ck, 0, sizeof(exword_cryptkey_t));
	memcpy(ck.blk1, info.key, 2);
	memcpy(ck.blk1 + 10, info.key + 10, 2);
	memcpy(ck.blk2, info.key + 2, 8);
	memcpy(ck.blk2 + 8, info.key + 12, 4);
	printf("Removing %s...", id);
	rsp = exword_unlock(device);
	rsp |= exword_cname(device, info.name, id);
	rsp |= exword_cryptkey(device, &ck);
	if (rsp == EXWORD_SUCCESS)
		rsp |= exword_remove_file(device, id, 0);
	rsp |= exword_lock(device);
	if (rsp == EXWORD_SUCCESS)
		printf("Done\n");
	else
		printf("Failed\n");
	return (rsp == EXWORD_SUCCESS);
}

int dict_install(exword_t *device, char *root, char *id)
{
	DIR *dhandle;
	struct dirent *entry;
	admini_t info;
	exword_cryptkey_t ck;
	exword_capacity_t cap;
	int rsp;
	char *name;
	char *path;
	char *filename;
	char *dir;
	struct stat buf;
	int size;
	memset(&ck, 0, sizeof(exword_cryptkey_t));
	memcpy(ck.blk1, key1, 2);
	memcpy(ck.blk1 + 10, key1 + 10, 2);
	memcpy(ck.blk2, key1 + 2, 8);
	memcpy(ck.blk2 + 8, key1 + 12, 4);
	if (_find(device, root, id, &info)) {
		printf("Dictionary with id %s already installed.\n", id);
		return 0;
	}
	dir = mkpath(PATH_SEP, get_data_dir(), id, NULL);
	dhandle = opendir(dir);
	if (dhandle == NULL) {
		printf("Can find dictionary directory %s.\n", id);
		free(dir);
		return 0;
	}
	size = _get_size(dir);
	rsp = exword_get_capacity(device, &cap);
	if (rsp != EXWORD_SUCCESS || size >= cap.free || size < 0) {
		printf("Insufficent space on device.\n");
		free(dir);
		closedir(dhandle);
		return 0;
	}
	name = _get_name(dir);
	if (name == NULL) {
		printf("%s: missing diction.htm\n", id);
		free(dir);
		return 0;
	}
	rsp = exword_unlock(device);
	rsp |= exword_cname(device, name, id);
	rsp |= exword_cryptkey(device, &ck);
	free(name);
	if (rsp == EXWORD_SUCCESS) {
		path = mkpath("\\", root, id, "_CONTENT", NULL);
		exword_setpath(device, path, 1);
		free(path);
		while ((entry = readdir(dhandle)) != NULL) {
			filename = mkpath(PATH_SEP, dir, entry->d_name, NULL);
			if (stat(filename, &buf) == 0 && S_ISREG(buf.st_mode)) {
				printf("Transferring %s...", entry->d_name);
				if (_upload_file(device, dir, entry->d_name, ck.xorkey))
					printf("Done\n");
				else
					printf("Failed\n");
			}
			free(filename);
		}
		closedir(dhandle);
		path = mkpath("\\", root, id, "_USER", NULL);
		exword_setpath(device, path, 1);
		free(path);
	}
	free(dir);
	rsp |= exword_lock(device);
	return (rsp == EXWORD_SUCCESS);
}
