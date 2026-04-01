# Variáveis para o compilador e as flags 
CC = gcc
CFLAGS = -Wall -Wextra -g

# Regra principal
all: ex1 count_words

# Como construir o executável a partir do teu código fonte
ex1: EX1.C
	$(CC) $(CFLAGS) -o ex1 EX1.C

count_words: count_words.c
	$(CC) $(CFLAGS) -o count_words count_words.c

# Regra para apagar o executável e limpar a pasta
clean:
	rm -f ex1 count_words