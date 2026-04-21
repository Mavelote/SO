#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <arpa/inet.h> 

#define SOCKET_PATH "/tmp/so_tp2_ex1.sock"
#define BUFFER_SIZE 1024

void processar_cliente(int client_fd){
    uint8_t pedido, status = 0;

    if(read(client_fd, &pedido, 1) <= 0){
        close(client_fd);
        return;
    }

    if(pedido != 1 && pedido != 2){
        status = 1;
        write(client_fd, &status, 1);
    }
    if(status != 0){
        close(client_fd);
        return;
    }

    int pipefd[2];
    if(pipe(pipefd) == -1){
        perror("Erro no pipe");
        close(client_fd);
        return;
    }

    pid_t pid = fork();
    if(pid == 0){
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if(pedido == 1){
            execlp("lscpu", "lscpu", NULL);
        }else{
            execlp("free", "free", "-h", NULL);
        }
        exit(EXIT_FAILURE);
    }else{

        close(pipefd[1]);
        
        char buffer[BUFFER_SIZE];
        ssize_t bytes_lidos;

        while((bytes_lidos = read(pipefd[0], buffer, BUFFER_SIZE)) > 0){
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

int main(){
    int server_fd, client_fd;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Erro a criar socket");
        exit(EXIT_FAILURE);
    }

    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Erro no listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor UNIX a escutar em %s...\n", SOCKET_PATH);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) continue;
        
        
        processar_cliente(client_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
    
    
}