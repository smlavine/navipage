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

extern char *argv0;

/* These functions are declared in the order they are called by each other. */

void vwarn(const char *, va_list);
void vewarn(const char *, va_list);
void verr(const int, const char *, va_list);

void warn(const char *, ...);
void ewarn(const char *, ...);
void err(const int, const char *, ...);
