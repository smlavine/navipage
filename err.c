/*
 * navipage - multi-file pager for watching YouTube videos
 * Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>
 *
 * This file is part of navipage.
 *
 * navipage is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * navipage is distributed in the hope that it will be useful,
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
#include <string.h>

#include "err.h"

char *argv0;

/*
 * This file contains six error and warning functions:
 * vwarn(), vewarn(), and verr(), as well as variadic wrappers for each.
 * 
 * vwarn() prints argv0 to stderr (which should be set by the caller to
 * the name of the program), followed by a colon and a space, followed by
 * a message formatted by a call to vfprintf(3) with the arguments given.
 *
 * vewarn() calls vwarn(), and then, if the given format string is neither
 * NULL nor empty, prints to stderr a colon and a space; then prints to
 * stderr the value of strerror(errno) as it was at the call of the function,
 * followed by a newline. strerror(3) MUST be called at the top of the
 * function because other functions called in the course of execution could
 * modify the value of errno.
 *
 * verr() calls vewarn() and exits the program with the provided code.
 *
 * warn() is a variadic wrapper for vwarn(), ewarn() is one for vewarn(), and
 * err() is one for verr().
 *
 * Some functions in this file are named identically to functions provided by
 * libbsd's err.h, but be aware that this file does NOT implement those
 * functions, and these functions may behave completely differently.
 */

void
vwarn(const char *fmt, va_list ap)
{

	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, ap);
}

void
vewarn(const char *fmt, va_list ap)
{
	const char *errstr = strerror(errno);

	vwarn(fmt, ap);
	if (fmt != NULL && fmt[0] != '\0')
		fputs(": ", stderr);
	fprintf(stderr, "%s\n", errstr);
}


void
verr(const int code, const char *fmt, va_list ap)
{
	vewarn(fmt, ap);

	exit(code);
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}

void
ewarn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vewarn(fmt, ap);
	va_end(ap);
}

void
err(const int code, const char *fmt, ...)
{

	va_list ap;
	
	va_start(ap, fmt);
	verr(code, fmt, ap);
	va_end(ap);
}
