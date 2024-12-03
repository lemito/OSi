CC = clang

all: clean main lib1 lib2
clean:
	rm -rf *.out *.o
lib1:
	$(CC) -o blocks2n.so -fPIC -shared blocks2n.c
lib2:
	$(CC) -o buddys.so -fPIC -shared buddys.c

main:
	$(CC) -o main -ldl main.c 
