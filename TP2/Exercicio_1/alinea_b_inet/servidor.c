#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void processar_cliente(int client_fd) {
    uint8_t pedido, status = 0;
    
    if (read(client_fd, &pedido, 1) <= 0) {
        close(client_fd);
        return;
    }

    if (pedido != 1 && pedido != 2) status = 1;
    write(client_fd, &status, 1);

    if (status != 0) {
        close(client_fd);
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Erro no pipe");
        close(client_fd);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO); 
        close(pipefd[1]);

        if (pedido == 1) execlp("lscpu", "lscpu", NULL);
        else execlp("free", "free", "-h", NULL);
        
        exit(EXIT_FAILURE); 
    } else {
        close(pipefd[1]); 
        char buffer[BUFFER_SIZE];
        ssize_t bytes_lidos;
        
        while ((bytes_lidos = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            uint32_t tamanho_bloco = htonl((uint32_t)bytes_lidos);
            write(client_fd, &tamanho_bloco, sizeof(tamanho_bloco));
            write(client_fd, buffer, bytes_lidos);
        }
        
        uint32_t fim = 0;
        write(client_fd, &fim, sizeof(fim));
        
        close(pipefd[0]);
        waitpid(pid, NULL, 0); 
    }
    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;

    // Criar socket do domínio Internet
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Erro a criar socket");
        exit(EXIT_FAILURE);
    }

    // reiniciar o servidor
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configurar o endereço e o porto
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(PORT);      

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Erro no listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor Internet (TCP) a escutar no porto %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) continue;
        
        processar_cliente(client_fd);
    }

    close(server_fd);
    return 0;
}