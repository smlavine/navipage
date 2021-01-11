# navipage version
VERSION = 0.3

# paths
PREFIX = /usr
#MANPREFIX = $(PREFIX)/share/man

#includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc

# flags
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS = -std=c99 -pedantic -Wall -Wextra $(INCS) $(CPPFLAGS)
OPTIMFLAGS = -O3
DEBUGFLAGS = -g -Og

# compiler and linker
CC = cc
