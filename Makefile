CFLAGS=-g -O0
test: main.o server.o server_start.o client.o msgdump.o jobs.o execute.o
	gcc -o test $^
