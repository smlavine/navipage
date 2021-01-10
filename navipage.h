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

#ifndef NAVIPAGE_H
#define NAVIPAGE_H

#define outofmem(x)	fprintf(stderr, "%s: error: out of memory\n", argv0);\
	exit((x));
#define MAX(A, B)   ((A) > (B) ? (A) : (B))
#define MIN(A, B)   ((A) < (B) ? (A) : (B))

/*
 * A list of files that will be read into buffers. They are not read into
 * buffers immediately because not all will be necessary.
 */
typedef struct {
	/* The amount of files in the list. */
	int amt;
	/* The amount of space allocated for the array. */
	size_t size;
	/* The amount of allocated space that is being used for the array. */
	size_t used;
	/* Pointer to the array. */
	char **v; 
} FileList;

typedef struct {
	unsigned int debug:1;
	unsigned int recurse_more:1;
} Flags;

#endif
