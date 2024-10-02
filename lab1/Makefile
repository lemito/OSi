CC = clang
FLAGS = -Wall -Werror -Wpedantic

all:
	$(CC) $(FLAGS) posix_ipc-example-client.c -o posix_ipc-example-client
	$(CC) $(FLAGS) posix_ipc-example-server.c -o posix_ipc-example-server
	make run
run:
	./posix_ipc-example-server data.base
clean: 
	rm -rf posix_ipc-example-client posix_ipc-example-server
trace:
	strace -f ./posix_ipc-example-server data.base
rerun:
	make clean
	make