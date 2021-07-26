# navipage version
VERSION = 0.7.0

# paths
PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

#includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc

# flags
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS = -ansi -Wall -Wextra -Wpedantic -lreadline $(INCS) $(CPPFLAGS)
OPTIMFLAGS = -O3
DEBUGFLAGS = -g -Og -fsanitize=address

# compiler and linker
CC = cc
