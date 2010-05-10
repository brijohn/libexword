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
#define CMD_MASK	0xff

#define DEV_SD		0x0100
#define DEV_INTERNAL	0x0200
uint8_t  debug_level;
uint16_t command;
uint8_t  do_scan;
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
        { "scan", 0, 0, &do_scan, 1,
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
	rsp = exword_connect(d);
	if (rsp != 0x20)
		goto fail;
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
	rsp = exword_disconnect(d);
	if (rsp != 0x20)
		goto fail;
	return 0;
fail:
	fprintf(stderr, "Failed operation (%d)\n", rsp);
	return rsp;
}

int display_capacity(exword_t *d)
{
	int rsp;
	exword_capacity_t cap;
	rsp = exword_connect(d);
	if (rsp != 0x20)
		goto fail;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_get_capacity(d, &cap);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_disconnect(d);
	if (rsp != 0x20)
		goto fail;
	if (command & DEV_SD)
		printf("SD Capacity: %d / %d\n", cap.total, cap.used);
	else
		printf("Internal Capacity: %d / %d\n", cap.total, cap.used);
	return 0;
fail:
	fprintf(stderr, "Failed operation (%d)\n", rsp);
	return rsp;
}

int send_file(exword_t *d, char *filename)
{
	int rsp, len;
	char *buffer;
	rsp = read_file(filename, &buffer, &len);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_connect(d);
	if (rsp != 0x20)
		goto fail;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_send_file(d, basename(filename), buffer, len);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_disconnect(d);
	if (rsp != 0x20)
		goto fail;
	free(buffer);
	return 0x20;
fail:
	free(buffer);
	fprintf(stderr, "Failed operation (%X)\n", rsp);
	return rsp;
}

int delete_file(exword_t *d, char *filename)
{
	int rsp;
	if (filename == NULL)
		return 0x50;
	rsp = exword_connect(d);
	if (rsp != 0x20)
		goto fail;
	if (command & DEV_SD)
		rsp = exword_setpath(d, SD_CARD);
	else
		rsp = exword_setpath(d, INTERNAL_MEM);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_remove_file(d, basename(filename));
	if (rsp != 0x20)
		goto fail;
	rsp = exword_disconnect(d);
	if (rsp != 0x20)
		goto fail;
	return 0;
fail:
	fprintf(stderr, "Failed operation (%X)\n", rsp);
	return rsp;
}

int display_model(exword_t *d)
{
	int rsp;
	uint16_t len = 30;
	char model[30];
	rsp = exword_connect(d);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_get_model(d, model, &len);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_disconnect(d);
	if (rsp != 0x20)
		goto fail;
	printf("Model: %s\n", model);
	return 0;
fail:
	fprintf(stderr, "Failed operation (%d)\n", rsp);
	return rsp;
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
	exit(0);
}

int main(int argc, const char** argv)
{
	poptContext optCon;
	exword_t *device;
	uint16_t vid, pid;
	optCon = poptGetContext(NULL, argc, argv, options, 0);
	poptGetNextOpt(optCon);
	if ((command & DEV_SD) && (command & DEV_INTERNAL))
		usage(optCon, 1, "--sd and --internal options mutually exclusive");
	if (command == 0)
		command |= DEV_INTERNAL;
	if (do_scan) {
		scan_devices();
	}
	if (usb_device == NULL || sscanf(usb_device, "%hx:%hx", &vid, &pid) != 2) {
		usage(optCon, 1, "Bad or no device ID specified");
	}
	device = exword_open_by_vid_pid(vid, pid);
	if (device == NULL) {
		fprintf(stderr, "Failed to open %04X:%04X\n", vid, pid);
	} else {
		exword_set_debug(device, debug_level);
		switch(command & CMD_MASK) {
			case CMD_LIST:
				list_files(device);
				break;
			case CMD_MODEL:
				display_model(device);
				break;
			case CMD_CAPACITY:
				display_capacity(device);
				break;
			case CMD_SEND:
				send_file(device, filename);
				break;
			case CMD_DELETE:
				delete_file(device, filename);
				break;
			default:
				usage(optCon, 1, "No such command");
		}
		exword_close(device);
	}
	return 0;
}
