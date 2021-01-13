/*
 * navipage - multi-file pager for watching YouTube videos
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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rogueutil.h"

#define outofmem(x)	fprintf(stderr, "%s: error: out of memory\n", argv0);\
	exit((x));
#define MAX(A, B)   ((A) > (B) ? (A) : (B))
#define MIN(A, B)   ((A) < (B) ? (A) : (B))

/*
 * To be used as an argument to add_file().
 */
enum {
	NO_RECURSE = 0,
	RECURSE = 1,
};

/*
 * A file buffer. This contains the actual text of the file, but also
 * pointers to the line breaks of the file, which come into use when the file
 * is being scrolled through.
 */
typedef struct {
	/* The actual text of the file. */
	char *text;

	/* The length of the file. */
	size_t length;

	/* The amount of space allocated for the file. */
	size_t size;

	/* An array of pointers to the first character of every line. This is used
	 * in scrolling.
	 */
	char **st;

	/* How many lines there are in the buffer. */
	size_t stlength;

	/* The amount of space allocated for st. */
	size_t stsize;

	/* The variable such that st[top] points to the start of the line that is
	 * drawn at the top of the screen. It changes when the screen is scrolled.
	 */
	size_t top;
} Buffer;

/*
 * A list of file buffers. Buffers can be switched between during the run of
 * the program.
 */
typedef struct {
	/* The amount of buffers in the list. */
	int amt;

	/* Index of the buffer that is currently open to the user. */
	int n;

	/* Pointer to the array. */
	Buffer *v;
} BufferList;

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

/*
 * Function prototypes.
 */
static int add_file(const char *, int);
static void cleanup(int);
static int cmpfilestring(const void *, const void *);
static void display_buffer(Buffer *);
static void error_buffer(Buffer *, const char *, ...);
static int init_buffer(Buffer *, char *);
static void update_rows(void);
static void usage(void);


extern char **environ;
/* Name of the program, as it is executed by the user. This is declared here
 * because argv[0] is going to be modiifed by getopt.
 */
char *argv0;
Flags flags;
FileList filel;
BufferList bufl;
int rows;
static const char *USAGE =
"navipage - multi-file pager for watching YouTube videos\n"
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
"Usage: navipage [-dhrv] files...\n"
"Options:\n"
"    -d  Enable debug output.\n"
"    -h  Print this help and exit.\n"
"    -r  Infinitely recurse in directories.\n"
"    -v  Print version and exit.\n";

/*
 * Append the given file path to filel. If the file is a directory, and
 * recurse is nonzero, then all files will be added to filel by calling this
 * function recursively -- but it will only recurse one directory level deep
 * unless flags.recurse_more is set.
 * Return value shall be 0 on success, and -1 on error. Upon irreconciliable
 * errors, such as running out of memory, the program will be exit with the
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

		/* Make sure that there is enough space allocated in filel for
		 * a new pointer.
		 */
		while (filel.size < filel.used) {
			filel.size += 4*sizeof(char *);
		}
		/* Make sure that realloc is valid before reallocating the
		 * filel.
		 */
		char **tmp = realloc(filel.v, filel.size);
		if (tmp == NULL) {
			outofmem(EXIT_FAILURE);
		} else {
			filel.v = tmp;
		}

		/* Allocate space for file path. */
		filel.v[filel.amt] =
			malloc((1+strlen(path))*sizeof(char));
		if (filel.v[filel.amt] == NULL) {
			outofmem(EXIT_FAILURE);
		}
		filel.used += sizeof(char *);
		strcpy(filel.v[filel.amt], path);
		filel.amt++;
	}

	return 0;
}

/*
 * Cleans up terminal settings and the like before exitting the program.
 */
static void
cleanup(int sig)
{
	switch (sig) {
	case 0: /* Used when I want to cleanup without raising a signal. */
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
	case SIGSEGV:
	case SIGHUP:
		system("stty sane");
		showcursor();
		/* Print a blank line to return shell prompt to the beginning of the
		 * next line.
		 */
		puts("");
	}
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
 * Display all text from b->st[b->top] to the end of the screen.
 */
static void
display_buffer(Buffer *b)
{
	int i;
	char *tmp;
	int len;

	cls();
	gotoxy(1, 1);
	for (i = 0; i < rows; i++) {
		if ((tmp = strchr(b->st[b->top + i], '\n')) == NULL) {
			tmp = strchr(b->st[b->top + i], '\0');
		}
		len = tmp - b->st[b->top + i] + 1;
		printf("%3zu ", b->top + i);
		fwrite(b->st[b->top + i], sizeof(char), len, stdout);
	}
	gotoxy(1, rows);
	printf("@%d/%d\t%s", bufl.n + 1, bufl.amt, filel.v[bufl.n]);
	fflush(stdout);
}

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
 * Read the file at path into b, and set various values of b, like length,
 * top, offset, etc. Returns 0 on success, -1 on error.
 */
static int
init_buffer(Buffer *b, char *path)
{
	FILE *fp;
	size_t i;
	/* This string is appended to the end of all buffers. */
	const char *ENDSTR = "\n\n";

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
	b->size = b->length + strlen(ENDSTR) + 2;
	if ((b->text = malloc(b->size*sizeof(char))) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	if (fread(b->text, sizeof(char), b->length, fp) != b->length) {
		error_buffer(b, "%s: fread failed on '%s'\n", argv0, path);
		return -1;
	}

	/* Terminate the string. */
	b->text[b->length] = '\0';
	strcat(b->text, ENDSTR);
	b->length += strlen(ENDSTR);

	/* Now we must find the amount of lines, and the location of all of the
	 * starts of lines. This will be used in scrolling the screen.
	 */

	b->top = 0;
	/* Get this edge case out of the way first. */
	if (b->length == 0) {
		b->stlength = 0;
		return 0;
	}

	/* We will incrememnt memory in blocks of 10. That is, every time we
	 * reallocate memory because there is not enough room, we will add enough
	 * memory to fit ten more char pointers.
	 */
	if ((b->st = malloc((b->stsize = 10)*sizeof(char *))) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	/* The first line starts at the first character of the text, so we start
	 * there.
	 */
	b->st[0] = b->text;
	b->stlength = 1;
	for (i = 1; i < b->length; i++) {
		if (b->text[i - 1] == '\n') {
			if (b->stlength >= b->stsize) {
				b->st = realloc(b->st, (b->stsize += 10)*sizeof(char *));
				if (b->st == NULL) {
					outofmem(EXIT_FAILURE);
				}
			}
			b->st[b->stlength] = b->text + i;
			b->stlength++;
		}
	}
	return 0;
}

/*
 * Update the 'rows' global variable, usually involving the rogueutil function
 * trows().
 */
static void
update_rows(void)
{
	rows = trows();
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
	const char *optstring = "dhrv";

	if (argc == 1) {
		usage();
		exit(EXIT_FAILURE);
	}

	/* Call cleanup before suddenly exiting the program. */
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGSEGV, cleanup);
	signal(SIGHUP, cleanup);

	argv0 = argv[0];
	filel.size = 4*sizeof(char *);
	filel.used = 0;
	if ((filel.v = malloc(filel.size)) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	update_rows();
	flags.debug = 0;
	flags.recurse_more = 0;

	/* Handle options. */
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'd':
			flags.debug = 1;
			break;
		case 'r':
			flags.recurse_more = 1;
			break;
		case 'v':
			printf("navipage version %s\n", VERSION);
			exit(EXIT_SUCCESS);
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case ':':
		case '?':
			usage();
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
	if (filel.amt == 0) {
		exit(EXIT_FAILURE);
	}

	/* Sort files such that the start of the list is the newest file --
	 * assuming that they are named like YYYYMMDD[...].
	 */
	qsort(filel.v, filel.amt, sizeof(char *), cmpfilestring);

	/*
	 * Now we can begin working with buffers.
	 */
	bufl.amt = filel.amt;
	bufl.n = 0;
	if ((bufl.v = malloc(bufl.amt*sizeof(Buffer))) == NULL) {
		outofmem(EXIT_FAILURE);
	}

	init_buffer(&bufl.v[bufl.n], filel.v[0]);

	/* To save time, we will only initialize buffers from file when the user
	 * wants to view them. Before then, we will initialize all but the first of
	 * the texts to NULL so that we can tell in the future whether or not we
	 * have already initialized them or not.
	 */
	for (i = 1; i < bufl.amt; i++) {
		bufl.v[i].text = NULL;
	}

	/*
	 * Before we start displaying files to the user and reading input, some
	 * housekeeping.
	 */

	/* Disable showing input on the screen while the program runs. This
	 * prevents all the 'j's and 'k's from showing on the bottom of the screen.
	 */
	system("stty -echo");
	/*
	 * Hide the cursor. It takes up a space, and can be seen moving when
	 * printing the file, which is undesirable.
	 */
	hidecursor();

	/*
	 * Showtime.
	 */
	display_buffer(&bufl.v[bufl.n]);

	/*
	 * The main input loop!
	 */
	for (;;) {
		switch (nb_getch()) {
		case 'g':
			/* Automatically scroll to the top of the page. */
			bufl.v[bufl.n].top = 0;
			display_buffer(&bufl.v[bufl.n]);
			break;
		case 'G':
			/* Automatically scroll to the bottom of the page. */
			bufl.v[bufl.n].top = bufl.v[bufl.n].stlength - 1 - rows;
			display_buffer(&bufl.v[bufl.n]);
			break;
		case 'h':
			/* Move to the next-most-recent buffer. */
			if (bufl.n > 0) {
				bufl.n--;
				if (bufl.v[bufl.n].text == NULL) {
					init_buffer(&bufl.v[bufl.n], filel.v[bufl.n]);
				}
				display_buffer(&bufl.v[bufl.n]);
			}
			break;
		case 'j':
		case '\005': /* scroll down (^E) */
			/* Scroll down one line. */
			if (bufl.v[bufl.n].top + rows < bufl.v[bufl.n].stlength - 1) {
				bufl.v[bufl.n].top++;
				display_buffer(&bufl.v[bufl.n]);
			}
			break;
		case 'k':
		case '\031': /* scroll up (^Y) */
			/* Scroll up one line. */
			if (bufl.v[bufl.n].top > 0) {
				bufl.v[bufl.n].top--;
				display_buffer(&bufl.v[bufl.n]);
			}
			break;
		case 'l':
			/* Move to the next-less-recent buffer. */
			if (bufl.n < bufl.amt - 1) {
				bufl.n++;
				if (bufl.v[bufl.n].text == NULL) {
					init_buffer(&bufl.v[bufl.n], filel.v[bufl.n]);
				}
				display_buffer(&bufl.v[bufl.n]);
			}
			break;
		case 'q':
			/* Quit navipage. */
			cleanup(0);
			exit(EXIT_SUCCESS);
			break;
		case 'r':
			/* Redraw the buffer. */
			update_rows();
			display_buffer(&bufl.v[bufl.n]);
			break;
		}
	}

	return 0;
}
