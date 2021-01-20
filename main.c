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
#include <readline/readline.h> /* Must be included after stdio.h. */
#include <readline/history.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "re.h"
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

/* The result of a search for a regex.
 */
typedef struct {
	/* A pointer to the match. */
	char *p;

	/* The length of the match. */
	int len;
} SearchResult;

/* Information relating to a search, including the regex searched for, and a
 * list of SearchResults.
 */
typedef struct {
	/* The current regex that has been searched for. */
	re_t search;

	/* The string corresponding to search. */
	char *regex;

	/* An array of SearchResults. */
	SearchResult *v;

	/* How many matches there are in v. */
	int amt;

	/* The amount of space allocated for v. */
	int size;

	/* Such that v[current] is the currently highlighted result in the buffer.
	 */
	int current;
} Search;

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
	int size;

	/* An array of pointers to the first character of every line. This is used
	 * in scrolling.
	 */
	char **st;

	/* How many lines there are in the buffer. */
	int st_amt;

	/* The amount of space allocated for st. */
	int st_size;

	/* The variable such that st[top] points to the start of the line that is
	 * drawn at the top of the screen. It changes when the screen is scrolled.
	 */
	int top;

	/* Information relating to a search of the buffer for a regex. */
	Search s;
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
	int size;

	/* The amount of allocated space that is being used for the array. */
	int used;

	/* Pointer to the array. */
	char **v; 
} FileList;

typedef struct {
	unsigned int debug:1;
	unsigned int recurse_more:1;
	unsigned int sh:1;
} Flags;

/*
 * Function prototypes.
 */
static int add_file(const char *, int);
static int change_buffer(int);
static void cleanup(void);
static int cmpfilestring(const void *, const void *);
static void display_buffer(Buffer *);
static void error_buffer(Buffer *, const char *, ...);
static void execute_command(void);
static void info(void);
static int init_buffer(Buffer *, char *);
static void input_loop(void);
static void prompt_search(void);
static void quit(int);
static void redraw(void);
static long scroll(int);
static void scroll_to_top(void);
static void scroll_to_bottom(void);
static void search_buffer(const char *, Buffer *);
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
/* ANSI escape code to swap the foreground and background colors. This is used
 * to create a 'highlight' of search results.
 */
static const char *REVERSE_VIDEO = "\033[07m";

static const char *USAGE =
"navipage - multi-file pager for watching YouTube videos\n"
"Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>\n"
"This program is free software (GPLv3+); see 'man navipage'\n"
"or <github.com/smlavine/navipage> for more information.\n"
"Usage: navipage [-dhrv] files...\n"
"Options:\n"
"    -d  Enable debug output.\n"
"    -h  Print this help and exit.\n"
"    -r  Infinitely recurse in directories.\n"
"    -s  If $NAVIPAGE_SH is set, run it as a\n"
"        shell script before files are read.\n"
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
 * Move by 'offset' buffers in the buffer list. If the operation is
 * successful, that is, the new value is in the range of possible indices,
 * then 0 is returned; otherwise, the value that bufl.n would have been set
 * to is returned.
 */
static int
change_buffer(int offset)
{
	int tmp;
	tmp = bufl.n + offset;
	if (tmp >= 0 && tmp < bufl.amt) {
		bufl.n = tmp;
		display_buffer(&bufl.v[bufl.n]);
		return 0;
	} else {
		return tmp;
	}
}

/*
 * Cleans up terminal settings and the like which were modified by navipage.
 * Usually called before exitting the program.
 */
static void
cleanup(void)
{
	system("stty sane");
	showcursor();
	/* Print a blank line to return shell prompt to the beginning of the
	 * next line.
	 */
	puts("");
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
	int i, linestoprint, linelen;
	char *eolptr;

	cls();
	gotoxy(1, 1);
	/* The amount of lines to be printed in this call. Print rows - 1 (the
	 * height of the screen, not including the status bar), but only print
	 * the amount of lines in the file if that is less than rows - 1, to
	 * avoid a segfault.
	 */
	linestoprint = MIN(b->st_amt, rows - 1);
	for (i = 0; i < linestoprint; i++) {
		/* If this line has a search match, print it in color. */
		if (b->s.search != NULL &&
				b->st[b->top + i] <= b->s.v[b->s.current].p &&
				b->s.v[b->s.current].p <=
				(i == linestoprint - 1 ?
				 &b->text[b->length - 1] :
				 b->st[b->top + i + 1])) {
			setColor(GREEN);
		}
		printf("%3d ", b->top + i + 1);
		/* Find the location of the end of the line, or if an eol cannot be
		 * found, then it is the last line in the file and we should find the
		 * eof.
		 */
		if ((eolptr = strchr(b->st[b->top + i], '\n')) == NULL) {
			eolptr = strchr(b->st[b->top + i], '\0');
		}
		linelen = eolptr - b->st[b->top + i] + 1;
		fwrite(b->st[b->top + i], sizeof(char), linelen, stdout);
		/* Set color back to normal for next line, if there was a search
		 * match on this line.
		 */
		resetColor();
	}
	/* Print status-bar information. */
	gotoxy(1, rows);
	printf("#%d/%d  %s  %s",
			bufl.n + 1, bufl.amt, filel.v[bufl.n], "Press 'i' for help.");
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
 * Get a command from the user, then execute it.
 */
static void
execute_command(void)
{
	char *line;
	gotoxy(1, rows);
	setColor(YELLOW);
	/* Clear the line of text before showing the command prompt. */
	printf("%*s", tcols(), "");
	gotoxy(1, rows);
	fflush(stdout);
	/* Re-enable displaying user input. */
	system("stty echo");
	showcursor();
	line = readline("!");
	if (line != NULL) {
		system(line);
	}
	gotoxy(1, rows);
	/* The command could change the color of text, so we should re-set it here
	 * in addition to above.
	 */
	setColor(YELLOW);
	fputs("navipage: press any key to return.", stdout);
	fflush(stdout);
	anykey(NULL);
	system("stty -echo");
	hidecursor();
	resetColor();
	display_buffer(&bufl.v[bufl.n]);
	free(line);
}

/*
 * Display helpful information in the following order, with the next option
 * being tried if the first fails:
 * 1. man navipage
 * 2. less README.md
 * 3. Displaying a link to https://github.com/smlavine/navipage
 */
static void
info(void)
{
	int ret;
	ret = system("man navipage");
	if (ret != 0) {
		ret = system("less README.md");
	}
	if (ret != 0) {
		gotoxy(1, rows);
		setColor(YELLOW);
		fputs("See <https://github.com/smlavine/navipage> for help.",
				stdout);
		resetColor();
		fflush(stdout);
	}
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

	/* Terminate the string. */
	b->text[b->length] = '\0';

	/* Now we must find the amount of lines, and the location of all of the
	 * starts of lines. This will be used in scrolling the screen.
	 */

	b->top = 0;
	/* Get this edge case out of the way first. */
	if (b->length == 0) {
		b->st_amt = 0;
		return 0;
	}

	/* We will incrememnt memory in blocks of 10. That is, every time we
	 * reallocate memory because there is not enough room, we will add enough
	 * memory to fit ten more char pointers.
	 */
	if ((b->st = malloc((b->st_size = 10)*sizeof(char *))) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	/* The first line starts at the first character of the text, so we start
	 * there.
	 */
	b->st[0] = b->text;
	b->st_amt = 1;
	for (i = 1; i < b->length; i++) {
		if (b->text[i - 1] == '\n') {
			if (b->st_amt >= b->st_size) {
				b->st = realloc(b->st, (b->st_size += 10)*sizeof(char *));
				if (b->st == NULL) {
					outofmem(EXIT_FAILURE);
				}
			}
			b->st[b->st_amt] = b->text + i;
			b->st_amt++;
		}
	}

	/* Initialize search values. */

	b->s.search = NULL;
	b->s.regex = NULL;
	b->s.v = NULL;
	b->s.amt = 0;
	b->s.size = 0;
	b->s.current = 0;
	return 0;
}

/*
 * The main input loop.
 */
static void
input_loop(void)
{
	for (;;) {
		switch (getch()) {
		case 'g':
			scroll_to_top();
			break;
		case 'G':
			scroll_to_bottom();
			break;
		case 'h':
			/* Move to the next-most-recent buffer. */
			change_buffer(-1);
			break;
		case 'i':
			info();
			break;
		case 'j':
		case '\005': /* scroll down (^E) */
			/* Scroll down one line. */
			scroll(1);
			break;
		case 'k':
		case '\031': /* scroll up (^Y) */
			/* Scroll up one line. */
			scroll(-1);
			break;
		case 'l':
			/* Move to the next-less-recent buffer. */
			change_buffer(1);
			break;
		case 'q':
			quit(0);
			break;
		case 'r':
			redraw();
			break;
		case '!':
			execute_command();
			break;
		case '/':
			prompt_search();
			break;
		}
	}
}

/*
 * Obtain a regex to search for in the current buffer.
 */
static void
prompt_search(void)
{
	char *line;
	gotoxy(1, rows);
	setColor(YELLOW);
	/* Clear the line of text before showing the search prompt. */
	printf("%*s", tcols(), "");
	gotoxy(1, rows);
	fflush(stdout);
	/* Re-enable displaying user input. */
	system("stty echo");
	showcursor();
	line = readline("/");
	if (line != NULL) {
		/* If nothing was entered on the line, don't search for it, but do
		 * delete the current search term. */
		if (strlen(line) == 0) {
			if (bufl.v[bufl.n].s.search != NULL) {
				free(bufl.v[bufl.n].s.regex);
				free(bufl.v[bufl.n].s.v);
			}
			bufl.v[bufl.n].s.search = NULL;
			bufl.v[bufl.n].s.regex = NULL;
			bufl.v[bufl.n].s.v = NULL;
			bufl.v[bufl.n].s.amt = 0;
			bufl.v[bufl.n].s.size = 0;
		} else {
			search_buffer(line, &bufl.v[bufl.n]);
		}
	}
	gotoxy(1, rows);
	system("stty -echo");
	hidecursor();
	resetColor();
	display_buffer(&bufl.v[bufl.n]);
	/* TODO: display search results like vim does in the buffer. */
	free(line);
}

/*
 * Quit navipage.
 */
static void
quit(int n)
{
	cleanup();
	switch (n) {
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		exit(EXIT_SUCCESS);
		break;
	case SIGSEGV:
	case SIGHUP:
		exit(EXIT_FAILURE);
		break;
	default:
		exit(n);
		break;
	}
}

/*
 * Redraw the current buffer.
 */
static void
redraw(void)
{
	update_rows();
	display_buffer(&bufl.v[bufl.n]);
}

/*
 * Scroll by 'offset' lines in the buffer. If the operation is successful, that
 * is, the caller doesn't try to scroll outside the bounds of the file, then 0
 * is returned; otherwise, the line number that would have been scrolled to
 * is returned.
 */
static long
scroll(int offset)
{
	int tmp;
	tmp = bufl.v[bufl.n].top + offset;
	/* tmp is the "new" line index. Compare it to see that it is less than the
	 * amount of lines, minus the amount of rows because top is of course at
	 * the top of the screen and, the end of the file will be at the bottom,
	 * and add 1 to that number because we do not print on the bottom-most
	 * line of the screen (that is for status information like the current
	 * buffer), and 1 more because st_amt is 1-indexed while tmp is 0-indexed.
	 * The second comparison is much simpler; simply see that the top of the
	 * screen does not scroll behind the start of the file.
	 */
	if (tmp < bufl.v[bufl.n].st_amt - rows + 2 && tmp >= 0) {
		bufl.v[bufl.n].top = tmp;
		display_buffer(&bufl.v[bufl.n]);
		return 0;
	} else {
		return tmp;
	}
}

/*
 * Scroll to the top of the buffer.
 */
static void
scroll_to_top(void)
{
	bufl.v[bufl.n].top = 0;
	display_buffer(&bufl.v[bufl.n]);
}

/*
 * Scroll to the bottom of the buffer.
 */
static void
scroll_to_bottom(void)
{
	bufl.v[bufl.n].top = bufl.v[bufl.n].st_amt - rows + 1;
	display_buffer(&bufl.v[bufl.n]);
}

/*
 * Search the given buffer for the given regex.
 */
static void
search_buffer(const char *regex, Buffer *b)
{
	int index, matchlength;
	char *front;

	front = b->text;

	/* If a term has already been searched for in this session, free the
	 * current regex string to avoid memory leak. Then copy the regex string
	 * into the current buffer, because it will be freed by prompt_search(),
	 * and compile the regex.
	 */
	if (b->s.search != NULL) {
		free(b->s.regex);
		free(b->s.v);
	}
	b->s.amt = 0;
	b->s.regex = malloc((strlen(regex) + 1)*sizeof(char));
	strcpy(b->s.regex, regex);
	b->s.search = re_compile(b->s.regex);
	b->s.size = 10;
	b->s.v = malloc(b->s.size*sizeof(SearchResult));
	if (b->s.v == NULL) {
		outofmem(EXIT_FAILURE);
	}

	/* Starting at the top of the file, search for matches to the regex. If
	 * a match is found, then it is added to b->s.v, and front is set to the
	 * character after the end of the match. The loop continues until a match
	 * is not found, meaning all of the matches in the file have been found.
	 */
	while ((index = re_matchp(b->s.search, front, &matchlength)) != -1) {
		/* Reallocate, if there isn't enough space. */
		if (b->s.amt >= b->s.size) {
			b->s.v = realloc(b->s.v, (b->s.size += 10)*sizeof(SearchResult));
			if (b->s.v == NULL) {
				outofmem(EXIT_FAILURE);
			}
			b->s.amt = 0;
		}
		/* Record location and length of match. */
		b->s.v[b->s.amt].p = &front[index];
		b->s.v[b->s.amt].len = matchlength;
		b->s.amt++;
		/* Move front to after the pattern, unless the end of the file would
		 * be reached.
		 */
		if ((size_t)(index + matchlength) < b->length) {
			front = &front[index + matchlength];
		} else {
			break;
		}
	}
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
	char *envstr;
	const char *optstring = "dhrsv";

	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	signal(SIGQUIT, quit);
	signal(SIGSEGV, quit);
	signal(SIGHUP, quit);

	/* Initialize variables. */
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
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case 'r':
			flags.recurse_more = 1;
			break;
		case 's':
			flags.sh = 1;
			break;
		case 'v':
			printf("navipage version %s\n", VERSION);
			exit(EXIT_SUCCESS);
			break;
		case ':':
		case '?':
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* If -s was specified, look for $NAVIPAGE_SH. If it exists, run it as a
	 * shell script.
	 */
	if (flags.sh) {
		for (i = 0; (envstr = environ[i]) != NULL; i++) {
			if (strstr(envstr, "NAVIPAGE_SH=") == envstr &&
					(envstr = strchr(envstr, '=') + 1)[0] != '\0') {
				/* Execute the shell script. */
				const char *cmd = "sh ";
				char fullcmd[strlen(cmd) + strlen(envstr) + 1];
				sprintf(fullcmd, "%s%s", cmd, envstr);
				system(fullcmd);
			}
		}
	}


	/* Set argc/argv to remaining arguments. */
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		/* If NAVIPAGE_DIR is set, read all files in it (if it is a directory)
		 * to filelist.
		 */
		for (i = 0; (envstr = environ[i]) != NULL; i++) {
			if (strstr(envstr, "NAVIPAGE_DIR=") == envstr &&
					(envstr = strchr(envstr, '=') + 1)[0] != '\0') {
				int recurse_before = flags.recurse_more;
				flags.recurse_more = 1;
				add_file(envstr, RECURSE);
				flags.recurse_more = recurse_before;
			}
		}
		if (filel.amt == 0) {
			usage();
			exit(EXIT_FAILURE);
		}
	}

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

	/*
	 * Iniitalize all buffers.
	 */
	for (i = 0; i < bufl.amt; i++) {
		init_buffer(&bufl.v[i], filel.v[i]);
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
	input_loop();

	return 0;
}
