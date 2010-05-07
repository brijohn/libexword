/**
	\file databuffer.c
	Network buffer handling routines.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 2005 Herton Ronaldo Krzesinski, All Rights Reserved.

	OpenOBEX is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with OpenOBEX. If not, see <http://www.gnu.org/>.
 */

#include "databuffer.h"
#include "obex.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


buf_t *buf_new(size_t default_size)
{
	buf_t *p;

	p = malloc(sizeof(buf_t));
	if (!p)
		return NULL;
	p->buffer = malloc(sizeof(uint8_t) * default_size);
	if (!p->buffer) {
		free(p);
		return NULL;
	}
	p->data = p->buffer;
	p->head_avail = 0;
	p->data_avail = default_size;
	p->tail_avail = 0;
	p->data_size = 0;
	return p;
}

size_t buf_total_size(buf_t *p)
{
	if (!p)
		return 0;
	return p->head_avail + p->data_avail + p->tail_avail + p->data_size;
}

void buf_resize(buf_t *p, size_t new_size)
{
	uint8_t *tmp;
	int bSize;

	if (!p)
		return;
	bSize = buf_total_size(p);
	if (new_size < bSize) {
		int itRem = bSize - new_size;
		if (itRem > p->data_avail) {
			itRem -= p->data_avail;
			p->data_avail = 0;
		} else {
			p->data_avail -= itRem;
			itRem = 0;
		}
		if (itRem > p->tail_avail) {
			itRem -= p->tail_avail;
			p->tail_avail = 0;
		} else {
			p->tail_avail -= itRem;
			itRem = 0;
		}
		/* When deallocating data from header we need to move
		 * the data used to the beginning after the header new
		 * allocated space, so we need to do memmove here */
		if (itRem > p->head_avail) {
			itRem -= p->head_avail;
			memmove(p->buffer, p->buffer + p->head_avail, p->data_size);
			p->head_avail = 0;
		} else {
			p->head_avail -= itRem;
			memmove(p->buffer + p->head_avail, p->buffer + p->head_avail + itRem, p->data_size);
			itRem = 0;
		}
		if (itRem > p->data_size) {
			p->data_size = 0;
		} else {
			p->data_size -= itRem;
		}
		bSize = 0;
	} else
		bSize = new_size - bSize;
	tmp = realloc(p->buffer, new_size);
	if (!new_size) {
		p->buffer = NULL;
		p->data = NULL;
		p->head_avail = 0;
		p->data_avail = 0;
		p->tail_avail = 0;
		p->data_size = 0;
		return;
	}
	if (!tmp)
		return;
	p->data_avail += bSize;
	p->buffer = tmp;
	p->data = p->buffer + p->head_avail;
}

buf_t *buf_reuse(buf_t *p)
{
	if (!p)
		return NULL;
	p->data_avail = buf_total_size(p);
	p->head_avail = 0;
	p->tail_avail = 0;
	p->data_size = 0;
	p->data = p->buffer;
	return p;
}

void *buf_reserve_begin(buf_t *p, size_t data_size)
{
	if (!p)
		return NULL;
	if (p->head_avail >= data_size) {
		p->head_avail -= data_size;
		p->data_size += data_size;
		p->data = p->buffer + p->head_avail;
		return p->buffer + p->head_avail;
	} else {
		if (data_size > p->head_avail + p->data_avail) {
			int tmp;
			tmp = p->data_avail;
			buf_resize(p, buf_total_size(p) + data_size -
			                    p->head_avail - p->data_avail);
			if (tmp == p->data_avail)
				return NULL;
			p->data_avail = 0;
		} else
			p->data_avail -= data_size - p->head_avail;
		memmove(p->buffer + data_size, p->buffer + p->head_avail, p->data_size);
		p->head_avail = 0;
		p->data = p->buffer;
		p->data_size += data_size;
		return p->buffer;
	}
}

void *buf_reserve_end(buf_t *p, size_t data_size)
{
	void *t;

	if (!p)
		return NULL;

	if (p->tail_avail >= data_size)
		p->tail_avail -= data_size;
	else {
		if (data_size > p->tail_avail + p->data_avail) {
			int tmp;
			tmp = p->data_avail;
			buf_resize(p, buf_total_size(p) + data_size -
			                    p->tail_avail - p->data_avail);
			if (tmp == p->data_avail)
				return NULL;
			p->data_avail = 0;
		} else
			p->data_avail -= data_size - p->tail_avail;
		p->tail_avail = 0;
	}
	t = p->buffer + p->head_avail + p->data_size;
	p->data_size += data_size;
	p->data = p->buffer + p->head_avail;
	return t;
}

void buf_insert_begin(buf_t *p, uint8_t *data, size_t data_size)
{
	uint8_t *dest;

	dest = (uint8_t *) buf_reserve_begin(p, data_size);
	assert(dest != NULL);
	memcpy(dest, data, data_size);
}

void buf_insert_end(buf_t *p, uint8_t *data, size_t data_size)
{
	uint8_t *dest;

	dest = (uint8_t *) buf_reserve_end(p, data_size);
	assert(dest != NULL);
	memcpy(dest, data, data_size);
}

void buf_remove_begin(buf_t *p, size_t data_size)
{
	if (!p)
		return;
	if (data_size < p->data_size) {
		p->head_avail += data_size;
		p->data_size -= data_size;
	} else {
		p->head_avail += p->data_size;
		p->data_size = 0;
	}
	p->data = p->buffer + p->head_avail;
}

void buf_remove_end(buf_t *p, size_t data_size)
{
	if (!p)
		return;
	if (data_size < p->data_size) {
		p->tail_avail += data_size;
		p->data_size -= data_size;
	} else {
		p->tail_avail += p->data_size;
		p->data_size = 0;
	}
}

void buf_dump(buf_t *p, const char *label)
{
	int i, n;

	if (!p || !label)
		return;

	n = 0;
	for (i = 0; i < p->data_size; ++i) {
		if (n == 0)
			log_debug("%s%s:", log_debug_prefix, label);
		log_debug(" %02X", p->data[i]);
		if (n >= 25 || i == p->data_size - 1) {
			log_debug("\n");
			n = 0;
		} else
			n++;
	}
}

void buf_free(buf_t *p)
{
	if (!p)
		return;
	if (p->buffer) {
		free(p->buffer);
	}
	free(p);
}
