CC = clang
FLAGS = -Wall -Werror -Wpedantic

all:
	$(CC) $(FLAGS) client.c -o client
	$(CC) $(FLAGS) server.c -o server
run:
	./server data.base
clean: 
	rm -rf client server
trace:
	strace -f ./server data.base
rerun:
	make clean
	make