.POSIX:

include config.mk

SRC = err.c main.c
OBJ = $(SRC:.c=.o)

all: options navipage

options:
	@echo 'navipage build options:'
	@echo "CFLAGS      = $(CFLAGS)"
	@echo "DEBUGFLAGS  = $(DEBUGFLAGS)"
	@echo "CC          = $(CC)"

err.o: err.h

navipage.o: rogueutil.h

$(OBJ): config.mk

navipage: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f navipage $(OBJ)

install: all
	mkdir -p $(PREFIX)/bin
	cp -f navipage $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin
	mkdir -p $(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < navipage.1 > $(MANPREFIX)/man1/navipage.1
	chmod 644 $(MANPREFIX)/man1/navipage.1

uninstall:
	rm -f $(PREFIX)/bin/navipage $(MANPREFIX)/man1/navipage.1

.PHONY: all options navipage clean install uninstall
