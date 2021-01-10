/*
 * navipage - A program to view and organize Omnavi files
 * Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "navipage.h"
#include "rogueutil.h"

typedef struct {
	/* The actual text of the file. */
	char *text;
	/* The length of the file. */
	size_t length;
	/* The amount of space allocated for the file. */
	size_t size;
	/* The line of the file that is drawn at the top of the page. Used to track
	 * progress through scrolling the file.
	 */
	unsigned int line;
} Buffer;

typedef struct {
	/* The amount of buffers in the list. */
	int amt;
	/* Index of the buffer that is currently open to the user. */
	int index;
	/* Pointer to the array. */
	Buffer *v;
} BufferList;

static void error_buffer(Buffer *, const char *, ...);
static int read_buffer(Buffer *, char *);
int start(void);

extern char *argv0;
extern FileList filelist;
extern Flags flags;

BufferList bufl;

/*
 * Fill the buffer with an error message designated by the arguments.
 */
static void
error_buffer(Buffer *b, const char *format, ...)
{
	va_list ap;
	b->size = 128;
	if ((b->text = malloc(b->size*sizeof(char))) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	va_start(ap, format);
	vsnprintf(b->text, b->size, format, ap);
	va_end(ap);
	b->length = strlen(b->text);
}

/*
 * Read the file at path into b. Returns 0 on success, -1 on error.
 */
static int
read_buffer(Buffer *b, char *path)
{
	FILE *fp;

	/* Each condition completes a task that I need to do to read the file into
	 * the buffer, and also checks if it succeeded. If the condition is true,
	 * then the function failed, and the error is handled accordingly.
	 */
	if ((fp = fopen(path, "r")) == NULL) {
		error_buffer(b, "%s: cannot fopen '%s': %s\n",
				argv0, path, strerror(errno));
		return -1;
	} else if (fseek(fp, 0L, SEEK_END) == -1) {
		error_buffer(b, "%s: cannot fseek '%s': %s\n",
				argv0, path, strerror(errno));
		return -1;
	} else if (ftell(fp) == -1) {
		error_buffer(b, "%s: cannot ftell '%s': %s\n",
				argv0, path, strerror(errno));
		return -1;
	}

	/* b->length must be assigned here because it is of type size_t, which is
	 * unsigned, and therefore cannot be -1. Because of this, -1 cannot be
	 * checked for to detect an error. Therefore it is assigned here, once
	 * we are sure there is not an error with ftell().
	 */
	b->length = (size_t)ftell(fp);
	rewind(fp);
	b->size = b->length + 1;
	if ((b->text = malloc(b->size*sizeof(char))) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	if (fread(b->text, sizeof(char), b->length, fp) != b->length) {
		error_buffer(b, "%s: fread failed on '%s'\n", argv0, path);
		return -1;
	}
	b->text[b->length] = '\0';

	b->line = 0;

	return 0;
}

/*
 * Starts to do the work of the program: load buffers, poll input, etc.
 */
int
start(void)
{
	int i;

	bufl.amt = filelist.amt;
	bufl.index = 0;
	if ((bufl.v = malloc(bufl.amt*sizeof(Buffer))) == NULL) {
		outofmem(EXIT_FAILURE);
	}

	read_buffer(&bufl.v[0], filelist.v[0]);

	if (flags.debug) {
		fprintf(stderr, "length: %zu\nsize: %zu\nline: %u\n",
				bufl.v[0].length, bufl.v[0].size, bufl.v[0].line);
	}

	/* To save time, we will only read buffers from file when the user wants to
	 * read it. Before then, we will initialize all but the first to NULL so
	 * that we can tell in the future whether or not we have read them or not.
	 */
	for (i = 1; i < bufl.amt; i++) {
		bufl.v[i].text = NULL;
	}

	return 0;
}
