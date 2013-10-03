/*
 * Copyright © 2012 Collabora, Ltd.
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

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#include "test-runner.h"

int
count_open_fds(void)
{
	DIR *dir;
	struct dirent *ent;
	int count = 0;

	dir = opendir("/proc/self/fd");
	assert(dir && "opening /proc/self/fd failed.");

	errno = 0;
	while ((ent = readdir(dir))) {
		const char *s = ent->d_name;
		if (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
			continue;
		count++;
	}
	assert(errno == 0 && "reading /proc/self/fd failed.");

	closedir(dir);

	return count;
}

void
print_open_fds(void)
{
	DIR *dir;
	struct dirent *ent;

	int stat = -1;
	char linkpath[] = "/proc/self/fd/000";
	char buf[100];

	dir = opendir("/proc/self/fd");
	assert(dir && "opening /proc/self/fd failed.");

	errno = 0;
	while ((ent = readdir(dir))) {
		const char *s = ent->d_name;
		if (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
			continue;

		if (ent->d_type == DT_LNK) {
			snprintf(linkpath, sizeof(linkpath), "/proc/self/fd/%s",
				 ent->d_name);
			stat = readlink(linkpath, buf, 100);
			/* terminate string */
			if (stat != -1)
				buf[stat] = 0;
		}

		fprintf(stderr, "  %-3s: %s\n", ent->d_name,
			(stat == -1) ? "XXX" : buf);
	}
	assert(errno == 0 && "reading /proc/self/fd failed.");

	closedir(dir);
}
