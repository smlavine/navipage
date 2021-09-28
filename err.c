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

/***********************************************************************
 * This file contains six error and warning functions:
 * vwarn(), vewarn(), and verr(), plus variadic wrappers for each.
 *
 ***********************************************************************
 * vwarn() prints argv0, ": ", and a message formatted from the given
 * format string and va_list.
 *
 * warn() is a variadic wrapper for vwarn().
 *
 ***********************************************************************
 * vewarn() calls vwarn(), then prints ": " if the format string is
 * neither NULL nor empty, the value of strerror(errno), and a newline.
 *
 * ewarn() is a variadic wrapper for vewarn().
 *
 ***********************************************************************
 * verr() calls vewarn() and exits the program with the provided code.
 *
 * err() is a variadic wrapper for verr().
 *
 ***********************************************************************
 * If argv0 is not set by the caller, behavior is undefined.
 *
 * Some functions in this file are named the same as functions
 * provided by libbsd's err.h. This file does NOT implement those
 * functions, and might behave completely differently.
 *
 ***********************************************************************
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
	/* strerror(3) must be called before anything else is done,
	 * otherwise errno could be modified by function calls made
	 * between when vewarn() was called and when output is printed.
	 */
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
