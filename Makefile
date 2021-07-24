.POSIX:

include config.mk

CFILES = main.c
HFILES = rogueutil.h

all: options navipage

options:
	@echo "CFLAGS      = $(CFLAGS)"
	@echo "OPTIMFLAGS  = $(OPTIMFLAGS)"
	@echo "DEBUGFLAGS  = $(DEBUGFLAGS)"

navipage: $(CFILES) $(HFILES)
	$(CC) -o $@ $(CFILES) $(CFLAGS) $(OPTIMFLAGS)

debug: $(CFILES) $(HFILES)
	$(CC) -o navipage-$@ $(CFILES) $(CFLAGS) $(DEBUGFLAGS)

clean:
	rm -f navipage navipage-debug

install: navipage
	mkdir -p $(PREFIX)/bin
	cp -f navipage $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin
	mkdir -p $(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < navipage.1 > $(MANPREFIX)/man1/navipage.1
	chmod 644 $(MANPREFIX)/man1/navipage.1

uninstall:
	rm -f $(PREFIX)/bin/navipage\
		$(MANPREFIX)/man1/navipage.1

.PHONY: all options navipage debug clean install uninstall
