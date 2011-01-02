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
#include <iconv.h>
#include <errno.h>
#include "obex.h"
#include "exword.h"

int debug;
const char Model[] = {0,'_',0,'M',0,'o',0,'d',0,'e',0,'l',0,0};
const char List[] = {0,'_',0,'L',0,'i',0,'s',0,'t',0,0};
const char Remove[] = {0,'_',0,'R',0,'e',0,'m',0,'o',0,'v',0,'e',0,0};
const char Cap[] = {0,'_',0,'C',0,'a',0,'p',0,0};
const char SdFormat[] = {0,'_',0,'S',0,'d',0,'F',0,'o',0,'r',0,'m',0,'a',0,'t',0,0};
const char UserId[] = {0,'_',0,'U',0,'s',0,'e',0,'r',0,'I',0,'d',0,0};
const char Unlock[] = {0,'_',0,'U',0,'n',0,'l',0,'o',0,'c',0,'k',0,0};
const char Lock[] = {0,'_',0, 'L',0,'o',0,'c',0,'k',0,0};
const char CName[] = {0,'_',0,'C',0,'N',0,'a',0,'m',0,'e',0,0};
const char CryptKey[] = {0,'_',0,'C',0,'r',0,'y',0,'p',0,'t',0,'K',0,'e',0,'y',0,0};
const char AuthChallenge[] = {0,'_',0,'A', 0, 'u', 0, 't', 0, 'h', 0, 'C', 0, 'h', 0, 'a', 0, 'l', 0, 'l', 0, 'e', 0, 'n', 0, 'g', 0, 'e', 0, 0};
const char AuthInfo[] = {0,'_',0,'A', 0, 'u', 0, 't', 0, 'h', 0, 'I', 0, 'n', 0, 'f', 0, 'o', 0, 0};

struct _exword {
	obex_t *obex_ctx;
	uint16_t vid;
	uint16_t pid;
	char manufacturer[20];
	char product[20];

	file_cb put_file_cb;
	file_cb get_file_cb;
	void * cb_userdata;
	char * cb_filename;
	uint32_t cb_filelength;
	uint32_t cb_transferred;
};

static char * convert (iconv_t cd,
		char **dst, int *dstsz,
		const char *src, int srcsz)
{
	size_t inleft, outleft, converted = 0;
	char *output, *outbuf, *tmp;
	const char *inbuf;
	size_t outlen;

	inleft = srcsz;
	inbuf = src;

	outlen = inleft;

	if (!(output = malloc(outlen))) {
		return NULL;
	}

	do {
		errno = 0;
		outbuf = output + converted;
		outleft = outlen - converted;
		converted = iconv(cd, (char **) &inbuf, &inleft, &outbuf, &outleft);
		if (converted != (size_t) -1 || errno == EINVAL)
			break;

		if (errno != E2BIG) {
			free(output);
			return NULL;
		}

		converted = outbuf - output;
		outlen += inleft * 2;

		if (!(tmp = realloc(output, outlen))) {
			free(output);
			return NULL;
		}

		output = tmp;
	} while (1);
	iconv(cd, NULL, NULL, &outbuf, &outleft);
	if (dst  != NULL)
		*dst = output;
	if (dstsz != NULL)
		*dstsz = (outbuf - output);
	return output;
}

char * convert_to_locale(char *fmt, char **dst, int *dstsz, const char *src, int srcsz)
{
	iconv_t cd;
	*dst = NULL;
	*dstsz = 0;
	cd = iconv_open("", fmt);
	if (cd == (iconv_t) -1)
		return NULL;
	*dst = convert(cd, dst, dstsz, src, srcsz);
	iconv_close(cd);
	return *dst;
}

char * locale_to_utf16(char **dst, int *dstsz, const char *src, int srcsz)
{
	iconv_t cd;
	*dst = NULL;
	*dstsz = 0;
	cd = iconv_open("UTF-16BE", "");
	if (cd == (iconv_t) -1)
		return NULL;
	*dst = convert(cd, dst, dstsz, src, srcsz);
	iconv_close(cd);
	return *dst;
}

char * utf16_to_locale(char **dst, int *dstsz, const char *src, int srcsz)
{
	iconv_t cd;
	*dst = NULL;
	*dstsz = 0;
	cd = iconv_open("", "UTF-16BE");
	if (cd == (iconv_t) -1)
		return NULL;
	*dst = convert(cd, dst, dstsz, src, srcsz);
	iconv_close(cd);
	return *dst;
}

int is_cmd(char *data, int length)
{
	if (length == 10) {
		if (memcmp(data, Cap, 10) == 0)
			return 1;
	}
	if (length == 12) {
		if (memcmp(data, List, 12) == 0 ||
		    memcmp(data, Lock, 12) == 0)
			return 1;
	}
	if (length == 14) {
		if (memcmp(data, Model, 14) == 0 ||
		    memcmp(data, CName, 14) == 0)
			return 1;
	}
	if (length == 16) {
		if (memcmp(data, Remove, 16) == 0 ||
		    memcmp(data, UserId, 16) == 0 ||
		    memcmp(data, Unlock, 16) == 0)
			return 1;
	}
	if (length == 20) {
		if (memcmp(data, SdFormat, 20) == 0 ||
		    memcmp(data, CryptKey, 20) == 0 ||
		    memcmp(data, AuthInfo, 20) == 0)
			return 1;
	}
	if (length == 30) {
		if (memcmp(data, AuthChallenge, 30) == 0)
			return 1;
	}
	return 0;
}

void exword_handle_callbacks(obex_t *self, obex_object_t *object, void *userdata)
{
	exword_t *exword = (exword_t*)userdata;
	char *tx_buffer = self->tx_msg->data;
	int len;
	struct list_head *pos;
	struct obex_header_element *h;
	struct obex_unicode_hdr * hdr;
	if (!exword)
		return;
	if (object->opcode == OBEX_CMD_PUT && exword->put_file_cb) {
		hdr = (struct obex_unicode_hdr *)(tx_buffer + 4);
		if (hdr->hi == OBEX_HDR_NAME &&
		    !is_cmd(hdr->hv, ntohs(hdr->hl) - 3)) {
			free(exword->cb_filename);
			utf16_to_locale(&exword->cb_filename, &len, hdr->hv, ntohs(hdr->hl) - 3);
			exword->cb_filelength = ntohl(*((uint32_t*)(tx_buffer + ntohs(hdr->hl) + 5)));
			exword->cb_transferred = 0;
			hdr = (struct obex_unicode_hdr *)(tx_buffer + ntohs(hdr->hl) + 9);
		}
		if (hdr->hi == OBEX_HDR_BODY || hdr->hi == OBEX_HDR_BODY_END) {
			exword->cb_transferred += ntohs(hdr->hl) - 3;
			exword->put_file_cb(exword->cb_filename,
					    exword->cb_transferred,
					    exword->cb_filelength,
					    exword->cb_userdata);
		}
	} else if (object->opcode == OBEX_CMD_GET && exword->get_file_cb) {
		hdr = (struct obex_unicode_hdr *)(tx_buffer + 4);
		if ((tx_buffer[2] != 0 || tx_buffer[3] != 3) && hdr->hi == OBEX_HDR_NAME) {
			if (!is_cmd(hdr->hv, ntohs(hdr->hl) - 3)) {
				free(exword->cb_filename);
				utf16_to_locale(&exword->cb_filename, &len, hdr->hv, ntohs(hdr->hl) - 3);
			} else {
				free(exword->cb_filename);
				exword->cb_filename = NULL;
			}
		}
		if (exword->cb_filename) {
			if (object->rx_body)
				exword->cb_transferred = object->rx_body->data_size;
			if (!list_empty(&object->rx_headerq)) {
				list_for_each(pos, &object->rx_headerq) {
					h = list_entry(pos, struct obex_header_element, link);
					if (h->hi == OBEX_HDR_LENGTH)
						exword->cb_filelength = ntohl(*((uint32_t*) h->buf->data));
					if (h->hi == OBEX_HDR_BODY)
						exword->cb_transferred = h->length;
				}
			}
			exword->get_file_cb(exword->cb_filename,
					    exword->cb_transferred,
					    exword->cb_filelength,
					    exword->cb_userdata);
		}
	}
}

exword_t * exword_open()
{
	return exword_open2(0x0020);
}

exword_t * exword_open2(uint16_t options)
{
	int i;
	ssize_t ret;
	uint8_t ver, locale;
	struct libusb_device_descriptor desc;
	libusb_device **dev_list = NULL;
	libusb_device *device = NULL;
	libusb_device_handle *dev = NULL;

	locale = options & 0xff;
	if (options & OPEN_TEXT)
		ver = locale;
	else if (options & OPEN_CD)
		ver = 0xf0;
	else
		ver = locale - 0x0f;

	exword_t *self = malloc(sizeof(exword_t));
	if (self == NULL)
		return NULL;
	memset(self, 0, sizeof(exword_t));
	ret = libusb_init(NULL);
	if (ret < 0)
		goto error;

	ret = libusb_get_device_list(NULL, &dev_list);
	if (ret < 0)
		goto error;

	for (i = 0; i < ret; i++) {
		device = dev_list[i];
		if (libusb_get_device_descriptor(device, &desc) == 0) {
			if (desc.idVendor == 0x07cf && desc.idProduct == 0x6101) {
				if (libusb_open(device, &dev) >= 0) {
					self->vid = desc.idVendor;
					self->pid = desc.idProduct;
					libusb_get_string_descriptor_ascii(dev, desc.iManufacturer, self->manufacturer, 20);
					libusb_get_string_descriptor_ascii(dev, desc.iProduct, self->product, 20);
					libusb_close(dev);
					break;
				}
			}
		}
	}

	if (i >= ret)
		goto error;

	libusb_exit(NULL);
	self->obex_ctx = obex_init(self->vid, self->pid);
	if (self->obex_ctx == NULL)
		goto error;
	obex_set_connect_info(self->obex_ctx, ver, locale);
	obex_register_callback(self->obex_ctx, exword_handle_callbacks, self);
	libusb_free_device_list(dev_list, 1);
	return self;

error:
	free(self);
	libusb_free_device_list(dev_list, 1);
	libusb_exit(NULL);
	return NULL;
}


void exword_close(exword_t *self)
{
	if (self) {
		obex_cleanup(self->obex_ctx);
		free(self->cb_filename);
		free(self);
	}
}

void exword_set_debug(int level)
{
	debug = level;
}

void exword_register_callbacks(exword_t *self, file_cb get, file_cb put, void *userdata)
{
	self->put_file_cb = put;
	self->get_file_cb = get;
	self->cb_userdata = userdata;
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

int exword_send_file(exword_t *self, char* filename, char *buffer, int len)
{
	int length, rsp;
	obex_headerdata_t hv;
	char *unicode;
	unicode = locale_to_utf16(&unicode, &length, filename, strlen(filename) + 1);
	if (unicode == NULL)
		return -1;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		free(unicode);
		return -1;
	}
	hv.bs = unicode;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, length, 0);
	hv.bq4 = len;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = buffer;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, len, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	free(unicode);
	return rsp;
}

int exword_get_file(exword_t *self, char* filename, char **buffer, int *len)
{
	int length, rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	char *unicode;
	*len = 0;
	*buffer = NULL;
	unicode = locale_to_utf16(&unicode, &length, filename, strlen(filename) + 1);
	if (unicode == NULL)
		return -1;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL) {
		free(unicode);
		return -1;
	}
	hv.bs = unicode;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, length, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_LENGTH) {
				*len = hv.bq4;
				*buffer = malloc(*len);
			}
			if (hi == OBEX_HDR_BODY) {
				memcpy(*buffer, hv.bs, *len);
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	free(unicode);
	return rsp;
}


int exword_remove_file(exword_t *self, char* filename, int convert_to_unicode)
{
	int rsp, length;
	obex_headerdata_t hv;
	char *unicode = NULL;
	length = strlen(filename) + 1;
	if (convert_to_unicode) {
		unicode = locale_to_utf16(&unicode, &length, filename, length);
		if (unicode == NULL)
			return -1;
	}
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		free(unicode);
		return -1;
	}
	hv.bs = Remove;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 16, 0);
	hv.bq4 = length;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = convert_to_unicode ? unicode : filename;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, length, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	free(unicode);
	return rsp;
}

int exword_sd_format(exword_t *self)
{
	int rsp, len;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		return -1;
	}
	hv.bs = SdFormat;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 20, 0);
	hv.bq4 = 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = "";
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 1, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_setpath(exword_t *self, uint8_t *path, uint8_t mkdir)
{
	int len, rsp;
	uint8_t non_hdr[2] = {(mkdir ? 0 : 2), 0x00};
	obex_headerdata_t hv;
	char *unicode;
	unicode = locale_to_utf16(&unicode, &len, path, strlen(path) + 1);
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
		hv.bs = unicode;
	}
	obex_object_set_nonhdr_data(obj, non_hdr, 2);
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, len, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	free(unicode);
	return rsp;
}

int exword_get_model(exword_t *self, exword_model_t * model)
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
				memcpy(model->model, hv.bs, 15);
				memcpy(model->sub_model, hv.bs + 14, 6);
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
				cap->free = ntohl(cap->free);
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_list(exword_t *self, exword_dirent_t **entries, uint16_t *count)
{
	int rsp;
	int i, size;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	*count = 0;
	*entries = NULL;
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
				*entries = malloc(sizeof(exword_dirent_t) * (*count + 1));
				memset(*entries, 0, sizeof(exword_dirent_t) * (*count + 1));
				for (i = 0; i < *count; i++) {
					size = ntohs(*(uint16_t*)hv.bs);
					(*entries)[i].size = size;
					(*entries)[i].flags = hv.bs[2];
					(*entries)[i].name = malloc(size - 3);
					memcpy((*entries)[i].name, hv.bs + 3, size - 3);
					hv.bs += size;
				}
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

void exword_free_list(exword_dirent_t *entries)
{
	int i;
	for (i = 0; entries[i].name != NULL; i++) {
		free(entries[i].name);
	}
	free(entries);
}

int exword_userid(exword_t *self, exword_userid_t id)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return -1;
	hv.bs = UserId;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 16, 0);
	hv.bq4 = 17;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = id.name;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 17, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_cryptkey(exword_t *self, exword_cryptkey_t *key)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return -1;
	hv.bs = CryptKey;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 20, 0);
	hv.bs = key->blk1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_CRYPTKEY, hv, 28, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_BODY) {
				memcpy(key->key, hv.bs, 12);
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_cname(exword_t *self, char *name, char* dir)
{
	int rsp;
	obex_headerdata_t hv;
	int dir_length, name_length;
	char *buffer;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return -1;
	dir_length = strlen(dir) + 1;
	name_length = strlen(name) + 1;
	buffer = malloc(dir_length + name_length);
	if (buffer == NULL)
		return -1;
	memcpy(buffer, dir, dir_length);
	memcpy(buffer + dir_length, name, name_length);
	hv.bs = CName;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 14, 0);
	hv.bq4 = dir_length + name_length;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = buffer;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, dir_length + name_length, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	free(buffer);
	return rsp;
}

int exword_unlock(exword_t *self)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return -1;
	hv.bs = Unlock;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 16, 0);
	hv.bq4 = 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = "";
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 1, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_lock(exword_t *self)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return -1;
	hv.bs = Lock;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 12, 0);
	hv.bq4 = 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = "";
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 1, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_authchallenge(exword_t *self, exword_authchallenge_t challenge)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return -1;
	hv.bs = AuthChallenge;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 30, 0);
	hv.bq4 = 20;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = challenge.challenge;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 20, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return rsp;
}

int exword_authinfo(exword_t *self, exword_authinfo_t *info)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return -1;
	hv.bs = AuthInfo;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 20, 0);
	hv.bs = info->blk1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_AUTHINFO, hv, 40, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_BODY) {
				memcpy(info->challenge, hv.bs, 20);
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
