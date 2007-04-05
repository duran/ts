PREFIX?=/usr/local
GCCFLAGS=-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED -D__STRICT_ANSI__
CFLAGS=-pedantic -ansi -Wall -g -O0 ${GCCFLAGS}
OBJECTS=main.o \
	server.o \
	server_start.o \
	client.o \
	msgdump.o \
	jobs.o \
	execute.o \
	msg.o \
	client_run.o \
	mail.o \
	error.o \
	signals.o
INSTALL=/usr/bin/install -c

all: ts

ts: $(OBJECTS)
	gcc -o ts $^

# Dependencies
main.o: main.c main.h
server_start.o: server_start.c main.h
server.o: server.c main.h msg.h
client.o: client.c main.h msg.h
msgdump.o: msgdump.c main.h msg.h
jobs.o: jobs.c main.h msg.h
execute.o: execute.c main.h msg.h
msg.o: msg.c main.h msg.h
client_run.o: client_run.c main.h
mail.o: mail.c main.h
error.o: error.c main.h msg.h
signals.o: signals.c main.h

clean:
	rm -f *.o ts

install:
	$(INSTALL) -d $(PREFIX)/bin
	$(INSTALL) ts $(PREFIX)/bin
	$(INSTALL) -d $(PREFIX)/man/man1
	$(INSTALL) -m 644 ts.1 $(PREFIX)/man/man1
