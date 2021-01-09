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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Taken from <https://github.com/sakhmatd/rogueutil>. */
#include "rogueutil.h"

/* A list of files that will be read into buffers. They are not read into
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
} Flags;

static void usage(void);

/* 
 * Name of the program, as it is executed by the user. This is declared here
 * because argv[0] is going to be modiifed by getopt.
 */
char *argv0;
FileList filelist;
Flags flags;

static const char *USAGE =
"navipage - A program to view and organize Omnavi files\n"
"Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>\n"
"\n"
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program. If not, see http://www.gnu.org/licenses/.\n"
"\n"
"Usage: navipage [-h] files...\n"
"Options:\n"
"	-h Print this help and exit.\n"
"	-d Enable debug output.\n"
"For examples, see README.md or https://github.com/smlavine/navipage.\n";

static void
usage()
{
	fputs(USAGE, stdout);
}

int
main(int argc, char *argv[])
{
	argv0 = argv[0];
	filelist.amt = 0;
	filelist.size = 4*sizeof(char *);
	filelist.used = 0;
	filelist.v = malloc(filelist.size);
	flags.debug = 0;

	if (filelist.v == NULL) {
		fprintf(stderr, "%s: error: out of memory\n", argv0);
		exit(EXIT_FAILURE);
	}

	/* Handle options. */
	int ch;
	const char *optstring = "dh";
	while ((ch = getopt(argc, argv, optstring)) != -1) {
		switch (ch) {
		case 'd':
			flags.debug = 1;
			break;
		case 'h':
			usage();
		case ':': /* FALLTHROUGH */
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Set argc/argv to remaining arguments. */
	argc -= optind;
	argv += optind;

	/* All remaining arguments should be paths to files to be read. */
	for (int i = 0; i < argc; i++) {
		struct stat statbuf;
		if (stat(argv[i], &statbuf) == -1) {
			int errsv = errno;
			fprintf(stderr, "%s: cannot stat '%s': %s\n",
					argv0, argv[i], strerror(errsv));
		} else if (!S_ISREG(statbuf.st_mode)) {
			fprintf(stderr, "%s: cannot read '%s': not a regular file\n",
					argv0, argv[i]);
		} else { 
			/* Add file path to the list. */

			/*
			 * Make sure that there is enough space allocated in filelist for
			 * a new pointer.
			 */
			while (filelist.size < filelist.used) {
				filelist.size += 4*sizeof(char *);
			}
			/*
			 * Make sure that realloc is valid before reallocating the
			 * filelist.
			 */
			char **tmp = realloc(filelist.v, filelist.size);
			if (tmp == NULL) {
				fprintf(stderr, "%s: error: out of memory\n", argv0);
				exit(EXIT_FAILURE);
			} else {
				filelist.v = tmp;
			}
			
			/* Allocate space for file path. */
			filelist.v[filelist.amt] = malloc((1+strlen(argv[i]))*sizeof(char));
			if (filelist.v[filelist.amt] == NULL) {
				fprintf(stderr, "%s: error: out of memory\n", argv0);
				exit(EXIT_FAILURE);
			}
			filelist.used += sizeof(char *);
			strcpy(filelist.v[filelist.amt], argv[i]);
			filelist.amt++;
		}
	}

	if (flags.debug) {
		printf("amt: %d\nsizeof(char *): %zu\nsize: %zu\nused: %zu\nv:\n",
				filelist.amt, sizeof(char *), filelist.size, filelist.used);
		for (int i = 0; i < filelist.amt; i++) {
			puts(filelist.v[i]);
		}
	}

	return 0;
}
