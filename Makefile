CFLAGS=-g -O0
ts: main.o server.o server_start.o client.o msgdump.o jobs.o execute.o
	gcc -o ts $^
