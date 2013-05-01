CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=DMON.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=DMON
MANDIR = /usr/share/man/man1
DESTDIR =

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

install-man:
	install -Dm644 DMON.1 "$(DESTDIR)$(MANDIR)/DMON.1"
	
	
install: install-man
