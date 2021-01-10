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

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "navipage.h"

/*
 * To be used as an argument to add_file().
 */
enum {
	NO_RECURSE = 0,
	RECURSE = 1,
};

extern int start(void);
static int add_file(const char *, int);
static int cmpfilestring(const void *, const void *);
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
"Usage: navipage [-dhr] files...\n"
"Options:\n"
"    -d  Enable debug output.\n"
"    -h  Print this help and exit.\n"
"    -r  Infinitely recurse in directories.\n"
"For examples, see README.md or https://github.com/smlavine/navipage.\n";

/*
 * Append the given file path to filelist. If the file is a directory, and
 * recurse is nonzero, then all files will be added to filelist by calling this
 * function recursively -- but it will only recurse one directory level deep
 * unless flags.recurse_more is set.
 * Return value shall be 0 on success, and -1 on error. Upon irreconciliable
 * errors, such as running out of memory, the program will be exitted with
 * code EXIT_FAILURE.
 */
static int
add_file(const char *path, int recurse)
{
	int errsv;
	struct dirent *d;
	DIR *dirp;
	char *newpath;
	struct stat statbuf;

	if (stat(path, &statbuf) == -1) {
		errsv = errno;
		fprintf(stderr, "%s: cannot stat '%s': %s\n",
				argv0, path, strerror(errsv));
		return -1;
	} else if (S_ISDIR(statbuf.st_mode)) {
		if (recurse) {
			/* Recurse upon the directory. */
			dirp = opendir(path);
			if (dirp == NULL) {
				errsv = errno;
				fprintf(stderr, "%s: cannot opendir '%s': %s\n",
						argv0, path, strerror(errsv));
				return -1;
			} else {
				/* We need to set errno here, because it is the only way to
				 * to determine if readdir() errors out, or finishes
				 * successfully.
				 */
				errno = 0;
				while ((d = readdir(dirp)) != NULL) {
					/* If we do not exclude these (the current and one-up
					 * directories), then they will recurse unto themselves,
					 * creating many repeats in the list.
					 */
					if (strcmp(d->d_name, ".") == 0 ||
							strcmp(d->d_name, "..") == 0) {
						continue;
					}

					/* +2 is for '/' and '\0'. */
					newpath =
						malloc(strlen(path)+2+strlen(d->d_name)*sizeof(char));
					sprintf(newpath, "%s", path);
					if (path[strlen(path) - 1] != '/') {
						/* Add strlen(newpath) here so that the original part
						 * of newpath -- the directory path -- is not
						 * overwritten.
						 */
						sprintf(newpath+strlen(newpath), "/");
					}
					sprintf(newpath+strlen(newpath), "%s", d->d_name);

					add_file(newpath,
							flags.recurse_more ? RECURSE : NO_RECURSE);

					free(newpath);
				}
				closedir(dirp);
				if ((errsv = errno) != 0) {
					fprintf(stderr, "%s: stopping readdir '%s': %s\n",
							argv0, path, strerror(errsv));
				}
			}
		} else {
			fprintf(stderr, "%s: -r not specified; omitting directory '%s'\n",
					argv0, path);
			return -1;
		}

	} else if (!S_ISREG(statbuf.st_mode)) {
		fprintf(stderr, "%s: cannot read '%s': not a regular file\n",
				argv0, path);
		return -1;
	} else { 
		/* Add file path to the list. */

		/* Make sure that there is enough space allocated in filelist for
		 * a new pointer.
		 */
		while (filelist.size < filelist.used) {
			filelist.size += 4*sizeof(char *);
		}
		/* Make sure that realloc is valid before reallocating the
		 * filelist.
		 */
		char **tmp = realloc(filelist.v, filelist.size);
		if (tmp == NULL) {
			outofmem(EXIT_FAILURE);
		} else {
			filelist.v = tmp;
		}

		/* Allocate space for file path. */
		filelist.v[filelist.amt] =
			malloc((1+strlen(path))*sizeof(char));
		if (filelist.v[filelist.amt] == NULL) {
			outofmem(EXIT_FAILURE);
		}
		filelist.used += sizeof(char *);
		strcpy(filelist.v[filelist.amt], path);
		filelist.amt++;
	}

	return 0;
}

/*
 * To be called by qsort. Sorts two strings, but only sorts the basename of
 * the path.
 */
static int
cmpfilestring(const void *p1, const void *p2)
{
	/* POSIX-compliant basename() may modify the path variable, which we don't
	 * want. For this reason, we copy the string before passing it to
	 * basename().
	 */
	char *base1, *base2, *copy1, *copy2;
	int ret;
	
	/* To quote 'man 3 qsort' on the reasoning for these casts:
	 * "The actual arguments to this function are "pointers to pointers to
	 * char", but strcmp(3) arguments are "pointers to char", hence the
	 * following cast plus dereference"
	 * This also applies to strlen.
	 */
	copy1 = malloc((1+strlen(*(const char **)p1))*sizeof(char));
	copy2 = malloc((1+strlen(*(const char **)p2))*sizeof(char));
	if (copy1 == NULL || copy2 == NULL) {
		outofmem(EXIT_FAILURE);
	}

	strcpy(copy1, *(const char **)p1);
	strcpy(copy2, *(const char **)p2);
	base1 = basename(copy1);
	base2 = basename(copy2);

	/* Note the negation here. This will sort the list with the most recent
	 * file at the start.
	 */
	ret = -strcmp(base1, base2);

	free(copy1);
	free(copy2);

	return ret;
}

/*
 * Print help about the program.
 */
static void
usage(void)
{
	fputs(USAGE, stdout);
}

int
main(int argc, char *argv[])
{
	int c, i;
	const char *optstring = "dhr";

	argv0 = argv[0];
	filelist.size = 4*sizeof(char *);
	filelist.used = 0;
	filelist.v = malloc(filelist.size);
	flags.debug = 0;
	flags.recurse_more = 0;

	if (filelist.v == NULL) {
		outofmem(EXIT_FAILURE);
	}

	/* Handle options. */
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'd':
			flags.debug = 1;
			break;
		case 'r':
			flags.recurse_more = 1;
			break;
		case 'h':
			usage();
			/* FALLTHROUGH */
		case ':':
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Set argc/argv to remaining arguments. */
	argc -= optind;
	argv += optind;

	/* All remaining arguments should be paths to files to be read. */
	for (i = 0; i < argc; i++) {
		add_file(argv[i], RECURSE);
	}

	/* Exit the program if there are no files to read. */
	if (filelist.amt == 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	/* Sort files such that the start of the list is the newest file --
	 * assuming that they are named like YYYYMMDD[...].
	 */
	qsort(filelist.v, filelist.amt, sizeof(char *), cmpfilestring);

	if (flags.debug) {
		fprintf(stderr,
				"amt: %d\nsizeof(char *): %zu\nsize: %zu\nused: %zu\nv:\n",
				filelist.amt, sizeof(char *), filelist.size, filelist.used);
		for (i = 0; i < filelist.amt; i++) {
			fputs(filelist.v[i], stderr);
		}
	}

	return start();
}
