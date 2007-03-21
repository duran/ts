CFLAGS=-g
test: main.o server.o server_start.o client.o
	gcc -o test $^
