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
ifdef DEBUG
	CFLAGS += -ggdb -O0
endif
LDFLAGS = -L$(PREFIX)/lib $(LIBS)

# compiler and linker
CC = cc
