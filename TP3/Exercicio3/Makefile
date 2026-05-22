CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -O2
LDFLAGS = -pthread

# Caminhos relativos para os outros exercícios (pastas ao lado)
DIR_EX1 = ../Exercicio_1
DIR_EX2 = ../Exercicio_2

# Ficheiros objeto necessários para construir o executável
OBJS = vetor_pool.o $(DIR_EX1)/countdown.o $(DIR_EX2)/thread_pool.o
TARGET = vetor_pool

all: $(TARGET)

# Compila o executável final juntando todas as peças
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Regra genérica para compilar qualquer ficheiro .c num .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpa os ficheiros temporários (.o) e o executável
clean:
	rm -f *.o $(DIR_EX1)/*.o $(DIR_EX2)/*.o $(TARGET)

.PHONY: all clean