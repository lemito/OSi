all:
	clang -g server.c -o server
	clang -g client.c -o client
	./server data.base

clean:
	rm -rf *.out *.o

rebuild: clean all