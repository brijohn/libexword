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
#include "obex.h"

static int obex_bulk_read(obex_t *self, buf_t *msg)
{
	int retval, actual_length;
	int expected_length;
	char * buffer;
	DEBUG(self, 4, "Read from endpoint %d\n", self->read_endpoint_address);
	do {
		buffer = buf_reserve_end(msg, self->mtu_rx);
		retval = libusb_bulk_transfer(self->usb_dev, self->read_endpoint_address, buffer, self->mtu_rx, &actual_length, 1245);
		buf_remove_end(msg, self->mtu_rx - actual_length);
		if (self->seq_check == -1)
			expected_length = ntohs(*((uint16_t*)(msg->data + 1)));
		else
			expected_length = ntohs(*((uint16_t*)(msg->data + 2))) + 1;
	} while ((expected_length != msg->data_size && retval == 0) ||
		 (actual_length == 0 && retval == 0));
	if (retval == 0)
		retval = msg->data_size;
	return retval;
}

static int obex_bulk_write(obex_t *self, buf_t *msg)
{
	int actual_length, retval;
	DEBUG(self, 4, "Write to endpoint %d\n", self->write_endpoint_address);
	retval = libusb_bulk_transfer(self->usb_dev, self->write_endpoint_address, msg->data, msg->data_size, &actual_length, 1245);
	if (retval == 0)
		retval = actual_length;
	return retval;
}

static int obex_verify_seq(obex_t *self, uint8_t seq) {
	int retval, actual_length = 0, count = 0;
	uint8_t check[1] = {0};
	self->seq_check = -1;
	do {
		retval = libusb_bulk_transfer(self->usb_dev, self->read_endpoint_address, check, 1, &actual_length, 1245);
		count++;
		if (retval == LIBUSB_ERROR_OVERFLOW)
			break;
	} while (count < 100 && actual_length != 1);
	if (retval < 0 || check[0] != seq) {
		if (retval == LIBUSB_ERROR_OVERFLOW || actual_length == 0) {
			self->seq_check = seq;
		} else {
			DEBUG(self, 4, "Sequence mismatch %d != %d\n",
			      seq, check[0]);
			return 0;
		}
	}
	return 1;
}

static int obex_claim_interface(obex_t *ctx)
{
	struct libusb_config_descriptor *config = NULL;
	const struct libusb_interface *intf;
	const struct libusb_interface_descriptor *intf_desc;
	const struct libusb_endpoint_descriptor *ep_desc;
	int ret = 0;
	int i, j, k;
	ret = libusb_get_active_config_descriptor(libusb_get_device(ctx->usb_dev), &config);
	if (ret < 0)
		goto done;
	for (i = 0; i < config->bNumInterfaces; i++) {
		intf = &config->interface[i];
		for (j = 0; j < intf->num_altsetting; j++) {
			intf_desc = &intf->altsetting[j];
			ctx->read_endpoint_address = 0;
			ctx->write_endpoint_address = 0;
			for (k = 0; k < intf_desc->bNumEndpoints; k++) {
				ep_desc = &intf_desc->endpoint[k];
				if ((ep_desc->bmAttributes & 3) == LIBUSB_TRANSFER_TYPE_BULK) {
					if (!ctx->read_endpoint_address &&
					    (ep_desc->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN)
						ctx->read_endpoint_address = ep_desc->bEndpointAddress;
					else if (!ctx->write_endpoint_address &&
						 (ep_desc->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_OUT)
						ctx->write_endpoint_address = ep_desc->bEndpointAddress;
				}
			}
			if (ctx->read_endpoint_address && ctx->write_endpoint_address)
				goto done;
		}
	}
	ret = LIBUSB_ERROR_NOT_FOUND;
done:
	if (ret == 0) {
		ctx->intf_num = intf_desc->bInterfaceNumber;
		ret = libusb_claim_interface(ctx->usb_dev, ctx->intf_num);
		if (ret == 0) {
			ret = libusb_set_interface_alt_setting(ctx->usb_dev, ctx->intf_num, intf_desc->bAlternateSetting);
			if (ret < 0)
				libusb_release_interface(ctx->usb_dev, ctx->intf_num);
		}
	}
	if (config != NULL)
		libusb_free_config_descriptor(config);
	return ret;
}

static void free_headerq(struct list_head *list)
{
	struct obex_header_element *h;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, list){
		h = list_entry(pos, struct obex_header_element, link);
		list_del(pos);
		buf_free(h->buf);
		free(h);
	}
}

static int send_body(obex_object_t *object,
		     struct obex_header_element *h,
		     buf_t *txmsg, unsigned int tx_left)
{
	struct obex_byte_stream_hdr *body_txh;
	unsigned int actual;

	body_txh = (struct obex_byte_stream_hdr*) buf_reserve_end(txmsg, sizeof(struct obex_byte_stream_hdr));

	if (!h->body_touched) {
		/* This is the first time we try to send this header
		   obex_object_addheaders has added a struct_byte_stream_hdr
		   before the actual body-data. We shall send this in every fragment
		   so we just remove it for now.*/

		buf_remove_begin(h->buf,  sizeof(struct obex_byte_stream_hdr) );
		h->body_touched = 1;
	}

	if (tx_left < ( h->buf->data_size +
			sizeof(struct obex_byte_stream_hdr) ) )	{
		DEBUG(object->context, 4, "Add BODY header\n");
		body_txh->hi = OBEX_HDR_BODY;
		body_txh->hl = htons((uint16_t)tx_left);

		buf_insert_end(txmsg, h->buf->data, tx_left
				- sizeof(struct obex_byte_stream_hdr) );

		buf_remove_begin(h->buf, tx_left
				- sizeof(struct obex_byte_stream_hdr) );
		/* We have completely filled the tx-buffer */
		actual = tx_left;
	} else {
		DEBUG(object->context, 4, "Add BODY_END header\n");

		body_txh->hi = OBEX_HDR_BODY_END;
		body_txh->hl = htons((uint16_t) (h->buf->data_size + sizeof(struct obex_byte_stream_hdr)));
		buf_insert_end(txmsg, h->buf->data, h->buf->data_size);
		actual = h->buf->data_size;

		list_del(&h->link);
		buf_free(h->buf);
		free(h);
	}

	return actual;
}

static int obex_object_receive_body(obex_object_t *object, buf_t *msg, uint8_t hi,
				uint8_t *source, unsigned int len)
{
	struct obex_header_element *element;

	DEBUG(object->context, 4, "This is a body-header. Len=%d\n", len);

	if (len > msg->data_size) {
		DEBUG(object->context, 1, "Header %d to big. HSize=%d Buffer=%d\n",
				hi, len, msg->data_size);
		return -1;
	}

	if (!object->rx_body) {
		int alloclen = OBEX_OBJECT_ALLOCATIONTRESHOLD + len;

		if (object->hinted_body_len)
			alloclen = object->hinted_body_len;

		DEBUG(object->context, 4, "Allocating new body-buffer. Len=%d\n", alloclen);
		if (!(object->rx_body = buf_new(alloclen)))
			return -1;
	}

	/* Reallocate body buffer if needed */
	if (object->rx_body->data_avail + object->rx_body->tail_avail < (int) len) {
		int t;
		DEBUG(object->context, 4, "Buffer too small. Go realloc\n");
		t = buf_total_size(object->rx_body);
		buf_resize(object->rx_body, t + OBEX_OBJECT_ALLOCATIONTRESHOLD + len);
		if (buf_total_size(object->rx_body) != t + OBEX_OBJECT_ALLOCATIONTRESHOLD + len) {
			DEBUG(object->context, 1, "Can't realloc rx_body\n");
			return -1;
		}
	}

	buf_insert_end(object->rx_body, source, len);

	if (hi == OBEX_HDR_BODY_END) {
		DEBUG(object->context, 4, "Body receive done\n");
		if ( (element = malloc(sizeof(struct obex_header_element)) ) ) {
			memset(element, 0, sizeof(struct obex_header_element));
			element->length = object->rx_body->data_size;
			element->hi = OBEX_HDR_BODY;
			element->buf = object->rx_body;

			/* Add element to rx-list */
			list_add_tail(&element->link, &object->rx_headerq);
		} else
			buf_free(object->rx_body);

		object->rx_body = NULL;
	} else
		DEBUG(object->context, 4, "Normal body fragment...\n");

	return 1;
}

static int obex_object_send(obex_t *self, obex_object_t *object)
{
	struct obex_header_element *h;
	struct obex_common_hdr *hdr;
	buf_t *txmsg;
	int actual, finished = 0;
	uint16_t tx_left;
	int addmore = 1;
	int real_opcode;
	char check[1];

	tx_left = self->mtu_tx - sizeof(struct obex_common_hdr);
	/* Reuse transmit buffer */
	txmsg = buf_reuse(self->tx_msg);

	/* Add nonheader-data first if any (SETPATH, CONNECT)*/
	if (object->tx_nonhdr_data) {
		DEBUG(self, 4, "Adding %d bytes of non-headerdata\n", object->tx_nonhdr_data->data_size);
		buf_insert_end(txmsg, object->tx_nonhdr_data->data, object->tx_nonhdr_data->data_size);

		buf_free(object->tx_nonhdr_data);
		object->tx_nonhdr_data = NULL;
	}

	/* Take headers from the tx queue and try to stuff as
	   many as possible into the tx-msg */
	while (addmore == 1 && !list_empty(&object->tx_headerq)) {

		h = list_entry(object->tx_headerq.next, struct obex_header_element, link);


		if (h->hi == OBEX_HDR_BODY) {
			/* The body may be fragmented over several packets. */
			tx_left -= send_body(object, h, txmsg, tx_left);
		} else if(h->hi == OBEX_HDR_EMPTY) {
			list_del(&h->link);
			free(h);
		} else if (h->length <= tx_left) {
			/* There is room for more data in tx msg */
			DEBUG(self, 4, "Adding non-body header\n");
			buf_insert_end(txmsg, h->buf->data, h->length);
			tx_left -= h->length;
			/* Remove from tx-queue */
			list_del(&h->link);
			buf_free(h->buf);
			free(h);
		} else if (h->length > self->mtu_tx) {
			/* Header is bigger than MTU. This should not happen,
			   because OBEX_ObjectAddHeader() rejects headers
			   bigger than the MTU */
			DEBUG(self, 0, "ERROR! header to big for MTU\n");
			return -1;
		} else {
			/* This header won't fit. */
			addmore = 0;
		}

		if (tx_left == 0)
			addmore = 0;
	};

	
	/* Decide which command to use, and if to use final-bit */
	if (!list_empty(&object->tx_headerq)) {
		real_opcode = object->opcode;
		finished = 0;
	} else {
		real_opcode = object->lastopcode | OBEX_FINAL;
		finished = 1;
	}

	DEBUG(self, 4, "Sending package with opcode %d\n", real_opcode);


	/* Insert common header */
	hdr = (struct obex_common_hdr *) buf_reserve_begin(txmsg, sizeof(struct obex_common_hdr));

	hdr->seq = self->seq_num++;
	hdr->opcode = real_opcode;
	hdr->len = htons((uint16_t)txmsg->data_size - 1);

	DUMPBUFFER(self, "Tx", txmsg);
	DEBUG(self, 1, "len = %d bytes\n", txmsg->data_size);

	actual = obex_bulk_write(self, txmsg);
	if (actual < 0) {
		return actual;
	} else {
		if (!obex_verify_seq(self, hdr->seq))
			return -1;
		return finished;

	}
}

int obex_object_receive(obex_t *self, obex_object_t *object)
{
	struct obex_rsp_hdr *hdr;
	struct obex_header_element *element;
	struct obex_connect_hdr *conn_hdr;
	struct obex_unicode_hdr *unicode;
	struct obex_uint_hdr *uint;
	buf_t *msg;
	int ret;
	int length;
	int version, mtu;
	uint8_t *source = NULL;
	unsigned int len, hlen;
	uint8_t hi;
	int err = 0;

	msg = self->rx_msg;
	ret = obex_bulk_read(self, msg);
	if (ret < 0) {
		return ret;
	}
	if (self->seq_check >= 0) {
		DEBUG(self, 4, "Delayed Sequence checking\n");
		if (msg->data[0] != self->seq_check) {
			DEBUG(self, 4, "Sequence mismatch %d != %d\n",
			      self->seq_check, msg->data[0]);
			return -1;
		}
		buf_remove_begin(msg, 1);
	}
	hdr = (struct obex_rsp_hdr *) msg->data;
	/* New data has been inserted at the end of message */
	DEBUG(self, 4, "Got %d bytes msg len=%d\n", ret, msg->data_size);

	/*
	 * Make sure that the buffer we have, actually has the specified
	 * number of bytes. If not the frame may have been fragmented, and
	 * we will then need to read more from the socket.  
	 */
	length = ntohs(hdr->len);
	/* Make sure we have a whole packet. Should not happen due to reading the entire MTU in*/
	if (length > msg->data_size) {
		DEBUG(self, 3, "Need more data, size=%d, len=%d!\n", length, msg->data_size);
		return msg->data_size;
	}
	DUMPBUFFER(self, "Rx", msg);
	/* Response of a CMD_CONNECT needs some special treatment.*/
	if (object->opcode == OBEX_CMD_CONNECT) {
		DEBUG(self, 2, "We expect a connect-rsp\n");
		if (hdr->rsp == (OBEX_RSP_SUCCESS | OBEX_FINAL)) {
			if (msg->data_size >= 9) {
				conn_hdr = (struct obex_connect_hdr *) ((msg->data) + 3);
				version = conn_hdr->version;
				mtu     = ntohs(conn_hdr->mtu);

				DEBUG(self, 1, "version=%02x\n", version);

				/* Limit to some reasonable value (usually OBEX_DEFAULT_MTU) */
				self->mtu_rx = OBEX_DEFAULT_MTU;
				self->mtu_tx = 0x4006;

				DEBUG(self, 1, "requested MTU=%02x, used MTU=%02x\n", mtu, self->mtu_tx);
			} else {
				DEBUG(self, 1, "Malformed connect-header received\n");
				return -1;
			}
		}
		object->headeroffset = 8;
	}
	/* So does CMD_DISCONNECT */
	if (object->opcode == OBEX_CMD_DISCONNECT) {
		DEBUG(self, 2, "CMD_DISCONNECT done. Resetting MTU!\n");
		self->mtu_rx = self->mtu_tx = OBEX_DEFAULT_MTU;
	}

	/* Remove command from buffer */
	buf_remove_begin(msg, sizeof(struct obex_rsp_hdr));

	/* Copy any non-header data (like in CONNECT and SETPATH) */
	if (object->headeroffset) {
		object->rx_nonhdr_data = buf_new(object->headeroffset);
		if (!object->rx_nonhdr_data)
			return -1;
		buf_insert_end(object->rx_nonhdr_data, msg->data, object->headeroffset);
		DEBUG(self, 4, "Command has %d bytes non-headerdata\n", object->rx_nonhdr_data->data_size);
		buf_remove_begin(msg, object->headeroffset);
		object->headeroffset = 0;
	}

	while ((msg->data_size > 0) && (!err)) {
		hi = msg->data[0];
		DEBUG(self, 4, "Header: %02x\n", hi);
		switch (hi & OBEX_HDR_TYPE_MASK) {
		case OBEX_HDR_TYPE_BYTES:
		case OBEX_HDR_TYPE_UNICODE:
			unicode = (struct obex_unicode_hdr *) msg->data;
			source = &msg->data[3];
			hlen = ntohs(unicode->hl);
			len = hlen - 3;
			if (hi == OBEX_HDR_BODY || hi == OBEX_HDR_BODY_END) {
				/* The body-header need special treatment */
				if (obex_object_receive_body(object, msg, hi, source, len) < 0)
						err = -1;
				/* We have already handled this data! */
				source = NULL;
			}
			break;

		case OBEX_HDR_TYPE_UINT8:
			source = &msg->data[1];
			len = 1;
			hlen = 2;
			break;

		case OBEX_HDR_TYPE_UINT32:
			source = &msg->data[1];
			len = 4;
			hlen = 5;
			break;
		default:
			DEBUG(self, 1, "Badly formed header received\n");
			source = NULL;
			hlen = 0;
			len = 0;
			err = -1;
			break;
		}

		/* Make sure that the msg is big enough for header */
		if (len > msg->data_size) {
			DEBUG(self, 1, "Header %d to big. HSize=%d Buffer=%d\n",
					hi, len, msg->data_size);
			source = NULL;
			err = -1;
		}

		if (source) {
			/* The length MAY be useful when receiving body. */
			if (hi == OBEX_HDR_LENGTH) {
				uint = (struct obex_uint_hdr *) msg->data;
				object->hinted_body_len = ntohl(uint->hv);
				DEBUG(self, 4, "Hinted body len is %d\n",
							object->hinted_body_len);
			}

			if ( (element = malloc(sizeof(struct obex_header_element)) ) ) {
				memset(element, 0, sizeof(struct obex_header_element));
				element->length = len;
				element->hi = hi;

				/* If we get an emtpy we have to deal with it...
				 * This might not be an optimal way, but it works. */
				if (len == 0) {
					DEBUG(self, 4, "Got empty header. Allocating dummy buffer anyway\n");
					element->buf = buf_new(1);
				} else {
					element->buf = buf_new(len);
					if (element->buf) {
						DEBUG(self, 4, "Copying %d bytes\n", len);
						buf_insert_end(element->buf, source, len);
					}
				}

				if (element->buf) {
					/* Add element to rx-list */
					list_add_tail(&element->link, &object->rx_headerq);
				} else{
					DEBUG(self, 1, "Cannot allocate memory\n");
					free(element);
					err = -1;
				}
			} else {
				DEBUG(self, 1, "Cannot allocate memory\n");
				err = -1;
			}
		}

		if (err)
			return err;

		DEBUG(self, 4, "Pulling %d bytes\n", hlen);
		buf_remove_begin(msg, hlen);
	}
	return hdr->rsp & ~OBEX_FINAL;
}

obex_t * obex_init(uint16_t vid, uint16_t pid)
{
	obex_t *self;
	int size;
	self = malloc(sizeof(obex_t));
	if (self == NULL)
		return NULL;
	memset(self, 0, sizeof(obex_t));

	if (libusb_init(&self->usb_ctx) < 0)
		goto out_err;

	self->usb_dev = libusb_open_device_with_vid_pid(self->usb_ctx, vid, pid);
	if (self->usb_dev == NULL)
		goto out_err;

	if (obex_claim_interface(self) < 0)
		goto out_err;

	self->seq_num = 0;
	self->debug = 0;
	self->version = OBEX_VERSION;
	self->locale = 0x00;
	self->mtu_rx = OBEX_DEFAULT_MTU;
	self->mtu_tx = OBEX_DEFAULT_MTU;
	self->mtu_tx_max = OBEX_MAXIMUM_MTU;

	self->rx_msg = buf_new(self->mtu_rx);
	if (self->rx_msg == NULL)
		goto out_err;

	self->tx_msg = buf_new(self->mtu_tx_max);
	if (self->tx_msg == NULL)
		goto out_err;

	return self;

out_err:
	if (self->tx_msg != NULL)
		buf_free(self->tx_msg);
	if (self->rx_msg != NULL)
		buf_free(self->rx_msg);
	if (self->usb_dev)
		libusb_close(self->usb_dev);
	if (self->usb_ctx)
		libusb_exit(self->usb_ctx);
	free(self);
	return NULL;
}

void obex_cleanup(obex_t *self)
{
	if (self) {
		if (self->tx_msg)
			buf_free(self->tx_msg);

		if (self->rx_msg)
			buf_free(self->rx_msg);

		libusb_release_interface(self->usb_dev, self->intf_num);
		libusb_close(self->usb_dev);
		libusb_exit(self->usb_ctx);
		free(self);
	}
}

void obex_set_connect_info(obex_t *self, uint8_t ver, uint8_t locale)
{
	self->version = ver;
	self->locale  = locale;
}

void obex_register_callback(obex_t *self, obex_callback cb, void *userdata)
{
	self->callback = cb;
	self->cb_userdata = userdata;
}

obex_object_t * obex_object_new(obex_t *self, uint8_t cmd)
{
	obex_object_t *object;

	object =  malloc(sizeof(obex_object_t));
	if (object == NULL)
		return NULL;

	memset(object, 0, sizeof(obex_object_t));

	object->context = self;

	INIT_LIST_HEAD(&object->tx_headerq);
	INIT_LIST_HEAD(&object->rx_headerq);
	INIT_LIST_HEAD(&object->rx_headerq_rm);

	object->cmd = cmd;
	object->opcode = cmd;
	object->lastopcode = cmd | OBEX_FINAL;

	/* Need some special woodoo magic on connect-frame */
	if (cmd == OBEX_CMD_CONNECT) {
		struct obex_connect_hdr *conn_hdr;

		object->tx_nonhdr_data = buf_new(7);
		if (!object->tx_nonhdr_data) {
			obex_object_delete(self, object);
			object = NULL;
		} else {
			conn_hdr = (struct obex_connect_hdr *) buf_reserve_end(object->tx_nonhdr_data, 7);
			conn_hdr->version = self->version;
			conn_hdr->flags = 0x40;              /* Flags */
			conn_hdr->mtu = htons(self->mtu_rx); /* Max packet size */
			memcpy(conn_hdr->unknown, "\x40\x00", 2); //unkown data sent during connect
			conn_hdr->locale = self->locale;
		}
	}
	return object;
}

int obex_object_delete(obex_t *self, obex_object_t *object)
{
	/* Free the headerqueues */
	free_headerq(&object->tx_headerq);
	free_headerq(&object->rx_headerq);
	free_headerq(&object->rx_headerq_rm);

	/* Free tx and rx msgs */
	buf_free(object->tx_nonhdr_data);
	object->tx_nonhdr_data = NULL;

	buf_free(object->rx_nonhdr_data);
	object->rx_nonhdr_data = NULL;

	buf_free(object->rx_body);
	object->rx_body = NULL;

	free(object);

	return 0;
}

int obex_object_addheader(obex_t *self, obex_object_t *object, uint8_t hi,
	obex_headerdata_t hv, uint32_t hv_size, unsigned int flags)
{
	int ret = -1;
	struct obex_header_element *element;
	unsigned int maxlen;

	if (flags & OBEX_FL_FIT_ONE_PACKET) {
		/* In this command all headers must fit in one packet! */
		DEBUG(self, 3, "Fit one packet!\n");
		maxlen = self->mtu_tx - object->totallen - sizeof(struct obex_common_hdr);
	} else {
		maxlen = self->mtu_tx - sizeof(struct obex_common_hdr);
	}

	element = malloc(sizeof(struct obex_header_element));
	if (element == NULL)
		return -1;

	memset(element, 0, sizeof(struct obex_header_element));

	element->hi = hi;
	element->flags = flags;

	if (hi == OBEX_HDR_EMPTY) {
		DEBUG(self, 2, "Empty header\n");
		list_add_tail(&element->link, &object->tx_headerq);
		return 1;
	}

	switch (hi & OBEX_HDR_TYPE_MASK) {
	case OBEX_HDR_TYPE_UINT32:
		DEBUG(self, 2, "4BQ header %d\n", hv.bq4);

		element->buf = buf_new(sizeof(struct obex_uint_hdr));
		if (element->buf) {
			struct obex_uint_hdr *hdr;
			ret = element->length = (unsigned int) sizeof(struct obex_uint_hdr);
			hdr = (struct obex_uint_hdr *) buf_reserve_end(element->buf, sizeof(struct obex_uint_hdr));
			hdr->hi = hi;
			hdr->hv = htonl(hv.bq4);
		}
		break;

	case OBEX_HDR_TYPE_UINT8:
		DEBUG(self, 2, "1BQ header %d\n", hv.bq1);

		element->buf = buf_new(sizeof(struct obex_ubyte_hdr));
		if (element->buf) {
			struct obex_ubyte_hdr *hdr;
			ret = element->length = sizeof(struct obex_ubyte_hdr);
			hdr = (struct obex_ubyte_hdr *) buf_reserve_end(element->buf, sizeof(struct obex_ubyte_hdr));
			hdr->hi = hi;
			hdr->hv = hv.bq1;
		}
		break;

	case OBEX_HDR_TYPE_BYTES:
	case OBEX_HDR_TYPE_UNICODE:
		DEBUG(self, 2, "BS/Unicode header size %d\n", hv_size);

		element->buf = buf_new(hv_size + sizeof(struct obex_unicode_hdr));
		if (element->buf) {
			struct obex_unicode_hdr *hdr;
			ret = element->length = hv_size + sizeof(struct obex_unicode_hdr);
			hdr = (struct obex_unicode_hdr *) buf_reserve_end(element->buf, hv_size + sizeof(struct obex_unicode_hdr));
			hdr->hi = hi;
			hdr->hl = htons((uint16_t)(hv_size + sizeof(struct obex_unicode_hdr)));
			memcpy(hdr->hv, hv.bs, hv_size);
			hi++;
		}
		break;

	default:
		DEBUG(self, 2, "Unsupported encoding %02x\n", hi & OBEX_HDR_TYPE_MASK);
		ret = -1;
		break;
	}

	/* Check if you can send this header without violating MTU or OBEX_FIT_ONE_PACKET */
	if (element->hi != OBEX_HDR_BODY || (flags & OBEX_FL_FIT_ONE_PACKET)) {
		if (maxlen < element->length) {
			DEBUG(self, 0, "Header to big\n");
			ret = -1;
		}
	}

	if (ret > 0) {
		object->totallen += ret;
		list_add_tail(&element->link, &object->tx_headerq);
		ret = 1;
	} else {
		buf_free(element->buf);
		free(element);
	}

	return ret;
}

int obex_object_getnextheader(obex_t *self, obex_object_t *object, uint8_t *hi,
			      obex_headerdata_t *hv, uint32_t *hv_size)
{
	uint32_t *bq4;
	struct obex_header_element *h;

	/* No more headers */
	if (list_empty(&object->rx_headerq))
		return 0;

	/* New headers are appended at the end of the list while receiving, so
	   we pull them from the front.
	   Since we cannot free the mem used just yet just put the header in
	   another list so we can free it when the object is deleted. */
	h = list_entry(object->rx_headerq.next, struct obex_header_element, link);
	list_move_tail(object->rx_headerq.next, &(object->rx_headerq_rm));

	*hi = h->hi;
	*hv_size= h->length;

	switch (h->hi & OBEX_HDR_TYPE_MASK) {
		case OBEX_HDR_TYPE_BYTES:
		case OBEX_HDR_TYPE_UNICODE:
			hv->bs = &h->buf->data[0];
			break;

		case OBEX_HDR_TYPE_UINT32:
			bq4 = (uint32_t*) h->buf->data;
			hv->bq4 = ntohl(*bq4);
			break;

		case OBEX_HDR_TYPE_UINT8:
			hv->bq1 = h->buf->data[0];
			break;
	}

	return 1;
}

int obex_object_set_nonhdr_data(obex_object_t *object, const uint8_t *buffer, unsigned int len)
{
	/* TODO: Check that we actually can send len bytes without violating MTU */

	if (object->tx_nonhdr_data)
		return -1;

	object->tx_nonhdr_data = buf_new(len);
	if (object->tx_nonhdr_data == NULL)
		return -1;

	buf_insert_end(object->tx_nonhdr_data, (uint8_t *)buffer, len);

	return 1;
}

int obex_request(obex_t *self, obex_object_t *object)
{
	int ret, rsp;
	do {
		ret = obex_object_send(self, object);
		if (ret < 0)
			return ret;
		rsp = obex_object_receive(self, object);
		if (self->callback)
			self->callback(self, object, self->cb_userdata);
	} while (rsp == OBEX_RSP_CONTINUE);
	return rsp;
}
