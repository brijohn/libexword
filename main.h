/* exword - program for transfering files to Casio-EX-Word dictionaries
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
 */

#ifndef _MAIN_H
#define _MAIN_H

#include "util.h"

struct state {
	exword_t *device;
	int mode;
	int region;
	int running;
	int connected;
	int debug;
	int mkdir;
	int authenticated;
	int disconnect_event;
	char *cwd;
	struct list_head dev_list;
	struct list_head cmd_list;
};

typedef void (*cmd_func)(struct state *);

struct cmd_arg {
	char *arg;
	struct list_head queue;
};

struct command {
	char *cmd_str;
	cmd_func ptr;
	char *help_short;
	char *help_long;
	int  mode;
};

typedef struct {
	char id[32];
	char key[16];
	char name[132];
} admini_t;

int content_list_remote(struct state *s, char *root);
int content_list_local(struct state *s);
int content_remove(struct state *s, char *root, char *id);
int content_decrypt(struct state *s, char *root, char *id);
int content_install(struct state *s, char *root, char *id);
int content_auth(struct state *s, char *user, char *auth);

#endif
