CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -O2
LDFLAGS = -pthread

# Caminhos relativos para a biblioteca do Thread Pool
DIR_EX2 = ../Exercicio_2

# Ficheiros objeto necessários
OBJS = servidor_estatistica_pool.o $(DIR_EX2)/thread_pool.o
TARGET = servidor_estatistica_pool

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(DIR_EX2)/*.o $(TARGET)

.PHONY: all clean