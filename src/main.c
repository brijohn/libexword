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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <libgen.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "util.h"

void quit(struct state *s);
void help(struct state *s);
void connect(struct state *s);
void disconnect(struct state *s);
void set(struct state *s);
void model(struct state *s);
void capacity(struct state *s);
void format(struct state *s);
void list(struct state *s);
void delete(struct state *s);
void send(struct state *s);
void get(struct state *s);
void setpath(struct state *s);
void content(struct state *s);

static struct state *st = NULL;

struct command commands[] = {
{"connect", connect, "connect [mode] [region]\t- connect to attached dictionary\n",
	"Connects to device.\n\n"
	"Region specifies the region of the device (default:ja).\n"
	"Mode can be one of the following values:\n"
	"library - connect as CASIO Library (default)\n"
	"text    - connect as Textloader\n"
	"cd      - connect as CDLoader\n", 0x700},
{"disconnect", disconnect, "disconnect\t\t- disconnect from dictionary\n",
	"Disconnects from device.\n", 0x700},
{"model", model, "model\t\t\t- display model information\n",
	"Displays model information of device.\n", 0x700},
{"capacity", capacity, "capacity\t\t- display medium capacity\n",
	"Displays capacity of current storage medium.\n", 0x700},
{"format", format, "format\t\t\t- format SD card\n",
	"Formats currently inserted SD Card.\n", 0x700},
{"list", list, "list\t\t\t- list files\n",
	"Lists files and directories under current path.\n\n"
	"Directories are enclosed in <>.\n"
	"Files or directories beginning with * were returned as unicode.\n", 0x700},
{"delete", delete, "delete <filename>\t- delete a file\n",
	"Deletes a file from dicionary.\n", 0x700},
{"send", send, "send <filename>\t\t- upload a file\n",
	"Uploads a file to dicionary.\n", 0x700},
{"get", get, "get <filename>\t\t- download a file\n",
	"Downloads a file from dicionary.\n", 0x700},
{"setpath", setpath, "setpath <path>\t\t- changes directory on dictionary\n",
	"Changes to the the specified path.\n\n"
	"<path> is in the form of <device>://<path>\n"
	"Example: drv0:/// - sets path to root of internal memory\n", 0x700},
{"cd",  content, "cd <sub-function>\t- audio cd commands\n",
	"This command allows manipulation of installed audio cds. It uses\n"
	"the storage medium of your current path as the storage device to\n"
	"operate on. The reset sub-function WILL delete all installed\n"
	"dictionaries. DP4+ Only\n\n"
	"Sub functions:\n"
	"reset <user>\t    - resets authentication info\n"
	"auth <user> [key]   - authenticate to dictionary\n"
	"list [local|remote] - list installed audio cds\n"
	"decrypt <id>\t    - decrypts specified audio cd\n"
	"remove  <id>\t    - removes specified audio cd\n"
	"install <id>\t    - installs specified audio cd\n", 0x400},
{"dict", content, "dict <sub-function>\t- add-on dictionary commands\n",
	"This command allows manipulation of add-on dictionaries. It uses\n"
	"the storage medium of your current path as the storage device to\n"
	"operate on. The reset sub-function WILL delete all installed\n"
	"dictionaries.\n\n"
	"Sub functions:\n"
	"reset <user>\t    - resets authentication info\n"
	"auth <user> [key]   - authenticate to dictionary\n"
	"list [local|remote] - list installed add-on dictionaries\n"
	"decrypt <id>\t    - decrypts specified add-on dictionary\n"
	"remove  <id>\t    - removes specified add-on dictionary\n"
	"install <id>\t    - installs specified add-on dictionary\n", 0x100},
{"set", set, "set <option> [value]\t- sets program options\n",
	"Sets <option> to [value], if no value is specified will display current value.\n\n"
	"Available options:\n"
	"debug <level>  - sets debug level (0-5)\n"
	"mkdir <on|off> - specifies whether setpath should create directories\n", 0x700},
{"exit", quit, "exit\t\t\t- exits program\n",
	"Exits program and disconnects from device.\n", 0x700},
{"help", help, NULL, NULL, 0x700},
{NULL, NULL, NULL, NULL, 0}
};

void load_history()
{
	char * filename;
	const char *data_dir = get_data_dir();
	filename = mkpath(PATH_SEP, data_dir, ".exword_history", NULL);
	read_history(filename);
	free(filename);
}

void store_history()
{
	char * filename;
	const char *data_dir = get_data_dir();
	filename = mkpath(PATH_SEP, data_dir, ".exword_history", NULL);
	write_history(filename);
	free(filename);
}

void queue_arg(struct list_head *head, char * arg)
{
	struct cmd_arg * _arg = xmalloc(sizeof(struct cmd_arg));
	_arg->arg = xmalloc(strlen(arg) + 1);
	strcpy(_arg->arg, arg);
	list_add_tail(&(_arg->queue), head);
}

char * peek_arg(struct list_head *head)
{
	char * arg = NULL;
	struct cmd_arg *_arg;
	if (!list_empty(head)) {
		_arg = list_entry(head->next, struct cmd_arg, queue);
		arg = _arg->arg;
	}
	return arg;
}

void dequeue_arg(struct list_head *head)
{
	struct cmd_arg * _arg;
	if (!list_empty(head)) {
		_arg = list_entry(head->next, struct cmd_arg, queue);
		list_del(&(_arg->queue));
		free(_arg->arg);
		free(_arg);
	}
}

void clear_arg_list(struct list_head *head)
{
	while (!list_empty(head)) {
		dequeue_arg(head);
	}
}

void fill_arg_list(struct list_head *head, char * str)
{
	char * token = strtok(str, " \t");
	queue_arg(head, token);
	while ((token = strtok(NULL, " \t")) != NULL) {
		queue_arg(head, token);
	}
}

void disconnect_notify(int reason, void *data)
{
	struct state *s = (struct state *)data;
	if (reason == EXWORD_DISCONNECT_UNPLUGGED) {
		fprintf(stderr, "\nDevice was unplugged\n");
		s->disconnect_event = 1;
	} else if (reason == EXWORD_DISCONNECT_ERROR) {
		fprintf(stderr, "\nInternal Server Error\n");
		s->disconnect_event = 1;
	}
}

int _setpath(struct state *s, char* device, char *pathname, int mkdir)
{
	char *path, *p;
	int rsp, i = 0, j;
	path = mkpath("\\", device, pathname, NULL);
	p = path;
	while(p[0] != '\0') {
		while((p[0] == '/' || p[0] == '\\') &&
		      (p[1] == '/' || p[1] == '\\')) {
			char *t = p;
			while(t[0] != '\0') {
				t++;
				t[-1] = t[0];
			}
		}
		if (p[0] == '/')
			p[0] = '\\';
		p++;
	}
	rsp = exword_setpath(s->device, path, mkdir);
	if (rsp == EXWORD_SUCCESS) {
		free(s->cwd);
		s->cwd = xmalloc(strlen(path) + 1);
		strcpy(s->cwd, path);
	}
	free(path);
	return rsp;
}

void quit(struct state *s)
{
	s->running = 0;
	disconnect(s);
}

void help(struct state *s)
{
	int i;
	char *cmd = peek_arg(&(s->cmd_list));
	if (cmd == NULL) {
		for (i = 0; commands[i].cmd_str != NULL; i++) {
			if (commands[i].help_short != NULL)
				printf("%s", commands[i].help_short);
		}
	} else {
		for (i = 0; commands[i].cmd_str != NULL; i++) {
			if (strcmp(cmd, commands[i].cmd_str) == 0) {
				if (commands[i].help_long != NULL)
					printf("%s", commands[i].help_long);
				else
					printf("No help available for %s\n", cmd);
				break;
			}
		}
		if (commands[i].cmd_str == NULL)
			printf("%s is not a command\n", cmd);
	}
}

void connect(struct state *s)
{
	int  options = EXWORD_MODE_LIBRARY | EXWORD_REGION_JA;
	char *mode;
	char *region;
	int error = 0;
	uint16_t count;
	exword_dirent_t *entries;
	if (s->connected)
		return;

	mode = peek_arg(&(s->cmd_list));
	if (mode != NULL) {
		if (strcmp(mode, "library") == 0) {
			options = EXWORD_MODE_LIBRARY;
		} else if (strcmp(mode, "text") == 0) {
			options = EXWORD_MODE_TEXT;
		} else if (strcmp(mode, "cd") == 0) {
			options = EXWORD_MODE_CD;
		} else {
			printf("Unknown 'mode': %s\n", mode);
			error = 1;
		}
		dequeue_arg(&(s->cmd_list));
		region = peek_arg(&(s->cmd_list));
		if (!error && region != NULL) {
			if (strcmp(region, "ja") == 0) {
				options |= EXWORD_REGION_JA;
			} else if (strcmp(region, "kr") == 0) {
				options |= EXWORD_REGION_KR;
			} else if (strcmp(region, "cn") == 0) {
				options |= EXWORD_REGION_CN;
			} else if (strcmp(region, "in") == 0) {
				options |= EXWORD_REGION_IN;
			} else if (strcmp(region, "it") == 0) {
				options |= EXWORD_REGION_IT;
			} else if (strcmp(region, "de") == 0) {
				options |= EXWORD_REGION_DE;
			} else if (strcmp(region, "es") == 0) {
				options |= EXWORD_REGION_ES;
			} else if (strcmp(region, "fr") == 0) {
				options |= EXWORD_REGION_FR;
			} else if (strcmp(region, "ru") == 0) {
				options |= EXWORD_REGION_RU;
			} else {
				printf("Unknown 'region': %s\n", region);
				error = 1;
			}
		} else if (!error) {
			options |= EXWORD_REGION_JA;
		}
	}
	if (!error) {
		printf("connecting to device...");
		if (exword_connect(s->device, options) != EXWORD_SUCCESS) {
			printf("device not found\n");
		} else {
			if (exword_setpath(s->device, "", 0) == EXWORD_SUCCESS) {
				if (exword_list(s->device, &entries, &count) == EXWORD_SUCCESS) {
					dev_list_scan(&(s->dev_list), entries);
					exword_free_list(entries);
				}
			}
			_setpath(s, "\\_INTERNAL_00", "/", 2);
			s->connected = 1;
			s->mode = (options & 0xff00);
			s->region = (options & 0x00ff);
			printf("done\n");
		}
	}
}

void disconnect(struct state *s)
{
	int i;
	if (!s->connected)
		return;
	if (!s->disconnect_event)
		printf("disconnecting...done\n");
	else
		rl_stuff_char('\n');
	exword_disconnect(s->device);
	free(s->cwd);
	s->cwd = NULL;
	s->mode = 0;
	s->region = 0;
	s->connected = 0;
	s->authenticated = 0;
	s->disconnect_event = 0;
	dev_list_clear(&(s->dev_list));
}

void model(struct state *s)
{
	int rsp;
	exword_model_t model;
	if (!s->connected)
		return;
	rsp = exword_get_model(s->device, &model);
	if (rsp == EXWORD_SUCCESS) {
		printf("Model: %s\nSub: %s\n", model.model, model.sub_model);
		if (model.capabilities & CAP_EXT)
			printf("Extended: %s\n", model.ext_model);
		if ((model.capabilities & ~CAP_EXT) != 0) {
			printf("Capabilities: %s %s %s %s %s\n",
				model.capabilities & CAP_SW ? "SW" : "",
				model.capabilities & CAP_P ? "P" : "",
				model.capabilities & CAP_F ? "F" : "",
				model.capabilities & CAP_C ? "C" : "",
				model.capabilities & CAP_C2 ? "C" : "");
		}
	} else {
		printf("%s\n", exword_error_to_string(rsp));
	}
}

void capacity(struct state *s)
{
	int rsp;
	exword_capacity_t cap;
	if (!s->connected)
		return;
	rsp = exword_get_capacity(s->device, &cap);
	if (rsp == EXWORD_SUCCESS)
		printf("Capacity: %d / %d\n", cap.total, cap.free);
	else
		printf("%s\n", exword_error_to_string(rsp));
}

void format(struct state *s)
{
	int rsp;
	if (!s->connected)
		return;
	printf("Formatting SD Card...");
	rsp = exword_sd_format(s->device);
	printf("%s\n", exword_error_to_string(rsp));
}

void send(struct state *s)
{
	int rsp, len;
	char *buffer;
	char *filename;
	char *name = NULL;
	if (!s->connected)
		return;
	filename = peek_arg(&(s->cmd_list));
	if (filename == NULL) {
		printf("No file specified\n");
	} else {
		name = xmalloc(strlen(filename) + 1);
		strcpy(name, filename);
		printf("uploading...");
		rsp = read_file(name, &buffer, &len);
		if (rsp == 0)
			rsp = exword_send_file(s->device, basename(name), buffer, len);
		free(name);
		free(buffer);
		printf("%s\n", exword_error_to_string(rsp));
	}
}

void get(struct state *s)
{
	int rsp, len;
	char *buffer = NULL, *name = NULL;
	char *filename;
	if (!s->connected)
		return;
	filename = peek_arg(&(s->cmd_list));
	if (filename == NULL) {
		printf("No file specified\n");
	} else {
		name = xmalloc(strlen(filename) + 1);
		strcpy(name, filename);
		printf("downloading...");
		rsp = exword_get_file(s->device, basename(name), &buffer, &len);
		if (rsp == EXWORD_SUCCESS)
			rsp = write_file(filename, buffer, len);
		free(name);
		free(buffer);
		printf("%s\n", exword_error_to_string(rsp));
	}
}

void delete(struct state *s)
{
	int rsp;
	char * filename;
	if (!s->connected)
		return;
	filename = peek_arg(&(s->cmd_list));
	if (filename == NULL) {
		printf("No file specified\n");
	} else {
		printf("deleting file...");
		if (filename[0] == '*')
			rsp = exword_remove_file(s->device, filename + 1, 1);
		else
			rsp = exword_remove_file(s->device, filename, 0);
		printf("%s\n", exword_error_to_string(rsp));
	}
}

void list(struct state *s)
{
	int rsp, i;
	char * name;
	int len;
	exword_dirent_t *entries;
	uint16_t count;
	if (!s->connected)
		return;
	rsp = exword_list(s->device, &entries, &count);
	if (rsp == EXWORD_SUCCESS) {
		for (i = 0; i < count; i++) {
			if (ENTRY_IS_UNICODE(&entries[i])) {
				convert_to_locale("UTF-16BE", &name,
						  &len, entries[i].name,
						  entries[i].size - 3);
				if (ENTRY_IS_DIRECTORY(&entries[i]))
					printf("<*%s>\n", name);
				else
					printf("*%s\n", name);
			} else {
				if (ENTRY_IS_DIRECTORY(&entries[i]))
					printf("<%s>\n", entries[i].name);
				else
					printf("%s\n", entries[i].name);
			}
		}
		exword_free_list(entries);
	}
	printf("%s\n", exword_error_to_string(rsp));
}

void content(struct state *s)
{
	int i;
	char val;
	char *ptr;
	char *root;
	char authkey[41];
	char *subfunc, *id, *user;
	if (!s->connected)
		return;
	ptr = strchr(s->cwd + 1, '\\');
	root = xmalloc(ptr - s->cwd + 7);
	strncpy(root, s->cwd, ptr - s->cwd);
	if (s->mode == EXWORD_MODE_CD) {
		strcat(root, "\\SOUND");
	}
	if (peek_arg(&(s->cmd_list)) == NULL) {
		printf("No sub-function specified.\n");
	} else {
		subfunc = xmalloc(strlen(peek_arg(&(s->cmd_list)) + 1));
		strcpy(subfunc, peek_arg(&(s->cmd_list)));
		dequeue_arg(&(s->cmd_list));
		if (strcmp(subfunc, "list") == 0) {
			id = peek_arg(&(s->cmd_list));
			if (id == NULL || strcmp(id, "remote") == 0)
				content_list_remote(s, root);
			else if (strcmp(id, "local") == 0)
				content_list_local(s);
			else
				printf("Unknown parameter passed to list.\n");
		} else if (strcmp(subfunc, "reset") == 0) {
			if (peek_arg(&(s->cmd_list)) == NULL) {
				printf("No username specified.\n");
			} else {
				user = peek_arg(&(s->cmd_list));
				if (content_reset(s, user))
					s->authenticated = 1;
				else
					s->authenticated = 0;
			}
		} else if (strcmp(subfunc, "auth") == 0) {
			if (peek_arg(&(s->cmd_list)) == NULL) {
				printf("No username specified.\n");
			} else {
				user = xmalloc(strlen(peek_arg(&(s->cmd_list)) + 1));
				strcpy(user, peek_arg(&(s->cmd_list)));
				dequeue_arg(&(s->cmd_list));
				if (peek_arg(&(s->cmd_list)) == NULL) {
					if (content_auth(s, user, NULL)) {
						s->authenticated = 1;
						printf("Authentication sucessful.\n");
					} else {
						printf("Authentication failed.\n");
					}
				} else {
					if (sscanf(peek_arg(&(s->cmd_list)), "0x%40[a-fA-F0-9]", authkey) < 1) {
						printf("Invalid character in authkey.\n");
					} else if (strlen(authkey) != 40) {
						printf("Authkey wrong length. Must be 20 bytes.\n");
					} else {
						for (i = 0; i < 40; i += 2) {
							if (authkey[i] >= 97)
								val = (authkey[i] - 87) << 4;
							else if (authkey[i] >= 65)
								val = (authkey[i] - 55) << 4;
							else if (authkey[i] >= 48)
								val = (authkey[i] - 48) << 4;
							if (authkey[i + 1] >= 97)
								val |= (authkey[i + 1] - 87);
							else if (authkey[i + 1] >= 65)
								val |= (authkey[i + 1] - 55);
							else if (authkey[i + 1] >= 48)
								val |= (authkey[i + 1] - 48);
							authkey[i / 2] = val;
						}
						if (content_auth(s, user, authkey)) {
							s->authenticated = 1;
							printf("Authentication sucessful.\n");
						} else {
							printf("Authentication failed.\n");
						}
					}
				}
				free(user);
			}
		} else if (strcmp(subfunc, "decrypt") == 0 ||
			   strcmp(subfunc, "remove")  == 0 ||
			   strcmp(subfunc, "install") == 0) {
			id = peek_arg(&(s->cmd_list));
			if (id == NULL) {
				printf("No id specified.\n");
			} else if (strlen(id) != 5) {
				printf("Id must be 5 characters long.\n");
			} else {
				if (!s->authenticated) {
					printf("Not authenticated.\n");
				} else {
					if (strcmp(subfunc, "decrypt") == 0)
						content_decrypt(s, root, id);
					else if (strcmp(subfunc, "install") == 0)
						content_install(s, root, id);
					else if (strcmp(subfunc, "remove") == 0)
						content_remove(s, root, id);
				}
			}
		} else {
			printf("Unknown subfunction\n");
		}
		if (exword_setpath(s->device, s->cwd, 0) != EXWORD_SUCCESS) {
			_setpath(s, root, "/", 0);
		}
		free(subfunc);
	}
	free(root);
}

void setpath(struct state *s)
{
	int rsp;
	char *arg;
	char *ptr;
	char *path;
	char *device;
	if (!s->connected)
		return;
	arg = peek_arg(&(s->cmd_list));
	if (arg == NULL) {
		printf("No path specified\n");
	} else {
		ptr = strstr(arg, "://");
		if (ptr != NULL) {
			device = xmalloc(ptr - arg + 1);
			path = xmalloc(strlen(ptr) - 2);
			strncpy(device, arg, ptr - arg);
			strcpy(path, ptr + 3);
			struct device_map *dev = dev_list_search(&(s->dev_list), device);
			if (dev != NULL) {
				rsp = _setpath(s, dev->root, path, s->mkdir);
				if (rsp != EXWORD_SUCCESS) {
					printf("%s\n", exword_error_to_string(rsp));
					exword_setpath(s->device, s->cwd, 0);
				}
			} else {
				printf("No such device `%s`.\n", device);
			}
			free(device);
			free(path);
		} else {
			printf("Invalid argument. Format <device>://<path>\n");
		}
	}
}

void set(struct state *s)
{
	char * opt;
	char * arg;
	opt = peek_arg(&(s->cmd_list));
	if (opt == NULL) {
		printf("No option specified\n");
	} else if (strcmp(opt, "debug") == 0) {
		uint8_t debug;
		dequeue_arg(&(s->cmd_list));
		arg = peek_arg(&(s->cmd_list));
		if (arg == NULL) {
			printf("Debug Level: %u\n", s->debug);
		} else {
			if (sscanf(arg, "%hhu", &debug) < 1) {
				printf("Invalid value\n");
			} else if (debug > 5) {
				printf("Value should be between 0 and 5\n");
			} else {
				s->debug = debug;
				exword_set_debug(s->device, s->debug);
			}
		}
	} else if (strcmp(opt, "mkdir") == 0) {
		dequeue_arg(&(s->cmd_list));
		arg = peek_arg(&(s->cmd_list));
		if (arg == NULL) {
			printf("Mkdir: %u\n", s->mkdir);
		} else {
			if (strcmp(arg, "on") == 0 ||
			    strcmp(arg, "yes") == 0 ||
			    strcmp(arg, "true") == 0) {
				s->mkdir = 1;
			} else if (strcmp(arg, "off") == 0 ||
				   strcmp(arg, "no") == 0 ||
				   strcmp(arg, "false") == 0) {
				s->mkdir = 0;
			} else {
				printf("Invalid value\n");
			}
		}
	} else {
		printf("Unknown option %s\n", opt);
	}
}

void process_command(struct state *s)
{
	int i;
	char * cmd = peek_arg(&(s->cmd_list));
	for (i = 0; commands[i].cmd_str != NULL; i++) {
		if (strcmp(cmd, commands[i].cmd_str) == 0) {
			if (commands[i].mode & s->mode || s->mode == 0) {
				dequeue_arg(&(s->cmd_list));
				commands[i].ptr(s);
			} else {
				printf("This option not supported in %s mode\n", mode_id2str(s->mode));
			}
			break;
		}
	}
	if (commands[i].cmd_str == NULL)
		printf("Unknown command\n");
}

char * create_prompt(char *cwd) {
	char *p;
	if (cwd == NULL) {
		p = xmalloc(4);
		strcpy(p, ">> ");
	} else {
		p = xmalloc(strlen(cwd) + 5);
		strcpy(p, cwd);
		strcat(p, " >> ");
	}
	return p;
}

int do_events()
{
	if (st->device) {
		exword_poll_disconnect(st->device);
		if (st->disconnect_event)
			disconnect(st);
	}
}

void interactive(struct state *s)
{
	char * line = NULL;
	char * prompt = NULL;
	printf("Exword dictionary tool.\n"
	       "Type 'help' for a list of commands.\n");
	s->running = 1;
	INIT_LIST_HEAD(&(s->cmd_list));
	INIT_LIST_HEAD(&(s->dev_list));
	load_history();
	s->device = exword_init();
	exword_register_disconnect_callback(s->device, disconnect_notify, s);
	exword_set_debug(s->device, s->debug);
	rl_set_keyboard_input_timeout(10000);
	rl_event_hook = do_events;
	st = s;
	while (s->running) {
		free(line);
		free(prompt);
		prompt = create_prompt(s->cwd);
		line = readline(prompt);
		if (line == NULL || *line == '\0')
			continue;
		add_history(line);
		fill_arg_list(&(s->cmd_list), line);
		process_command(s);
		clear_arg_list(&(s->cmd_list));
	}
	free(line);
	free(prompt);
	store_history();
	exword_deinit(s->device);
}

int main(int argc, const char** argv)
{
	struct state s;
	setlocale(LC_ALL, "");
	memset(&s, 0, sizeof(struct state));
	interactive(&s);
	return 0;
}
