CFLAGS=-g -O0
OBJECTS=main.o \
	server.o \
	server_start.o \
	client.o \
	msgdump.o \
	jobs.o \
	execute.o \
	msg.o \
	tail.o


ts: $(OBJECTS)
	gcc -o ts $^

clean:
	rm -f *.o ts
