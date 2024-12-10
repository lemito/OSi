CC = clang

all: clean main lib1 lib2
clean:
	rm -rf *.out *.o
lib1:
	$(CC) -o blocks2n.so -fPIC -shared blocks2n.c -lm
lib2:
	$(CC) -o buddys.so -fPIC -shared buddys.c -lm

main:
	$(CC) -o main.out -ldl main.c 
