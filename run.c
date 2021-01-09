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

#include <stdio.h>

#include "navipage.h"
#include "rogueutil.h"


typedef struct {
	/* The actual text of the file. */
	char *text;
	/* The line of the file that is drawn at the top of the page. Used to track
	 * progress through scrolling the file.
	 */
	unsigned int line;
} Buffer;

int start(void);

extern char *argv0;
extern FileList filelist;
extern Flags flags;

/*
 * Starts to do the work of the program: load buffers, poll input, etc.
 */
int
start(void)
{
	return 0;
}
