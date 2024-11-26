CC = clang
CFLAGS = -g
OBJ_SERVER = server.o
OBJ_CLIENT = client.o

all: server client

server: $(OBJ_SERVER)
	$(CC) $(CFLAGS) $^ -o $@

client: $(OBJ_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c pool.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf *.o server client

rebuild: clean all
