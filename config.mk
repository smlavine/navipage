# navipage version
VERSION = 0.8.0

# paths
PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

#includes and libs
INCS = -I$(PREFIX)/include
LIBS = -lreadline

# flags
CPPFLAGS = $(INCS) -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic
#uncomment for debugging
#CFLAGS = -g -Og -std=c99 -fsanitize=address -Wall -Wextra -Wpedantic
LDFLAGS = -L$(PREFIX)/lib $(LIBS)

# compiler and linker
CC = cc
