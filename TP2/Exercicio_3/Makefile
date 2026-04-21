CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

all: servidor_estatistica cliente_estatistica

servidor_estatistica: servidor_estatistica.c
	$(CC) $(CFLAGS) -o servidor_estatistica servidor_estatistica.c

cliente_estatistica: cliente_estatistica.c
	$(CC) $(CFLAGS) -o cliente_estatistica cliente_estatistica.c

clean:
	rm -f servidor_estatistica cliente_estatistica