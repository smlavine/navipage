include config.mk

CFILES = main.c run.c
HFILES = rogueutil.h navipage.h

all: options navipage

options:
	@echo "CFLAGS      = $(CFLAGS)"
	@echo "OPTIMFLAGS  = $(OPTIMFLAGS)"
	@echo "DEBUGFLAGS  = $(DEBUGFLAGS)"

navipage: $(CFILES) $(HFILES)
	$(CC) -o $@ $(CFILES) $(CFLAGS) $(OPTIMFLAGS)

debug: $(CFILES) $(HFILES)
	$(CC) -o navipage-$@ $(CFILES) $(CFLAGS) $(DEBUGFLAGS)

