/*
 * err - Small error-printing library
 * Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdarg.h>

extern char *argv0;

/* These functions are declared in the order they are called by each other. */

void vwarn(const char *, va_list);
void vewarn(const char *, va_list);
void verr(const int, const char *, va_list);

void warn(const char *, ...);
void ewarn(const char *, ...);
void err(const int, const char *, ...);
