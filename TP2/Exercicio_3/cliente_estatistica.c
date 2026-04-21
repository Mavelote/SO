#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <endian.h>

#define MAX_BLOCK_ELEMENTS 1024

typedef struct {
    int id;
    char *ip;
    int port;
    uint32_t num_elements;
} ClientThreadArgs;

void *run_client_connection(void *args){

    ClientThreadArgs *client_args = (ClientThreadArgs *)args;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("Erro a criar socket no cliente");
        pthread_exit(NULL);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client_args->port);
    inet_pton(AF_INET, client_args->ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        printf("[Cliente %d] Falha ao ligar ao servidor %s:%d\n", client_args->id, client_args->ip, client_args->port);
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("[Cliente %d] Ligado ao servidor. A gerar e enviar vetor de %u elementos...\n", client_args->id, client_args->num_elements);

    uint32_t elements_sent = 0;
    uint16_t buffer[MAX_BLOCK_ELEMENTS];

    while (elements_sent < client_args->num_elements){
        uint32_t elements_to_send = client_args->num_elements - elements_sent;
        if (elements_to_send > MAX_BLOCK_ELEMENTS){
            elements_to_send = MAX_BLOCK_ELEMENTS;
        }

        for (uint32_t i = 0; i < elements_to_send; i++){
            buffer[i] = htons((uint16_t)(rand() % 1000));
        }

        uint32_t bytes_to_send = elements_to_send * sizeof(uint16_t);
        uint32_t net_bytes_to_send = htonl(bytes_to_send);

        write(sockfd, &net_bytes_to_send, sizeof(net_bytes_to_send));
        write(sockfd, buffer, bytes_to_send);

        elements_sent += elements_to_send;
    }

    uint32_t end_block = 0;
    write(sockfd, &end_block, sizeof(end_block));

    uint8_t status;
    if(read(sockfd, &status, 1) <= 0){
        printf("[Cliente %d] Servidor fechou a ligação inesperadamente.\n", client_args->id);
        close(sockfd);
        pthread_exit(NULL);
    }

    if (status == 0){
        uint16_t min_net, max_net;
        uint32_t sum_net;

        read(sockfd, &min_net, sizeof(min_net));
        read(sockfd, &max_net, sizeof(max_net));
        read(sockfd, &sum_net, sizeof(sum_net));

        uint16_t min = ntohs(min_net);
        uint16_t max = ntohs(max_net);
        uint32_t sum = be64toh(sum_net);

        printf("[Cliente %d] SUCESSO | Min: %u | Max: %u | Soma: %u\n", client_args->id, min, max, sum);
    }
    else if (status == 1 || status == 2) { 
        uint32_t err_len_net;
        read(sockfd, &err_len_net, sizeof(err_len_net)); 
        uint32_t err_len = ntohl(err_len_net);
        
        char err_msg[1024] = {0};
        if (err_len > 0 && err_len < sizeof(err_msg)) {
            read(sockfd, err_msg, err_len); 
        }
        printf("[Cliente %d] ERRO (Status %d): %s\n", client_args->id, status, err_msg);
    }

    close(sockfd);
    pthread_exit(NULL);


}

int main(int argc, char *argv[]) {
    
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <IP Servidor> <Porto> <Num Elementos> <Num Ligações Simultâneas>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    uint32_t num_elements = (uint32_t)atol(argv[3]);
    int num_connections = atoi(argv[4]);

    if (num_connections <= 0) {
        fprintf(stderr, "O número de ligações simultâneas deve ser pelo menos 1.\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    
    pthread_t threads[num_connections];
    ClientThreadArgs args[num_connections];

    for (int i = 0; i < num_connections; i++) {
        args[i].id = i + 1;
        args[i].ip = ip;
        args[i].port = port;
        args[i].num_elements = num_elements;
        
        if (pthread_create(&threads[i], NULL, run_client_connection, &args[i]) != 0) {
            perror("Erro ao criar thread do cliente");
        }
    }

    
    for (int i = 0; i < num_connections; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Todas as %d ligações terminaram.\n", num_connections);
    return EXIT_SUCCESS;
}