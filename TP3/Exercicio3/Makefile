CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -O2
LDFLAGS = -pthread

DIR_EX1 = ../Exercicio1
DIR_EX2 = ../Exercicio2

OBJS = vector_pool.o $(DIR_EX1)/countdown.o $(DIR_EX2)/thread_pool.o
TARGET = vector_pool

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(DIR_EX1)/*.o $(DIR_EX2)/*.o $(TARGET)

.PHONY: all clean