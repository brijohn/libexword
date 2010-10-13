/* exword - program for transfering files to Casio-EX-Word dictionaries
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

#include <popt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "exword.h"

#define CMD_NONE	0x0
#define CMD_LIST	0x1
#define CMD_MODEL	0x2
#define CMD_SEND	0x4
#define CMD_DELETE	0x8
#define CMD_CAPACITY	0x10
#define CMD_DISCONNECT	0x20
#define CMD_CONNECT	0x40
#define CMD_SCAN	0x80

#define CMD_MASK	0xff
#define REQ_CON_MASK	(CMD_LIST      | \
			CMD_MODEL      | \
			CMD_SEND       | \
			CMD_DELETE     | \
			CMD_CAPACITY   | \
			CMD_DISCONNECT)

#define DEV_SD		0x0100
#define DEV_INTERNAL	0x0200
#define CMD_INTERACTIVE 0x8000

uint8_t  debug_level;
uint16_t command;
uint16_t vid, pid;
char    *usb_device;
char    *filename;



struct poptOption options[] = {
        { "sd", 0, POPT_BIT_SET, &command, DEV_SD,
        "access external sd card" },
        { "internal", 0, POPT_BIT_SET, &command, DEV_INTERNAL,
        "access internal memory (default)" },
        { "list", 0, POPT_BIT_SET, &command, CMD_LIST,
        "list files on device" },
        { "send", 0, POPT_BIT_SET, &command, CMD_SEND,
        "send file to device" },
        { "delete", 0, POPT_BIT_SET, &command, CMD_DELETE,
        "delete file from device" },
        { "model", 0, POPT_BIT_SET, &command, CMD_MODEL,
        "get device model" },
        { "capacity", 0, POPT_BIT_SET, &command, CMD_CAPACITY,
        "get device capacity" },
        { "interactive", 0, POPT_BIT_SET, &command, CMD_INTERACTIVE,
        "interactive mode" },
        { "scan", 0, POPT_BIT_SET, &command, CMD_SCAN,
        "scan system for device ids" },
        { "file", 0, POPT_ARG_STRING, &filename, 0,
        "file name to transfer/delete" },
        { "device", 0, POPT_ARG_STRING, &usb_device, 0,
        "usb device (eg 0c7f:6101)" },
        { "debug", 0, POPT_ARG_INT, &debug_level, 0,
        "debug level (0..5)" },
        POPT_AUTOHELP
        { NULL, 0, 0, NULL, 0 }
   };


void usage(poptContext optCon, int exitcode, char *error) {
	poptPrintUsage(optCon, stderr, 0);
	if (error) fprintf(stderr, "%s\n", error);
	poptFreeContext(optCon);
	exit(exitcode);
}

int read_file(char* filename, char **buffer, int *len)
{
	int fd;
	struct stat buf;
	*buffer = NULL;
	if (filename == NULL)
		return 0x44;
	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return 0x44;
	if (fstat(fd, &buf) < 0) {
		close(fd);
		return 0x44;
	}
	*buffer = malloc(buf.st_size);
	if (*buffer == NULL) {
		close(fd);
		return 0x50;
	}
	*len = read(fd, *buffer, buf.st_size);
	if (*len < 0) {
		free(buffer);
		*buffer = NULL;
		close(fd);
		return 0x50;
	}
	close(fd);
	return 0x20;
}

int list_files(exword_t *d)
{
	int rsp, i;
	directory_entry_t *entries;
	uint16_t len;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_list(d, &entries, &len);
	if (rsp != 0x20)
		goto fail;
	for (i = 0; i < len; i++) {
		if (entries[i].flags)
			printf("<%s>\n", entries[i].name);
		else
			printf("%s\n", entries[i].name);
	}
	free(entries);
fail:
	return rsp;
}

int display_capacity(exword_t *d)
{
	int rsp;
	exword_capacity_t cap;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_get_capacity(d, &cap);
	if (rsp != 0x20)
		goto fail;
	if (command & DEV_SD)
		printf("SD Capacity: %d / %d\n", cap.total, cap.used);
	else
		printf("Internal Capacity: %d / %d\n", cap.total, cap.used);
fail:
	return rsp;
}

int send_file(exword_t *d, char *filename)
{
	int rsp, len;
	char *buffer;
	rsp = read_file(filename, &buffer, &len);
	if (rsp != 0x20)
		goto fail;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_send_file(d, basename(filename), buffer, len);
fail:
	free(buffer);
	return rsp;
}

int delete_file(exword_t *d, char *filename)
{
	int rsp;
	if (filename == NULL)
		return 0x40;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_remove_file(d, basename(filename));
fail:
	return rsp;
}

int display_model(exword_t *d)
{
	int rsp;
	uint16_t len = 30;
	char model[30];
	rsp = exword_get_model(d, model, &len);
	if (rsp != 0x20)
		goto fail;
	printf("Model: %s\n", model);
fail:
	return rsp;
}

int connect(exword_t *d)
{
	int rsp;
	rsp = exword_connect(d);
	return rsp;
}

int disconnect(exword_t *d)
{
	int rsp;
	rsp = exword_disconnect(d);
	return rsp;
}

int parse_commandline(char *cmdl)
{
	int ret = 0;
	char *cmd = NULL;
	command &= ~CMD_MASK;
	sscanf(cmdl, "%ms", &cmd);
	if (strcmp(cmd, "scan") == 0) {
		command |= CMD_SCAN;
	} else if (strcmp(cmd, "help") == 0) {
		printf("Commands:\n");
		printf("scan               - List Devices\n");
		printf("connect <vid:pid>  - connect to dictionary\n");
		printf("disconnect         - disconnect to dictionary\n");
		printf("model              - print model\n");
		printf("storage <sd|mem>   - switch storage medium\n");
		printf("list               - list files\n");
		printf("capacity           - display medium capacity\n");
		printf("delete  <filename> - delete filename\n");
		printf("send    <filename> - upload filename\n");
	} else if (strcmp(cmd, "connect") == 0) {
		if (sscanf(cmdl, "%*s %hx:%hx", &vid, &pid) != 2)
			printf("connect requires a vid and pid\n");
		else
			command |= CMD_CONNECT;
	} else if (strcmp(cmd, "storage") == 0) {
		char type[4];
		sscanf(cmdl, "%*s %4s", type);
		if ((strcmp(type, "sd") && strcmp(type, "mem"))) {
			printf("Requires either 'sd' or 'mem'\n");
		} else {
			if (strcmp(type, "sd") == 0) {
				command &= ~DEV_INTERNAL;
				command |= DEV_SD;
				printf("Switching to SD Card\n");
			} else {
				command &= ~DEV_SD;
				command |= DEV_INTERNAL;
				printf("Switching to Internal Memory\n");
			}
		}
	} else if(strcmp(cmd, "delete") == 0 ||
		  strcmp(cmd, "send") == 0) {
		free(filename);
		filename = NULL;
		sscanf(cmdl, "%*s %ms", &filename);
		if (filename == NULL) {
			printf("Requires filename\n");
		} else {
			if (strcmp(cmd, "delete") == 0)
				command |= CMD_DELETE;
			else
				command |= CMD_SEND;
		}
	} else if(strcmp(cmd, "exit") == 0) {
		ret = 1;
	} else if (strcmp(cmd, "disconnect") == 0) {
		command |= CMD_DISCONNECT;
	} else if (strcmp(cmd, "model") == 0) {
		command |= CMD_MODEL;
	} else if (strcmp(cmd, "capacity") == 0) {
		command |= CMD_CAPACITY;
	} else if (strcmp(cmd, "list") == 0) {
		command |= CMD_LIST;
	} else {
		ret = -1;
	}
	free(cmd);
	return ret;
}

void interactive() {
	char line[BUFSIZ];
	int rsp, ret = 0;
	exword_t *handle = NULL;

	printf("exword interactive mode\n");
	while (ret != 1) {
		printf(">> ");
		fgets(line, BUFSIZ, stdin);
		ret = parse_commandline(line);

		if (ret < 0) {
			printf("Invalid command\n");
			continue;
		}

		if (handle == NULL &&
		    command & REQ_CON_MASK) {
			printf("Not connected\n");
			continue;
		}

		switch(command & CMD_MASK) {
		case CMD_SCAN:
			scan_devices();
			break;
		case CMD_CONNECT:
			printf("connecting to %04X:%04X...", vid, pid);
			handle = exword_open_by_vid_pid(vid, pid);
			if (handle == NULL) {
				printf("device not found\n");
			} else {
				if(connect(handle) != 0x20) {
					printf("connect failed\n");
					exword_close(handle);
					handle = NULL;
				} else {
					printf("done\n");
				}
			}
			break;
		case CMD_DISCONNECT:
			printf("disconnecting...");
			disconnect(handle);
			exword_close(handle);
			handle = NULL;
			printf("done\n");
			break;
		case CMD_MODEL:
			rsp = display_model(handle);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_LIST:
			rsp = list_files(handle);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_CAPACITY:
			rsp = display_capacity(handle);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_DELETE:
			printf("deleting...");
			rsp = delete_file(handle, filename);
			printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_SEND:
			printf("uploading...");
			rsp = send_file(handle, filename);
			printf("%s\n", exword_response_to_string(rsp));
			break;
		}
	}
	if (handle != NULL) {
		disconnect(handle);
		exword_close(handle);
	}
	free(filename);
}

int scan_devices()
{
	int i, num;
	exword_device_t *devices = NULL;
	num = exword_scan_devices(&devices);
	if (num > 0) {
		printf("%s\n", "Devices:");
		for (i = 0; i< num; i++) {
			printf("%d: %04X:%04X %s %s\n", i, devices[i].vid,
							devices[i].pid,
							devices[i].manufacturer,
							devices[i].product);
		}
		exword_free_devices(devices);
	} else {
		printf("%s\n", "No Devices Found.");
	}
}

int main(int argc, const char** argv)
{
	poptContext optCon;
	exword_t *device;
	uint16_t vid, pid;
	int rsp;
	optCon = poptGetContext(NULL, argc, argv, options, 0);
	poptGetNextOpt(optCon);
	if (command & CMD_INTERACTIVE) {
		interactive();
		exit(0);
	}

	if ((command & DEV_SD) && (command & DEV_INTERNAL))
		usage(optCon, 1, "--sd and --internal options mutually exclusive");
	if (command == 0)
		command |= DEV_INTERNAL;
	if (command & CMD_SCAN) {
		scan_devices();
		poptFreeContext(optCon);
		exit(0);
	}
	if (usb_device == NULL || sscanf(usb_device, "%hx:%hx", &vid, &pid) != 2) {
		usage(optCon, 1, "Bad or no device ID specified");
	}
	device = exword_open_by_vid_pid(vid, pid);
	if (device == NULL) {
		fprintf(stderr, "Failed to open %04X:%04X\n", vid, pid);
	} else {
		exword_set_debug(device, debug_level);
		rsp = connect(device);
		if (rsp == 0x20) {
			switch(command & CMD_MASK) {
			case CMD_LIST:
				rsp = list_files(device);
				break;
			case CMD_MODEL:
				rsp = display_model(device);
				break;
			case CMD_CAPACITY:
				rsp = display_capacity(device);
				break;
			case CMD_SEND:
				rsp = send_file(device, filename);
				break;
			case CMD_DELETE:
				rsp = delete_file(device, filename);
				break;
			default:
				usage(optCon, 1, "No such command");
			}
		}
		disconnect(device);
		printf("%s\n", exword_response_to_string(rsp));
		exword_close(device);
	}
	poptFreeContext(optCon);
	return 0;
}
