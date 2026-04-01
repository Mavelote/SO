# Variáveis para o compilador e as flags 
CC = gcc
CFLAGS = -Wall -Wextra -g

# Nome do ficheiro executável final
TARGET = ex1

# Regra principal
all: $(TARGET)

# Como construir o executável a partir do teu código fonte
$(TARGET): EX1.C
	$(CC) $(CFLAGS) -o $(TARGET) EX1.C

# Regra para apagar o executável e limpar a pasta
clean:
	rm -f $(TARGET)