#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <1 (CPUINFO) | 2 (MEMINFO)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint8_t pedido = atoi(argv[1]);
    
    // Criar socket AF_INET
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Erro a criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar a ligação ao IP e Porto do servidor
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    
    // Converter o IP em texto para formato binário de rede
    if (inet_pton(AF_INET, SERVER_IP, &addr.sin_addr) <= 0) {
        perror("Endereço IP inválido");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Erro a ligar ao servidor");
        exit(EXIT_FAILURE);
    }

    
    write(sockfd, &pedido, 1);

    uint8_t status;
    if (read(sockfd, &status, 1) <= 0) {
        perror("Erro a ler status");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (status == 1) {
        printf("Erro: Serviço inválido (status 1).\n");
    } else if (status == 2) {
        printf("Erro na execução do serviço (status 2).\n");
    } else if (status == 0) {
        uint32_t tamanho_bloco;
        char buffer[1024];

        while (1) {
            if (read(sockfd, &tamanho_bloco, sizeof(tamanho_bloco)) <= 0) break;
            tamanho_bloco = ntohl(tamanho_bloco);
            
            if (tamanho_bloco == 0) break;

            ssize_t bytes_lidos = 0;
            while (bytes_lidos < tamanho_bloco) {
                ssize_t n = read(sockfd, buffer + bytes_lidos, tamanho_bloco - bytes_lidos);
                if (n <= 0) break;
                bytes_lidos += n;
            }
            write(STDOUT_FILENO, buffer, bytes_lidos);
        }
    }

    close(sockfd);
    return 0;
}