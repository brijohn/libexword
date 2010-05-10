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

#include <stdlib.h>
#include <stdio.h>
#include "obex.h"
#include "exword.h"

int debug;
const char Model[] = {0,'_',0,'M',0,'o',0,'d',0,'e',0,'l',0,0};
const char List[] = {0,'_',0,'L',0,'i',0,'s',0,'t',0,0};
const char Remove[] = {0,'_',0,'R',0,'e',0,'m',0,'o',0,'v',0,'e',0,0};
const char Cap[] = {0,'_',0,'C',0,'a',0,'p',0,0};
const char Unlock[] = {0,'_',0,'U',0,'n',0,'l',0,'o',0,'c',0,'k',0,0};
const char Lock[] = {0,'_',0, 'L',0,'o',0,'c',0,'k',0,0};
const char CName[] = {0,'_',0,'C',0,'N',0,'a',0,'m',0,'e',0,0};
const char CryptKey[] = {0,'_',0,'C',0,'r',0,'y',0,'p',0,'t',0,'K',0,'e',0,'y',0,0};


struct _exword {
	obex_t *obex_ctx;
};

static int exword_char_to_unicode(uint8_t *uc, const uint8_t *c)
{
	int len, n;


	len = n = strlen((char *) c);

	uc[n*2+1] = 0;
	uc[n*2] = 0;

	while (n--) {
		uc[n*2+1] = c[n];
		uc[n*2] = 0;
	}

	return (len * 2) + 2;
}

exword_t * exword_open(exword_device_t *device)
{
	if (device == NULL)
		return NULL;

	exword_t *self = malloc(sizeof(exword_t));
	if (self == NULL)
		return NULL;

	self->obex_ctx = obex_init(device->vid, device->pid);
	if (self->obex_ctx == NULL) {
		free(self);
		return NULL;
	}
	return self;
}

exword_t * exword_open_by_vid_pid(uint16_t vid, uint16_t pid)
{
	exword_device_t *devices = NULL;
	exword_t *self = NULL;
	int num;
	int i;
	num = exword_scan_devices(&devices);
	if (num <= 0)
		return NULL;
	for (i = 0; i < num; i++) {
		if (devices[i].vid == vid && devices[i].pid == pid) {
			self = exword_open(&devices[i]);
			if (self)
				break;
		}
	}
	exword_free_devices(devices);
	return self;
}

void exword_close(exword_t *self)
{
	if (self) {
		obex_cleanup(self->obex_ctx);
		free(self);
	}
}

void exword_set_debug(exword_t *self, int level)
{
	debug = level;
}

int exword_connect(exword_t *self)
{
	int rsp;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_CONNECT);
	if (obj == NULL)
		return -1;
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_send_file(exword_t *self, char* filename, char *file_data, int length)
{
	int len, rsp;
	obex_headerdata_t hv;
	uint8_t *unicode = malloc(strlen(filename) * 2 + 2);
	if (unicode == NULL)
		return -1;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		free(unicode);
		return -1;
	}
	len = exword_char_to_unicode(unicode, filename);
	hv.bs = unicode;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, len, 0);
	hv.bq4 = len;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = file_data;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, length, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	free(unicode);
	return rsp;
}

int exword_remove_file(exword_t *self, char* filename)
{
	int rsp, len;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		return -1;
	}
	hv.bs = Remove;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 16, 0);
	hv.bq4 = len = strlen(filename) + 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = filename;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, len, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_setpath(exword_t *self, uint8_t *path)
{
	int len, rsp;
	obex_headerdata_t hv;
	uint8_t *unicode = malloc(strlen(path) * 2 + 2);
	if (unicode == NULL)
		return -1;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_SETPATH);
	if (obj == NULL) {
		free(unicode);
		return -1;
	}
	if (strlen(path) == 0) {
		len = 0;
		hv.bs = path;
	} else {
		len = exword_char_to_unicode(unicode, path);
		hv.bs = unicode;
	}
	obex_object_set_nonhdr_data(obj, "\x02\x00", 2);
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, len, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	free(unicode);
	return rsp;
}

int exword_get_model(exword_t *self, uint8_t * model, uint16_t *count)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return -1;
	hv.bs = Model;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 14, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_BODY) {
				*count = (*count < hv_size ? *count : hv_size);
				memcpy(model, hv.bs, *count);
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_get_capacity(exword_t *self, exword_capacity_t *cap)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return -1;
	hv.bs = Cap;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 10, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_BODY) {
				memcpy(cap, hv.bs, sizeof(exword_capacity_t));
				cap->total = ntohl(cap->total);
				cap->used = ntohl(cap->used);
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_list(exword_t *self, directory_entry_t **entries, uint16_t *count)
{
	int rsp;
	int i, size;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return -1;
	hv.bs = List;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 12, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_BODY) {
				*count = ntohs(*(uint16_t*)hv.bs);
				hv.bs += 2;
				*entries = malloc(sizeof(directory_entry_t) * *count);
				memset(*entries, 0, sizeof(directory_entry_t) * *count);
				for (i = 0; i < *count; i++) {
					size = ntohs(*(uint16_t*)hv.bs);
					memcpy(*entries + i, hv.bs, size);
					(*entries)[i].size = size;
					hv.bs += size;
				}
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_disconnect(exword_t *self)
{
	int rsp;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_DISCONNECT);
	if (obj == NULL)
		return -1;
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

void exword_free_devices(exword_device_t *devices)
{
	free(devices);
}

int exword_scan_devices(exword_device_t **devices)
{
	int i, num = 0;
	ssize_t ret = 0;
	libusb_device **dev_list;
	libusb_device *device;
	libusb_device_handle *dev;
	struct libusb_device_descriptor desc;
	ret = libusb_init(NULL);
	if (ret < 0)
		goto done;
	ret = libusb_get_device_list(NULL, &dev_list);
	if (ret < 0)
		goto done;
	*devices = malloc(sizeof(exword_device_t) * ret);
	memset(*devices, 0, sizeof(exword_device_t) * ret);
	if (*devices == NULL)
		return -1;
	for (i = 0; i < ret; i++) {
		device = dev_list[i];
		if (libusb_get_device_descriptor(device, &desc) == 0) {
			if (desc.idVendor == 0x7cf) {
				if (libusb_open(device, &dev) >= 0) {
					(*devices)[num].vid = desc.idVendor;
					(*devices)[num].pid = desc.idProduct;
					libusb_get_string_descriptor_ascii(dev, desc.iManufacturer, (*devices)[num].manufacturer, 20);
					libusb_get_string_descriptor_ascii(dev, desc.iProduct, (*devices)[num].product, 20);
					libusb_close(dev);
					num++;
				}
			}
		}
	}
	*devices = realloc(*devices, sizeof(exword_device_t) * num);
done:
	libusb_free_device_list(dev_list, 1);
	libusb_exit(NULL);
	return num;
}

char *exword_response_to_string(int rsp)
{
	switch (rsp) {
	case OBEX_RSP_CONTINUE:
		return "Continue";
	case OBEX_RSP_SWITCH_PRO:
		return "Switching protocols";
	case OBEX_RSP_SUCCESS:
		return "OK, Success";
	case OBEX_RSP_CREATED:
		return "Created";
	case OBEX_RSP_ACCEPTED:
		return "Accepted";
	case OBEX_RSP_NO_CONTENT:
		return "No Content";
	case OBEX_RSP_BAD_REQUEST:
		return "Bad Request";
	case OBEX_RSP_UNAUTHORIZED:
		return "Unauthorized";
	case OBEX_RSP_PAYMENT_REQUIRED:
		return "Payment required";
	case OBEX_RSP_FORBIDDEN:
		return "Forbidden";
	case OBEX_RSP_NOT_FOUND:
		return "Not found";
	case OBEX_RSP_METHOD_NOT_ALLOWED:
		return "Method not allowed";
	case OBEX_RSP_CONFLICT:
		return "Conflict";
	case OBEX_RSP_INTERNAL_SERVER_ERROR:
		return "Internal server error";
	case OBEX_RSP_NOT_IMPLEMENTED:
		return "Not implemented!";
	case OBEX_RSP_DATABASE_FULL:
		return "Database full";
	case OBEX_RSP_DATABASE_LOCKED:
		return "Database locked";
	default:
		return "Unknown response";
	}
}
