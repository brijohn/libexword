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

/**
 * @page Protocol
 * @verbinclude protocol.txt
 *
 * @mainpage libexword API Reference
 *
 * @section intro Introduction
 *
 * libexword is an open source library that allows communication with CASIO
 * Exword Dictionaries. The current version of the source code is available
 * at <a href="https://github.com/brijohn/libexword">libexword homepage</a>.
 *
 */

/** @defgroup device Device handling
 * This page details the functions used for opening and closing devices.
 */

/** @defgroup encoding Character encoding
 * This page details functions used for converting between different character encodings.
 */

/** @defgroup misc Miscellaneous
 */

/** @defgroup cmd Device commands
 * This page details the functions used to send commands to the device.
 */

static const char Model[] = {0,'_',0,'M',0,'o',0,'d',0,'e',0,'l',0,0};
static const char List[] = {0,'_',0,'L',0,'i',0,'s',0,'t',0,0};
static const char Remove[] = {0,'_',0,'R',0,'e',0,'m',0,'o',0,'v',0,'e',0,0};
static const char Cap[] = {0,'_',0,'C',0,'a',0,'p',0,0};
static const char SdFormat[] = {0,'_',0,'S',0,'d',0,'F',0,'o',0,'r',0,'m',0,'a',0,'t',0,0};
static const char UserId[] = {0,'_',0,'U',0,'s',0,'e',0,'r',0,'I',0,'d',0,0};
static const char Unlock[] = {0,'_',0,'U',0,'n',0,'l',0,'o',0,'c',0,'k',0,0};
static const char Lock[] = {0,'_',0, 'L',0,'o',0,'c',0,'k',0,0};
static const char CName[] = {0,'_',0,'C',0,'N',0,'a',0,'m',0,'e',0,0};
static const char CryptKey[] = {0,'_',0,'C',0,'r',0,'y',0,'p',0,'t',0,'K',0,'e',0,'y',0,0};
static const char AuthChallenge[] = {0,'_',0,'A', 0, 'u', 0, 't', 0, 'h', 0, 'C', 0, 'h', 0, 'a', 0, 'l', 0, 'l', 0, 'e', 0, 'n', 0, 'g', 0, 'e', 0, 0};
static const char AuthInfo[] = {0,'_',0,'A', 0, 'u', 0, 't', 0, 'h', 0, 'I', 0, 'n', 0, 'f', 0, 'o', 0, 0};


/** @ingroup device
 * @typedef struct exword_t exword_t
 * Structure representing an exword device handle.
 */
/// @cond exclude
struct exword_t {
	obex_t *obex_ctx;
	uint16_t vid;
	uint16_t pid;
	char manufacturer[20];
	char product[20];

	int internal_error;

	file_cb put_file_cb;
	file_cb get_file_cb;
	void * cb_userdata;
	char * cb_filename;
	uint32_t cb_filelength;
	uint32_t cb_transferred;

	disconnect_cb disconnect_callback;
	void * disconnect_data;
	struct libusb_transfer *int_urb;
	char int_buffer[16];
};
/// @endcond


static int obex_to_exword_error(exword_t *self, int obex_rsp)
{
	self->internal_error = 0;
	switch(obex_rsp) {
	case OBEX_RSP_SUCCESS:
		return EXWORD_SUCCESS;
	case OBEX_RSP_FORBIDDEN:
		return EXWORD_ERROR_FORBIDDEN;
	case OBEX_RSP_NOT_FOUND:
		return EXWORD_ERROR_NOT_FOUND;
	case OBEX_RSP_INTERNAL_SERVER_ERROR:
		/* Send disconnect on Internal Errors.
		 * No commands will work after receiving an internal error
		 * and the DP3 series autodisconnect anyways. Explicitly
		 * calling exword_disconnect is done since DP5s (and maybe DP4s)
		 * do not autodisconnect.
		 */
		self->internal_error = 1;
		exword_disconnect(self);
		return EXWORD_ERROR_INTERNAL;
	default:
		return EXWORD_ERROR_OTHER;
	}
}

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

	if (!(output = malloc(outlen)))
		return NULL;

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
	if (dst != NULL)
		*dst = output;
	if (dstsz != NULL)
		*dstsz = (outbuf - output);
	return output;
}

/** @ingroup encoding
 * Convert string to current locale.
 * This function will convert a string from the specified format to
 * the current locale.
 * @note The destination string is allocated by the function and must
 * be freed by the user.
 * @param[in] fmt encoding format to convert from
 * @param[out] dst destination string
 * @param[out] dstsz size of destination string
 * @param[in] src source string
 * @param[in] srcsz size of source string
 * @returns converted string
 */
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

/** @ingroup encoding
 * Convert string from current locale.
 * This function will convert a string from the current locale to
 * the the specified format.
 * @note The destination string is allocated by the function and must
 * be freed by the user.
 * @param[in] fmt encoding format to convert to
 * @param[out] dst destination string
 * @param[out] dstsz size of destination string
 * @param[in] src source string
 * @param[in] srcsz size of source string
 * @returns converted string
 */
char * convert_from_locale(char *fmt, char **dst, int *dstsz, const char *src, int srcsz)
{
	iconv_t cd;
	*dst = NULL;
	*dstsz = 0;
	cd = iconv_open(fmt, "");
	if (cd == (iconv_t) -1)
		return NULL;
	*dst = convert(cd, dst, dstsz, src, srcsz);
	iconv_close(cd);
	return *dst;
}

static int is_cmd(char *data, int length)
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

void send_disconnect_event(exword_t *self, int reason) {
	if (self->disconnect_callback) {
		self->disconnect_callback(reason, self->disconnect_data);
	}
}

void exword_handle_interrupt(struct libusb_transfer *transfer)
{
	exword_t *self = (exword_t *)transfer->user_data;
	switch(transfer->status) {
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_COMPLETED:
		libusb_submit_transfer(transfer);
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_ERROR:
		send_disconnect_event(self, EXWORD_DISCONNECT_UNPLUGGED);
		break;
	case LIBUSB_TRANSFER_CANCELLED:
		if (self->internal_error)
			send_disconnect_event(self, EXWORD_DISCONNECT_ERROR);
		else
			send_disconnect_event(self, EXWORD_DISCONNECT_NORMAL);
		break;
	}
}

static void exword_handle_callbacks(obex_t *self, obex_object_t *object, void *userdata)
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
			convert_to_locale("UTF-16BE", &exword->cb_filename, &len, hdr->hv, ntohs(hdr->hl) - 3);
			if (exword->cb_filename == NULL)
				exword->cb_filename = strdup("Unknown");
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
				convert_to_locale("UTF-16BE", &exword->cb_filename, &len, hdr->hv, ntohs(hdr->hl) - 3);
				if (exword->cb_filename == NULL)
					exword->cb_filename = strdup("Unknown");
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

/** @ingroup device
 * Opens device.
 * This function will open the attached exword device with the default
 * options of \ref EXWORD_MODE_LIBRARY and \ref EXWORD_REGION_JA.
 * @returns pointer to a device handle.
 */
exword_t * exword_open()
{
	return exword_open2(0x0020);
}

/** @ingroup device
 * Opens device.
 * This function will open the attached device using the specified mode
 * and region.
 * @param options bit mask of mode and region
 * @returns pointer to a device handle.
 */
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
	if (options & EXWORD_MODE_TEXT)
		ver = locale;
	else if (options & EXWORD_MODE_CD)
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
	if (ret < 0) {
		libusb_exit(NULL);
		goto error;
	}

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
	libusb_free_device_list(dev_list, 1);
	libusb_exit(NULL);

	if (i >= ret)
		goto error;

	self->int_urb = libusb_alloc_transfer(0);
	if (self->int_urb == NULL)
		goto error;

	self->obex_ctx = obex_init(self->vid, self->pid);
	if (self->obex_ctx == NULL)
		goto error;
	obex_set_connect_info(self->obex_ctx, ver, locale);
	obex_register_callback(self->obex_ctx, exword_handle_callbacks, self);

	libusb_fill_interrupt_transfer(self->int_urb, self->obex_ctx->usb_dev,
				       self->obex_ctx->interrupt_endpoint_address,
				       self->int_buffer, 16, exword_handle_interrupt,
				       self, 3000);

	return self;

error:
	libusb_free_transfer(self->int_urb);
	free(self);
	return NULL;
}

/** @ingroup device
 * Closes device.
 * This function closes the device and performs necessary cleanup.
 * @param self device handle
 */
void exword_close(exword_t *self)
{
	if (self) {
		libusb_free_transfer(self->int_urb);
		obex_cleanup(self->obex_ctx);
		free(self->cb_filename);
		free(self);
	}
}

/** @ingroup misc
 * Sets the debug message level.
 * This function sets the debug level for the currentlt opened device.
 * @param self device handle
 * @param level debug level (0-5)
 */
void exword_set_debug(exword_t *self, int level)
{
	self->obex_ctx->debug = level;
}

/** @ingroup misc
 * Registers callback functions for sending and recieving files.
 * These functions will be invoked during file transfers after each
 * chunk of the file is transferred.\n\n
 * To remove a callback use NULL for function pointer
 * @param self device handle
 * @param get pointer to function for reporting download transfer progress
 * @param put pointer to function for reporting upload transfer progress
 * @param userdata pointer passed to callback functions
 */
void exword_register_transfer_callbacks(exword_t *self, file_cb get, file_cb put, void *userdata)
{
	self->put_file_cb = put;
	self->get_file_cb = get;
	self->cb_userdata = userdata;
}

/** @ingroup device
 * Registers callback function for disconnect notifications.
 * The registered function will be invoked during a disconnect
 * event.\n\n
 * To remove a callback use NULL for function pointer
 * @param self device handle
 * @param disconnect pointer to disconnect notification function
 * @param userdata pointer passed to callback function
 */
void exword_register_disconnect_callback(exword_t *self, disconnect_cb disconnect, void *userdata)
{
	self->disconnect_callback = disconnect;
	self->disconnect_data = userdata;
}

/** @ingroup misc
 * Checks for pending disconnect event.
 * This function should be called periodically from
 * your main loop to check for a disconnect event.\n\n
 * If it is not called no disconnect notification will be
 * sent to the application.
 * @param self device handle
 */
void exword_handle_disconnect_event(exword_t *self)
{
	struct timeval tv = {0, 0};
	libusb_handle_events_timeout(self->obex_ctx->usb_ctx, &tv);
}


/** @ingroup cmd
 * Send connect command.
 * @note Any commands sent before this will fail.
 * @param self device handle
 * @return response code
 */
int exword_connect(exword_t *self)
{
	int rsp;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_CONNECT);
	if (obj == NULL)
		return -1;
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	libusb_submit_transfer(self->int_urb);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Upload a file to device.
 * This command will write the given file data as file filename to the device.
 * @param self device handle
 * @param filename name of file being sent.
 * @param buffer pointer to file data.
 * @param len size of buffer.
 * @return response code
 */
int exword_send_file(exword_t *self, char* filename, char *buffer, int len)
{
	int length, rsp;
	obex_headerdata_t hv;
	char *unicode;
	unicode = convert_from_locale("UTF-16BE", &unicode, &length, filename, strlen(filename) + 1);
	if (unicode == NULL)
		return EXWORD_ERROR_OTHER;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		free(unicode);
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Download a file from device.
 * This command will read a file from the device.
 * @note buffer is allocated by the function and must be freed by the user.
 * @param[in] self device handle
 * @param[in] filename name of file being sent.
 * @param[out] buffer pointer to recieved file data.
 * @param[out] len size of buffer.
 * @return response code
 */
int exword_get_file(exword_t *self, char* filename, char **buffer, int *len)
{
	int length, rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	char *unicode;
	*len = 0;
	*buffer = NULL;
	unicode = convert_from_locale("UTF-16BE", &unicode, &length, filename, strlen(filename) + 1);
	if (unicode == NULL)
		return EXWORD_ERROR_OTHER;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL) {
		free(unicode);
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Remove a file from device.
 * This command will remove the given file from the device.\n\n
 * Dataplus5 models require convert_to_unicode option when operating in Text mode.
 * @param self device handle
 * @param filename name of file being sent
 * @param convert_to_unicode automatically convert filename to UTF-16 if true
 * @return response code
 */
int exword_remove_file(exword_t *self, char* filename, int convert_to_unicode)
{
	int rsp, length;
	obex_headerdata_t hv;
	char *unicode = NULL;
	length = strlen(filename) + 1;
	if (convert_to_unicode) {
		unicode = convert_from_locale("UTF-16BE", &unicode, &length, filename, length);
		if (unicode == NULL)
			return EXWORD_ERROR_OTHER;
	}
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		free(unicode);
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Format SD card.
 * This command will format the inserted SD card.
 * @param self device handle
 * @return response code
 */
int exword_sd_format(exword_t *self)
{
	int rsp, len;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL) {
		return EXWORD_ERROR_NO_MEM;
	}
	hv.bs = SdFormat;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 20, 0);
	hv.bq4 = 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = "";
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 1, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Sets the current path on device.
 * Pathname should start with either \\_INTERNAL_00 or \\_SD_00, which will
 * access either internal memory or the sd card respectively.\n\n
 * Passing an empty string for path will allow you to get a list of storage mediums.
 * @param self device handle
 * @param path new path
 * @param mkdir if true create path if non existant
 * @return response code
 */
int exword_setpath(exword_t *self, uint8_t *path, uint8_t mkdir)
{
	int len, rsp;
	uint8_t non_hdr[2] = {(mkdir ? 0 : 2), 0x00};
	obex_headerdata_t hv;
	char *unicode;
	unicode = convert_from_locale("UTF-16BE", &unicode, &len, path, strlen(path) + 1);
	if (unicode == NULL)
		return EXWORD_ERROR_OTHER;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_SETPATH);
	if (obj == NULL) {
		free(unicode);
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Get model information.
 * This function retrieves the model information of the connected device.
 * @param[in] self device handle
 * @param[out] model model information
 * @return response code
 */
int exword_get_model(exword_t *self, exword_model_t * model)
{
	int rsp;
	obex_headerdata_t hv;
	const uint8_t *ptr;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	hv.bs = Model;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 14, 0);
	rsp = obex_request(self->obex_ctx, obj);
	if ((rsp & ~OBEX_FINAL) == OBEX_RSP_SUCCESS) {
		while (obex_object_getnextheader(self->obex_ctx, obj, &hi, &hv, &hv_size)) {
			if (hi == OBEX_HDR_BODY) {
				model->capabilities = 0;
				memcpy(model->model, hv.bs, 15);
				memcpy(model->sub_model, hv.bs + 14, 6);
				ptr = hv.bs + 23;
				while (ptr < (hv.bs + hv_size)) {
					if (memcmp(ptr, "SW", 2) == 0) {
						model->capabilities |= CAP_SW;
					} else if (memcmp(ptr, "P", 1) == 0) {
						model->capabilities |= CAP_P;
					} else if (memcmp(ptr, "F", 1) == 0) {
						model->capabilities |= CAP_F;
					} else if (memcmp(ptr, "CY", 2) == 0) {
						memcpy(model->ext_model, ptr, 6);
						model->capabilities |= CAP_EXT;
					} else if (memcmp(ptr, "C", 1) == 0) {
						model->capabilities |= CAP_C;
					}
					ptr += strlen(ptr) + 1;
				}
				break;
			}
		}
	}
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Get storage capacity.
 * This function retrieves the storage capacity the the currently selected storage medium.\n
 * The storage medium being accessed is selected using \ref exword_setpath.
 * @param[in] self device handle
 * @param[out] cap capacity
 * @return response code
 */
int exword_get_capacity(exword_t *self, exword_capacity_t *cap)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Get file list.
 * This function will retreive the file list for the currently set path.\n
 * Entries must be freed with \ref exword_free_list.
 * @param[in] self device handle
 * @param[out] entries array of directory entries
 * @param[out] count number of elements in entries array
 * @return response code
 */
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
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Free list of directory entries.
 * @param entries array of directory entries
 * @return response code
 */
void exword_free_list(exword_dirent_t *entries)
{
	int i;
	for (i = 0; entries[i].name != NULL; i++) {
		free(entries[i].name);
	}
	free(entries);
}

/** @ingroup cmd
 * Set userid.
 * This function updates the user_id of connected device.
 * @param self device handle
 * @param id new user_id
 * @return response code
 */
int exword_userid(exword_t *self, exword_userid_t id)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	hv.bs = UserId;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 16, 0);
	hv.bq4 = 17;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = id.name;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 17, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Generate new CryptKey.
 * This function is used to gerneate the CryptKey used for encryptng dictionaries.\n\n
 * key.blk1 and key.blk2 are used for input and the key will be returned in key.key.
 * @param[in] self device handle
 * @param[in,out] key CryptKey info
 * @return response code
 */
int exword_cryptkey(exword_t *self, exword_cryptkey_t *key)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
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
	memcpy(key->key + 12, key->blk2 + 8, 4);
	get_xor_key(key->key, 16, key->xorkey);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Set add-on dictionary name information.
 * This function is used to register the add-on dictionary with device.
 * @param self device handle
 * @param name add-on name
 * @param dir install directory
 * @return response code
 */
int exword_cname(exword_t *self, char *name, char* dir)
{
	int rsp;
	obex_headerdata_t hv;
	int dir_length, name_length;
	char *buffer;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	dir_length = strlen(dir) + 1;
	name_length = strlen(name) + 1;
	buffer = malloc(dir_length + name_length);
	if (buffer == NULL)
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Unlock device.
 * This function needs to be called after dding or removing add-on dictionaries.
 * @param self device handle
 * @return response code
 */
int exword_unlock(exword_t *self)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	hv.bs = Unlock;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 16, 0);
	hv.bq4 = 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = "";
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 1, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Lock device.
 * This function needs to be called prior to adding or removing add-on dictionaries.
 * @param self device handle
 * @return response code
 */
int exword_lock(exword_t *self)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	hv.bs = Lock;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 12, 0);
	hv.bq4 = 1;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = "";
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 1, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Authenticate to device.
 * This function will try to authenticate to the currently connected device.
 * @param self device handle
 * @param challenge 20 byte challenge key
 * @return response code
 */
int exword_authchallenge(exword_t *self, exword_authchallenge_t challenge)
{
	int rsp;
	obex_headerdata_t hv;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_PUT);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	hv.bs = AuthChallenge;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_NAME, hv, 30, 0);
	hv.bq4 = 20;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_LENGTH, hv, 0, 0);
	hv.bs = challenge.challenge;
	obex_object_addheader(self->obex_ctx, obj, OBEX_HDR_BODY, hv, 20, 0);
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Reset authentication info.
 * On return info.challenge will contain the new challenge key for the device.
 * @note Issuing this command causes the device to delete all installed dictionaries.
 * @param[in] self device handle
 * @param[in, out] info authentication info
 * @return response code
 */
int exword_authinfo(exword_t *self, exword_authinfo_t *info)
{
	int rsp;
	obex_headerdata_t hv;
	uint8_t hi;
	uint32_t hv_size;
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_GET);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
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
	return obex_to_exword_error(self, rsp);
}

/** @ingroup cmd
 * Disconnect from device.
 * This function should be the last command sent and will disconnect from the device.
 * @param self device handle
 * @return response code
 */
int exword_disconnect(exword_t *self)
{
	int rsp;
	libusb_cancel_transfer(self->int_urb);
	obex_object_t *obj = obex_object_new(self->obex_ctx, OBEX_CMD_DISCONNECT);
	if (obj == NULL)
		return EXWORD_ERROR_NO_MEM;
	rsp = obex_request(self->obex_ctx, obj);
	obex_object_delete(self->obex_ctx, obj);
	return obex_to_exword_error(self, rsp);
}

/** @ingroup misc
 * Converts error code to string
 * @note return value is a static string and should not be freed.
 * @param code error code
 * @return error message
 */
char *exword_error_to_string(int code)
{
	switch (code) {
	case EXWORD_SUCCESS:
		return "OK, Success";
	case EXWORD_ERROR_FORBIDDEN:
		return "Forbidden";
	case EXWORD_ERROR_NOT_FOUND:
		return "Not found";
	case EXWORD_ERROR_INTERNAL:
		return "Internal server error";
	case EXWORD_ERROR_NO_MEM:
		return "Insufficient memory";
	case EXWORD_ERROR_OTHER:
	default:
		return "Unknown error";
	}
}
