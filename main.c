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

#include "rogueutil.h"

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define ST_SIZE_INCR 10

#define URL   "https://sr.ht/~smlavine/navipage"
#define USAGE "Copyright (C) 2021 Sebastian LaVine <mail@smlavine.com>\n" \
	"This program is free software (GPLv3+); see 'man navipage'\n" \
	"or <" URL "> for more information.\n" \
	"Usage: navipage [-dhnrsv] files...\n" \
	"Options:\n" \
	"    -d  Enable debug output.\n" \
	"    -h  Print this help and exit.\n" \
	"    -n  Display line numbers.\n" \
	"    -r  Infinitely recurse in directories.\n" \
	"    -s  Run $NAVIPAGE_SH before reading files.\n" \
	"    -v  Print version and exit."

enum add_path_recurse_argument {
	NO_RECURSE = 0,
	RECURSE = 1
};

/*
 * Used for certain non-obvious input keys used in input_loop().
 */
enum key {
	CTRL_E = '\005', /* Scroll wheel down in st and other terminals. */
	CTRL_Y = '\031'  /* Scroll wheel up. */
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
	long length;

	/* The amount of space allocated for the file. */
	long size;

	/* An array of pointers to the first character of every line. This is
	 * used in scrolling.
	 */
	char **st;

	/* How many lines there are in the buffer. */
	int st_amt;

	/* The amount of space allocated for st. */
	int st_size;

	/* The variable such that st[top] points to the start of the line that
	 * is drawn at the top of the screen. See scroll().
	 */
	int top;
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
	unsigned int numbers:1;
	unsigned int recurse_more:1;
	unsigned int sh:1;
} Flags;

/* Function prototypes. */
static int add_directory(const char *const, const int);
static int add_path(const char *const, const int);
static int change_buffer(const int);
static void cleanup(void);
static int compare_path_basenames(const void *, const void *);
static void display_buffer(const Buffer *const);
static void error_buffer(Buffer *const, const char *, ...);
static void execute_command(void);
static void handle_signals(const int);
static void info(void);
static int init_buffer(Buffer *const, const char *const);
static void input_loop(void);
static void outofmem(const int);
static void quit(const int);
static void redraw(void);
static int scroll(const int);
static void scroll_to_top(void);
static void scroll_to_bottom(void);
static void toggle_numbers(void);
static void update_rows(void);
static void usage(void);
static void version(void);

/* Name of the program, as it is executed by the user. This is declared here
 * because argv[0] is going to be modiifed by getopt.
 */
char *argv0;

Flags flags;
FileList filel;
BufferList bufl;
int rows;

/*
 * Append the files in the directory called path to filel. Return value shall
 * be 0 on success, and -1 on error. Upon irreconciliable errors, such as
 * running out of memory, the program shall be exited with code EXIT_FAILURE.
 */
static int
add_directory(const char *const path, const int recurse)
{
	int errsv;
	struct dirent *d;
	DIR *dirp;
	char *newpath;

	dirp = opendir(path);
	if (dirp == NULL) {
		errsv = errno;
		fprintf(stderr, "%s: cannot opendir '%s': %s\n",
				argv0, path, strerror(errsv));
		return -1;
	}

	/* We need to reset errno here, because it is the only way to
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

		newpath = malloc(sizeof(*newpath) *
				(strlen(path) + strlen(d->d_name) + 2));
		if (newpath == NULL)
			outofmem(EXIT_FAILURE);
		sprintf(newpath, "%s/%s", path, d->d_name);

		add_path(newpath, recurse);

		free(newpath);
	}
	closedir(dirp);
	if ((errsv = errno) != 0) {
		fprintf(stderr, "%s: stopping readdir '%s': %s\n",
				argv0, path, strerror(errsv));
		return -1;
	}

	return 0;
}

/*
 * Append the file at path to filel. If path is a directory, and recurse is
 * nonzero, then all files in path will be added to filel through
 * add_directory(). Return value shall be 0 on success, and -1 on error. Upon
 * irreconciliable errors, such as running out of memory, the program shall be
 * exited with code EXIT_FAILURE.
 */
static int
add_path(const char *path, const int recurse)
{
	int errsv;
	char **realloc_check;
	struct stat statbuf;

	if (stat(path, &statbuf) == -1) {
		errsv = errno;
		fprintf(stderr, "%s: cannot stat '%s': %s\n",
				argv0, path, strerror(errsv));
		return -1;
	}
	if (S_ISDIR(statbuf.st_mode)) {
		if (recurse) {
			return add_directory(path, recurse);
		}
		fprintf(stderr, "%s: no -r; omitting directory '%s'\n",
				argv0, path);
		return -1;
	}

	if (!S_ISREG(statbuf.st_mode)) {
		fprintf(stderr, "%s: cannot read '%s': not a regular file\n",
				argv0, path);
		return -1;
	}

	/* Add file path to the list. */

	/* Make sure that there is enough space allocated in filel for a new
	 * pointer.
	 */
	while (filel.size < filel.used) {
		/* This 4 is arbitrary, it just seems like a good amount to
		 * increment filel.size by, balancing speed (adding only
		 * sizeof(char *) over and over again could be quite slow) and
		 * memory (a larger number could result in much more memory
		 * than needed being allocated.
		 */
		filel.size += 4*sizeof(char *);
	}

	/* Make sure that realloc is valid before reallocating the filel.
	*/
	realloc_check = realloc(filel.v, filel.size);
	if (realloc_check == NULL) {
		outofmem(EXIT_FAILURE);
	} else {
		filel.v = realloc_check;
	}

	/* Allocate space for file path. */
	filel.v[filel.amt] =
		malloc((1+strlen(path))*sizeof(char));
	if (filel.v[filel.amt] == NULL) {
		outofmem(EXIT_FAILURE);
	}
	filel.used += sizeof(char *);

	/* Add the file path! */
	strcpy(filel.v[filel.amt], path);
	filel.amt++;

	return 0;
}

/*
 * Move to the 0-indexed 'new'-th buffer. That is, change_buffer(0) will switch
 * to the first buffer, etc. If the operation is successful, meaning the new
 * value is in the range of possible indices, then 0 is returned; otherwise,
 * the value that bufl.n would have been set to is returned.
 */
static int
change_buffer(const int new)
{
	if (new >= 0 && new < bufl.amt) {
		bufl.n = new;
		display_buffer(&bufl.v[bufl.n]);
		return 0;
	}

	return new;
}

/*
 * Cleans up terminal settings and the like which were modified by navipage.
 * Usually called before exiting the program.
 */
static void
cleanup(void)
{
	system("stty sane");
	showcursor();
	/* Return the shell prompt to the beginning of the next line. */
	putchar('\n');
}

/*
 * Compares two file paths by their basename. Of importance to us is that files
 * named in YYYYMMDD format are compared such that the file named with the
 * further date is "less than" the other path. Return value shall be the
 * negative of what strcmp() would return given the basename of the paths.
 * Upon irreconciliable errors, such as running out of memory, the program
 * shall be exited with code EXIT_FAILURE.
 */
static int
compare_path_basenames(const void *p1, const void *p2)
{
	/* POSIX-compliant basename() may modify the path variable, which we
	 * don't want. For this reason, we copy the string before passing it to
	 * basename().
	 */
	char *base[2], *copy[2];

	/* Introducing variable 'ret' to store the result of strcmp().
	 * Returning the result of `-strcmp(base[0], base[1]);` at the end of
	 * the function after `free()` leads to use-after-free bug.
	 */
	int ret;

	/* p1 and p2 are pointers to pointers to char, but strcmp() arguments
	 * are "pointers to char", hence the following cast plus dereference.
	 * (Paraphrasing `man 3 qsort`)
	 */
	copy[0] = strdup(*(const char **)p1);
	copy[1] = strdup(*(const char **)p2);
	if (copy[0] == NULL || copy[1] == NULL) outofmem(EXIT_FAILURE);

	base[0] = basename(copy[0]);
	base[1] = basename(copy[1]);

	ret = -strcmp(base[0], base[1]);

	free(copy[0]);
	free(copy[1]);

	return ret;
}

/*
 * Display all text from b->st[b->top] to the end of the screen.
 */
static void
display_buffer(const Buffer *const b)
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
		/* Find the location of the end of the line, or if an eol
		 * cannot be found, then it is the last line in the file and we
		 * should find the eof.
		 */
		if ((eolptr = strchr(b->st[b->top + i], '\n')) == NULL) {
			eolptr = strchr(b->st[b->top + i], '\0');
		}
		linelen = eolptr - b->st[b->top + i] + 1;
		if (flags.numbers) {
			/* Print the line number at the start of each line. */
			printf("%3d ", b->top + i + 1);
		}
		fwrite(b->st[b->top + i], sizeof(char), linelen, stdout);
	}
	/* Print status-bar information. */
	gotoxy(1, rows);
	printf("#%d/%d  %s  %s",
			bufl.n + 1, bufl.amt, filel.v[bufl.n],
			"Press 'i' for help.");
	fflush(stdout);
}

/*
 * Fill the buffer with an error message designated by the arguments.
 */
static void
error_buffer(Buffer *const b, const char *format, ...)
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
	const color_code EC_COLOR = YELLOW; /* EC short for execute_command */
	char *line;

	/* Clear the status line before showing the command prompt. */
	gotoxy(1, rows);
	setString("\033[2K"); /* VT100 escape code */

	/* Re-enable displaying user input. */
	system("stty echo");

	showcursor();

	setColor(EC_COLOR); /* Display readline prompt in EC_COLOR */
	if ((line = readline("!")) != NULL) {
		resetColor();
		fflush(stdout);
		system(line);
		free(line);
	}

	system("stty -echo");
	gotoxy(1, rows);
	/* -1 means to use the current background color. */
	colorPrint(EC_COLOR, -1, "navipage: press any key to return.");
	fflush(stdout);
	anykey(NULL);

	hidecursor();
	resetColor();
	display_buffer(&bufl.v[bufl.n]);
	/* fflush(stdout) -- unneeded, is ran at the end of display_buffer() */
}

/*
 * Handle signals.
 */
static void
handle_signals(int sig)
{
	switch (sig) {
	case SIGHUP:
		quit(EXIT_FAILURE);
		break;
	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
	default:
		quit(EXIT_SUCCESS);
		break;
	}
}

/*
 * Display helpful information in the following order, with the next option
 * being tried if the first fails:
 * 1. man 1 navipage
 * 2. man ./navipage.1
 * 3. less README.md
 * 4. Displaying a link to URL
 */
static void
info(void)
{
	if (system("man 1 navipage")   == 0) return;
	if (system("man ./navipage.1") == 0) return;
	if (system("less README.md")   == 0) return;

	gotoxy(1, rows);
	setString("\033[2K"); /* VT100 escape code; clears the status line */
	/* -1 means to use the current background color. */
	colorPrint(YELLOW, -1, "Find help online at <" URL ">.");
	fflush(stdout);
}

/*
 * Read the file at path into b, and set various values of b, like length,
 * top, offset, etc. Returns 0 on success, -1 on error.
 * TODO: Change this function to return a 'Buffer *' and NULL on error.
 */
static int
init_buffer(Buffer *const b, const char *const path)
{
	FILE *fp;
	char *errfunc;
	long i;

	/* Spaces are intentionally used for alignment here because this is an
	 * odd expression and formatting the usual way with tabs looks worse.
	 */
	if ( (errfunc = "fopen", (fp = fopen(path, "r")) == NULL) ||
	     (errfunc = "fseek", fseek(fp, 0L, SEEK_END) == -1)   ||
	     (errfunc = "ftell", (b->length = ftell(fp)) == -1) ) {
		error_buffer(b, "%s: cannot %s '%s': %s\n",
				argv0, errfunc, path, strerror(errno));
		return -1;
	}

	rewind(fp);
	b->size = b->length + 1;
	if ((b->text = malloc(sizeof(*b->text) * b->size)) == NULL)
		outofmem(EXIT_FAILURE);
	if (fread(b->text, sizeof(char), b->length, fp) != (size_t)b->length) {
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

	/* We will increment memory in blocks of 10. That is, every time we
	 * reallocate memory because there is not enough room, we will add
	 * enough memory to fit ten more char pointers.
	 */
	b->st_size = ST_SIZE_INCR;
	if ((b->st = malloc(sizeof(*b->st) * b->st_size)) == NULL)
		outofmem(EXIT_FAILURE);

	/* The first line starts at the first character of the text, so we
	 * start there.
	 */
	b->st[0] = b->text;
	b->st_amt = 1;

	for (i = 1; i < b->length; i++) {
		if (b->text[i - 1] != '\n')
			continue;

		/* b->text[i] is the first character of a line, so append
		 * &b->text[i] to b->st.
		 */
		while (b->st_size <= b->st_amt) {
			char **realloc_check;

			b->st_size += ST_SIZE_INCR;
			realloc_check = realloc(b->st,
					sizeof(*b->st) * b->st_size);
			if (realloc_check == NULL)
				outofmem(EXIT_FAILURE);
			else
				b->st = realloc_check;
		}

		b->st[b->st_amt++] = &b->text[i];
	}

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
			change_buffer(bufl.n - 1);
			break;
		case 'H':
			/* Move to the first buffer. */
			change_buffer(0);
			break;
		case 'i':
			info();
			break;
		case 'j':
		case CTRL_E:
			/* Scroll down one line. */
			scroll(1);
			break;
		case 'k':
		case CTRL_Y:
			/* Scroll up one line. */
			scroll(-1);
			break;
		case 'l':
			/* Move to the next-less-recent buffer. */
			change_buffer(bufl.n + 1);
			break;
		case 'L':
			/* Move to the last buffer. */
			change_buffer(bufl.amt - 1);
			break;
		case 'N':
			toggle_numbers();
			break;
		case 'q':
			quit(EXIT_SUCCESS);
			break;
		case 'r':
			redraw();
			break;
		case '!':
			execute_command();
			break;
		}
	}
}

/*
 * Print a line that we are "out of memory" and exit navipage. This is usually
 * called in the case of a malloc error.
 * TODO: Write something better and less ad-hoc than this, using strerror() and
 * related functions. Maybe just use BSD errx functions?
 */
static void
outofmem(const int code)
{
	fprintf(stderr, "%s: error: out of memory\n", argv0);
	exit(code);
}

/*
 * Quit navipage.
 */
static void
quit(const int code)
{
	cleanup();
	exit(code);
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
 * Scroll by 'offset' lines in the buffer. On success, 0 is returned;
 * otherwise the line number that would have been made the top of the screen
 * is returned.
 */
static int
scroll(const int offset)
{
	int newtop;

	newtop = bufl.v[bufl.n].top + offset;

	/* newtop must be >= 0 because it will be used as an array index.
	 * newtop must be < bufl.v[bufl.n].st_amt - rows + 2 because if it
	 * isn't, then we will have a buffer overrun of bufl.v[bufl.n].st. See
	 * display_buffer() for a better understanding of this.
	 */
	if (newtop < 0 || newtop >= bufl.v[bufl.n].st_amt - rows + 2)
		return newtop;

	bufl.v[bufl.n].top = newtop;
	display_buffer(&bufl.v[bufl.n]);
	return 0;
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
 * Toggle whether or not to print line numbers.
 */
static void
toggle_numbers(void)
{
	flags.numbers = !flags.numbers;
	display_buffer(&bufl.v[bufl.n]);
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
	puts(USAGE);
}

/*
 * Print what version of navipage this is.
 */
static void
version(void)
{
	puts("navipage " VERSION);
}

int
main(int argc, char *argv[])
{
	int c, i;
	char *envstr;
	struct sigaction sa;

	if (sa.sa_handler = handle_signals,
			sigaction(SIGINT, &sa, NULL)  == -1 ||
			sigaction(SIGTERM, &sa, NULL) == -1 ||
			sigaction(SIGQUIT, &sa, NULL) == -1 ||
			sigaction(SIGHUP, &sa, NULL)  == -1) {
		fprintf(stderr, "%s: cannot sigaction: %s\n",
				argv[0], strerror(errno));
		return EXIT_FAILURE;
	}

	argv0 = argv[0];
	filel.size = 4*sizeof(char *);
	filel.used = 0;
	if ((filel.v = malloc(filel.size)) == NULL) {
		outofmem(EXIT_FAILURE);
	}
	update_rows();

	/* Handle options. */
	while ((c = getopt(argc, argv, "dhnrsv")) != -1) {
		switch (c) {
		case 'd':
			flags.debug = 1;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case 'n':
			flags.numbers = 1;
			break;
		case 'r':
			flags.recurse_more = 1;
			break;
		case 's':
			flags.sh = 1;
			break;
		case 'v':
			version();
			exit(EXIT_SUCCESS);
			break;
		case ':':
		case '?':
			usage();
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Set argc to the amount of arguments remaining, and point argv to the
	 * first argument after the options.
	 */
	argc -= optind;
	argv += optind;

	if (flags.sh && (envstr = getenv("NAVIPAGE_SH")) != NULL) {
		/* Run $NAVIPAGE_SH before reading files. */
		system(envstr);
	}

	if (argc == 0 && (envstr = getenv("NAVIPAGE_DIR")) != NULL) {
		/* Add the files at $NAVIPAGE_DIR to filel. */
		add_path(envstr, RECURSE);
	}

	/* All remaining arguments are paths to files to be read. */
	for (i = 0; i < argc; i++) {
		add_path(argv[i], flags.recurse_more);
	}

	/* Exit the program if no files were read. */
	if (filel.amt == 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	qsort(filel.v, filel.amt, sizeof(char *), compare_path_basenames);

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
	 * prevents all the 'j's and 'k's from showing on the bottom of the
	 * screen.
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
