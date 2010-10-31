/* libexword - library for transffering files to Casio-EX-Word dictionaries
 *
 * Most of this code was borrowed and adopted from OpenOBEX to work with
 * the casio EX-word dictionaries
 *
 * Copyright (C) 2010 - Brian Johnson <brijohn@gmail.com>
 * Copyright (c) 1999, 2000 Pontus Fuchs, All Rights Reserved.
 * Copyright (c) 1999, 2000 Dag Brattli, All Rights Reserved.
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
#ifndef OBEX_H
#define OBEX_H

#include <libusb.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "databuffer.h"

extern int debug;

#define log_debug(format, ...) fprintf(stderr, format, ## __VA_ARGS__)
#define log_debug_prefix ""

#  define DEBUG(obex, n, format, ...) \
          if (debug >= (n)) \
            log_debug("%s%s(): " format, log_debug_prefix, __FUNCTION__, ## __VA_ARGS__)

#define DUMPBUFFER(obex, label, msg) \
        if (debug >=5) buf_dump(msg, label);

#define OBEX_OBJECT_ALLOCATIONTRESHOLD 10240

#define OBEX_VERSION		0x20

#define OBEX_FL_FIT_ONE_PACKET	0x01	/* This header must fit in one packet */

#define OBEX_HDR_TYPE_UNICODE	(0 << 6)  /* zero terminated unicode string (network byte order) */
#define OBEX_HDR_TYPE_BYTES	(1 << 6)  /* byte array */
#define OBEX_HDR_TYPE_UINT8	(2 << 6)  /* 8bit unsigned integer */
#define OBEX_HDR_TYPE_UINT32	(3 << 6)  /* 32bit unsigned integer */
#define OBEX_HDR_TYPE_MASK	0xc0

#define OBEX_HDR_ID_COUNT	 0x00	/* Number of objects (used by connect) */
#define OBEX_HDR_ID_NAME	 0x01	/* Name of the object */
#define OBEX_HDR_ID_TYPE	 0x02	/* Type of the object */
#define OBEX_HDR_ID_LENGTH	 0x03	/* Total length of object */
#define OBEX_HDR_ID_TIME	 0x04	/* Last modification time of (ISO8601) */
#define OBEX_HDR_ID_DESCRIPTION	 0x05	/* Description of object */
#define OBEX_HDR_ID_TARGET	 0x06	/* Identifies the target for the object */
#define OBEX_HDR_ID_HTTP	 0x07	/* An HTTP 1.x header */
#define OBEX_HDR_ID_BODY	 0x08	/* Data part of the object */
#define OBEX_HDR_ID_BODY_END	 0x09	/* Last data part of the object */
#define OBEX_HDR_ID_WHO		 0x0a	/* Identifies the sender of the object */
#define OBEX_HDR_ID_CONNECTION	 0x0b	/* Connection identifier */
#define OBEX_HDR_ID_APPARAM	 0x0c	/* Application parameters */
#define OBEX_HDR_ID_AUTHCHAL	 0x0d	/* Authentication challenge */
#define OBEX_HDR_ID_AUTHRESP	 0x0e	/* Authentication response */
#define OBEX_HDR_ID_CREATOR	 0x0f	/* indicates the creator of an object */
#define OBEX_HDR_ID_AUTHINFO	 0x30	/* get authentication info */
#define OBEX_HDR_ID_CRYPTKEY	 0x31	/* file transfer key ?? */
#define OBEX_HDR_ID_MASK	 0x3f

#define OBEX_HDR_EMPTY		0x00	/* Empty header (buggy OBEX servers) */
#define OBEX_HDR_COUNT		(OBEX_HDR_ID_COUNT        | OBEX_HDR_TYPE_UINT32 )
#define OBEX_HDR_NAME		(OBEX_HDR_ID_NAME         | OBEX_HDR_TYPE_UNICODE)
#define OBEX_HDR_TYPE		(OBEX_HDR_ID_TYPE         | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_LENGTH		(OBEX_HDR_ID_LENGTH       | OBEX_HDR_TYPE_UINT32 )
#define OBEX_HDR_TIME		(OBEX_HDR_ID_TIME         | OBEX_HDR_TYPE_BYTES  ) /* Format: ISO 8601 */
#define OBEX_HDR_TIME2		(OBEX_HDR_ID_TIME         | OBEX_HDR_TYPE_UINT32 ) /* Deprecated use HDR_TIME instead */
#define OBEX_HDR_DESCRIPTION	(OBEX_HDR_ID_DESCRIPTION  | OBEX_HDR_TYPE_UNICODE)
#define OBEX_HDR_TARGET		(OBEX_HDR_ID_TARGET       | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_HTTP		(OBEX_HDR_ID_HTTP         | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_BODY		(OBEX_HDR_ID_BODY         | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_BODY_END	(OBEX_HDR_ID_BODY_END     | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_WHO		(OBEX_HDR_ID_WHO          | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_CONNECTION	(OBEX_HDR_ID_CONNECTION   | OBEX_HDR_TYPE_UINT32 )
#define OBEX_HDR_APPARAM	(OBEX_HDR_ID_APPARAM      | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_AUTHCHAL	(OBEX_HDR_ID_AUTHCHAL     | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_AUTHRESP	(OBEX_HDR_ID_AUTHRESP     | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_CREATOR	(OBEX_HDR_ID_CREATOR      | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_CRYPTKEY	(OBEX_HDR_ID_CRYPTKEY     | OBEX_HDR_TYPE_BYTES  )
#define OBEX_HDR_AUTHINFO	(OBEX_HDR_ID_AUTHINFO     | OBEX_HDR_TYPE_BYTES  )

/* Commands */
#define OBEX_CMD_CONNECT	0x00
#define OBEX_CMD_DISCONNECT	0x01
#define OBEX_CMD_PUT		0x02
#define OBEX_CMD_GET		0x03
#define OBEX_CMD_SETPATH	0x05
#define OBEX_FINAL		0x80

/* Responses */
#define	OBEX_RSP_CONTINUE		0x10
#define OBEX_RSP_SWITCH_PRO		0x11
#define OBEX_RSP_SUCCESS		0x20
#define OBEX_RSP_CREATED		0x21
#define OBEX_RSP_ACCEPTED		0x22
#define OBEX_RSP_NON_AUTHORITATIVE	0x23
#define OBEX_RSP_NO_CONTENT		0x24
#define OBEX_RSP_RESET_CONTENT		0x25
#define OBEX_RSP_PARTIAL_CONTENT        0x26
#define OBEX_RSP_MULTIPLE_CHOICES	0x30
#define OBEX_RSP_MOVED_PERMANENTLY	0x31
#define OBEX_RSP_MOVED_TEMPORARILY	0x32
#define OBEX_RSP_SEE_OTHER		0x33
#define OBEX_RSP_NOT_MODIFIED		0x34
#define OBEX_RSP_USE_PROXY		0x35
#define OBEX_RSP_BAD_REQUEST		0x40
#define OBEX_RSP_UNAUTHORIZED		0x41
#define OBEX_RSP_PAYMENT_REQUIRED	0x42
#define OBEX_RSP_FORBIDDEN		0x43
#define OBEX_RSP_NOT_FOUND		0x44
#define OBEX_RSP_METHOD_NOT_ALLOWED	0x45
#define OBEX_RSP_NOT_ACCEPTABLE		0x46
#define OBEX_RSP_PROXY_AUTH_REQUIRED	0x47
#define OBEX_RSP_REQUEST_TIME_OUT	0x48
#define OBEX_RSP_CONFLICT		0x49
#define OBEX_RSP_GONE			0x4a
#define OBEX_RSP_LENGTH_REQUIRED	0x4b
#define OBEX_RSP_PRECONDITION_FAILED	0x4c
#define OBEX_RSP_REQ_ENTITY_TOO_LARGE	0x4d
#define OBEX_RSP_REQ_URL_TOO_LARGE	0x4e
#define OBEX_RSP_UNSUPPORTED_MEDIA_TYPE	0x4f
#define OBEX_RSP_INTERNAL_SERVER_ERROR	0x50
#define OBEX_RSP_NOT_IMPLEMENTED	0x51
#define OBEX_RSP_BAD_GATEWAY		0x52
#define OBEX_RSP_SERVICE_UNAVAILABLE	0x53
#define OBEX_RSP_GATEWAY_TIMEOUT	0x54
#define OBEX_RSP_VERSION_NOT_SUPPORTED	0x55
#define OBEX_RSP_DATABASE_FULL		0x60
#define OBEX_RSP_DATABASE_LOCKED	0x61

/* Min, Max and default transport MTU */
#define OBEX_DEFAULT_MTU	4096
#define OBEX_MINIMUM_MTU	255
#define OBEX_MAXIMUM_MTU	65535

typedef union {
	uint32_t bq4;
	uint8_t bq1;
	const uint8_t *bs;
} obex_headerdata_t;

typedef struct {
	struct libusb_context *usb_ctx;
	struct libusb_device_handle *usb_dev;
	uint8_t intf_num;
	uint8_t read_endpoint_address;
	uint8_t write_endpoint_address;
	uint16_t mtu_rx;
	uint16_t mtu_tx;
	uint16_t mtu_tx_max;
	buf_t *tx_msg;
	buf_t *rx_msg;
	int debug;
	uint8_t seq_num;
} obex_t;

#pragma pack(1)
struct obex_common_hdr {
	uint8_t  seq;
	uint8_t  opcode;
	uint16_t len;
};
#pragma pack()

#pragma pack(1)
struct obex_rsp_hdr {
	uint8_t  rsp;
	uint16_t len;
};
#pragma pack()

#pragma pack(1)
struct obex_connect_hdr {
	uint8_t  version;
	uint8_t  flags;
	uint16_t mtu;
	uint8_t  unknown[3];
};
#pragma pack()

#pragma pack(1)
struct obex_uint_hdr {
	uint8_t  hi;
	uint32_t hv;
};
#pragma pack()

#pragma pack(1)
struct obex_ubyte_hdr {
	uint8_t hi;
	uint8_t hv;
};
#pragma pack()

#pragma pack(1)
struct obex_unicode_hdr {
	uint8_t  hi;
	uint16_t hl;
	uint8_t  hv[0];
};
#pragma pack()

#define obex_byte_stream_hdr obex_unicode_hdr

struct obex_header_element {
	buf_t *buf;
	uint8_t hi;
	unsigned int flags;
	unsigned int length;
	unsigned int offset;
	int body_touched;
	struct list_head link;
};

typedef struct {
	time_t time;

	struct list_head tx_headerq;		/* List of headers to transmit*/
	struct list_head rx_headerq;		/* List of received headers */
	struct list_head rx_headerq_rm;		/* List of recieved header already read by the app */
	buf_t *rx_body;		/* The rx body header need some extra help */
	buf_t *tx_nonhdr_data;	/* Data before of headers (like CONNECT and SETPATH) */
	buf_t *rx_nonhdr_data;	/* -||- */

	uint8_t cmd;			/* The command of this object */

					/* The opcode fields are used as
					   command when sending and response
					   when recieving */

	uint8_t opcode;			/* Opcode for normal packets */
	uint8_t lastopcode;		/* Opcode for last packet */
	unsigned int headeroffset;	/* Where to start parsing headers */

	int hinted_body_len;		/* Hinted body-length or 0 */
	int totallen;			/* Size of all headers */

	int continue_received;		/* CONTINUE received after sending last command */

} obex_object_t;

obex_t * obex_init(uint16_t vid, uint16_t pid);
void obex_cleanup(obex_t *self);
obex_object_t * obex_object_new(obex_t *self, uint8_t cmd);
int obex_object_delete(obex_t *self, obex_object_t *object);
int obex_object_add_header(obex_t *self, obex_object_t *object,
			   uint8_t hi, obex_headerdata_t hv, uint32_t hv_size,
			   unsigned int flags);
int obex_object_getnextheader(obex_t *self, obex_object_t *object,
			      uint8_t *hi, obex_headerdata_t *hv, uint32_t *hv_size);
int obex_object_set_nonhdr_data(obex_object_t *object, const uint8_t *buffer, unsigned int len);
int obex_request(obex_t *self, obex_object_t *object);

#endif
