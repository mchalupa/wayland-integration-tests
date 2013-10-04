/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <unistd.h>

#include "wit-global.h"
#include "wit-assert.h"

int
asswrite(int fd, void *src, size_t size)
{
	int stat = write(fd, src, size);
	assertf(stat == size,
		"Sent %d instead of %lu bytes",	stat, size);

	return stat;
}

int
assread(int fd, void *dest, size_t size)
{
	int stat = read(fd, dest, size);
	assertf(stat == size,
		"Recieved %d instead of %lu bytes", stat, size);

	return stat;
}

/* Send messages to counterpart */
void
send_message(int fd, enum optype op, ...)
{
	va_list vl;
	int stat, cont, count;

	/* enum optype is defined from 1 */
	assertf(op > 0, "Wrong operation");
	assertf(fd >= 0, "Wrong filedescriptor");

	va_start(vl, op);

	switch (op) {
		case CAN_CONTINUE:
			cont = va_arg(vl, int);

			asswrite(fd, &op, sizeof(op));
			asswrite(fd, &cont, sizeof(int));
			break;
		case RUN_FUNC:
			asswrite(fd, &op, sizeof(op));
			break;
		case EVENT_COUNT:
			count = va_arg(vl, int);

			asswrite(fd, &op, sizeof(op));
			asswrite(fd, &count, sizeof(int));
			break;
		case BARRIER:
			asswrite(fd, &op, sizeof(op));
			break;
		case EVENT_EMIT:
		case SEND_BYTES:
			assertf(0, "Not implemented");
		default:
			assertf(0, "Unknown operation (%d)", op);
	}

	va_end(vl);
}
