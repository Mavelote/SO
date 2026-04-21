#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/select.h> 

#define SOCKET_PATH "/tmp/so_tp2_ex1.sock"
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

// Função auxiliar para encontrar o maior descritor
int max(int a, int b) {
    return (a > b) ? a : b;
}

int main() {
    int server_fd_unix, server_fd_inet, client_fd;
    struct sockaddr_un addr_unix;
    struct sockaddr_in addr_inet;

    // configurar o socket UNIX igual á alinea a
    server_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(SOCKET_PATH);
    memset(&addr_unix, 0, sizeof(addr_unix));
    addr_unix.sun_family = AF_UNIX;
    strncpy(addr_unix.sun_path, SOCKET_PATH, sizeof(addr_unix.sun_path) - 1);
    bind(server_fd_unix, (struct sockaddr*)&addr_unix, sizeof(addr_unix));
    listen(server_fd_unix, 5);

    
    // configurar o socket Internet igual á alinea b
    
    server_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_inet, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr_inet, 0, sizeof(addr_inet));
    addr_inet.sin_family = AF_INET;
    addr_inet.sin_addr.s_addr = INADDR_ANY;
    addr_inet.sin_port = htons(PORT);
    bind(server_fd_inet, (struct sockaddr*)&addr_inet, sizeof(addr_inet));
    listen(server_fd_inet, 5);

    printf("Servidor MULTIPLEXADO ativo!\n");
    printf(" -> A escutar UNIX em %s\n", SOCKET_PATH);
    printf(" -> A escutar TCP no porto %d\n", PORT);


    // Loop do select para monitorizar ambos os sockets
    fd_set readfds; 
    int max_sd = max(server_fd_unix, server_fd_inet);

    while (1) {
        // Limpar o conjunto e adicionar os dois sockets do servidor
        FD_ZERO(&readfds);
        FD_SET(server_fd_unix, &readfds);
        FD_SET(server_fd_inet, &readfds);

        // O processo bloqueia aqui à espera que um dos sockets receba atividade
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            exit(EXIT_FAILURE);
        }


        // se ativar o socket UNIX, aceita a ligação e processa o cliente
        if (FD_ISSET(server_fd_unix, &readfds)) {
            client_fd = accept(server_fd_unix, NULL, NULL);
            if (client_fd >= 0) {
                printf("[NOVA LIGAÇÃO] Cliente UNIX detetado.\n");
                processar_cliente(client_fd);
            }
        }

        // se ativar o socket TCP, aceita a ligação e processa o cliente
        if (FD_ISSET(server_fd_inet, &readfds)) {
            client_fd = accept(server_fd_inet, NULL, NULL);
            if (client_fd >= 0) {
                printf("[NOVA LIGAÇÃO] Cliente TCP detetado.\n");
                processar_cliente(client_fd);
            }
        }
    }

    close(server_fd_unix);
    close(server_fd_inet);
    unlink(SOCKET_PATH);
    return 0;
}