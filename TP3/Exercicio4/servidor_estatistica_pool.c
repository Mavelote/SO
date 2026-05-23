#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <endian.h> 
#include <limits.h>

#include "../Exercicio2/thread_pool.h"

#define PORT 8080
#define SOCKET_PATH "/tmp/so_tp3_ex4.sock"
#define NUM_WORKER_THREADS 4

typedef struct {
    unsigned long tcp_connections;
    unsigned long unix_connections;
    unsigned long total_vector_elements;
    pthread_mutex_t stat_mutex;
} ServerStats;

ServerStats global_stats;

typedef struct {
    int client_fd;
    int is_tcp; 
} ClientData;

ssize_t read_all(int fd, void *buffer, size_t n) {
    size_t bytes_read = 0;
    while (bytes_read < n) {
        ssize_t res = read(fd, (char *)buffer + bytes_read, n - bytes_read);
        if (res <= 0) return res; 
        bytes_read += res;
    }
    return bytes_read;
}

void *print_statistics_func(void *arg) {
    (void)arg; 
    
    while (1) {
        sleep(1); 
        
        pthread_mutex_lock(&global_stats.stat_mutex);
        
        unsigned long total_conn = global_stats.tcp_connections + global_stats.unix_connections;
        double avg_size = 0.0;
        
        if (total_conn > 0) {
            avg_size = (double)global_stats.total_vector_elements / total_conn;
        }

        printf("\n--- ESTATISTICAS DO SERVIDOR ---\n");
        printf("Ligacoes TCP:  %lu\n", global_stats.tcp_connections);
        printf("Ligacoes UNIX: %lu\n", global_stats.unix_connections);
        printf("Dimensao media dos vetores: %.2f elementos\n", avg_size);
        printf("--------------------------------\n");
        
        pthread_mutex_unlock(&global_stats.stat_mutex);
    }
    return NULL;
}

void handle_client(void *arg) {
    ClientData *cdata = (ClientData *)arg;
    int client_fd = cdata->client_fd;
    int is_tcp = cdata->is_tcp;
    free(cdata); 

    pthread_mutex_lock(&global_stats.stat_mutex);
    if (is_tcp) global_stats.tcp_connections++;
    else global_stats.unix_connections++;
    pthread_mutex_unlock(&global_stats.stat_mutex);

    uint16_t *vector = NULL;
    uint32_t total_elements = 0;
    uint8_t status = 0; 

    while (1) {
        uint32_t net_block_size;
        if (read_all(client_fd, &net_block_size, sizeof(net_block_size)) <= 0) {
            status = 1; break; 
        }

        uint32_t block_bytes = ntohl(net_block_size);
        if (block_bytes == 0) break; 

        if (block_bytes % sizeof(uint16_t) != 0) {
            status = 1; break;
        }

        uint32_t new_elements = block_bytes / sizeof(uint16_t);
        
        uint16_t *temp = realloc(vector, (total_elements + new_elements) * sizeof(uint16_t));
        if (!temp) {
            status = 2; break; 
        }
        vector = temp;

        if (read_all(client_fd, vector + total_elements, block_bytes) <= 0) {
            status = 1; break;
        }

        for (uint32_t i = 0; i < new_elements; i++) {
            vector[total_elements + i] = ntohs(vector[total_elements + i]);
        }
        total_elements += new_elements;
    }

    if (total_elements == 0 && status == 0) status = 1;

    if (status == 0) {
        pthread_mutex_lock(&global_stats.stat_mutex);
        global_stats.total_vector_elements += total_elements;
        pthread_mutex_unlock(&global_stats.stat_mutex);
       
        uint16_t global_min = UINT16_MAX;
        uint16_t global_max = 0;
        uint64_t global_sum = 0;

        for (uint32_t i = 0; i < total_elements; i++) {
             global_sum += vector[i];
             if (vector[i] > global_max) global_max = vector[i];
             if (vector[i] < global_min) global_min = vector[i];
        }

        // VERIFICAÇÃO DE ERROS NO WRITE:
        if (write(client_fd, &status, 1) < 0) perror("Erro no write status");
        
        uint16_t net_min = htons(global_min);
        uint16_t net_max = htons(global_max);
        uint64_t net_sum = htobe64(global_sum); 

        if (write(client_fd, &net_min, sizeof(net_min)) < 0) perror("Erro min");
        if (write(client_fd, &net_max, sizeof(net_max)) < 0) perror("Erro max");
        if (write(client_fd, &net_sum, sizeof(net_sum)) < 0) perror("Erro sum");
        
    } else {
        if (write(client_fd, &status, 1) < 0) perror("Erro no write erro");
        char *err_msg = (status == 1) ? "Pedido invalido" : "Erro interno";
        uint32_t err_len_net = htonl(strlen(err_msg) + 1);
        
        if (write(client_fd, &err_len_net, sizeof(err_len_net)) < 0) perror("Erro len");
        if (write(client_fd, err_msg, strlen(err_msg) + 1) < 0) perror("Erro msg");
    }

    free(vector); 
    close(client_fd);
}

int max(int a, int b) { return (a > b) ? a : b; }

int main(void) {
    memset(&global_stats, 0, sizeof(ServerStats));
    pthread_mutex_init(&global_stats.stat_mutex, NULL);

    pthread_t stats_thread;
    pthread_create(&stats_thread, NULL, print_statistics_func, NULL);
    pthread_detach(stats_thread); 

    thread_pool_t *pool = thread_pool_create(NUM_WORKER_THREADS);

    int server_fd_unix, server_fd_inet;
    struct sockaddr_un addr_unix;
    struct sockaddr_in addr_inet;

    server_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(SOCKET_PATH);
    memset(&addr_unix, 0, sizeof(addr_unix));
    addr_unix.sun_family = AF_UNIX;
    strncpy(addr_unix.sun_path, SOCKET_PATH, sizeof(addr_unix.sun_path) - 1);
    bind(server_fd_unix, (struct sockaddr*)&addr_unix, sizeof(addr_unix));
    listen(server_fd_unix, 5);

    server_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_inet, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr_inet, 0, sizeof(addr_inet));
    addr_inet.sin_family = AF_INET;
    addr_inet.sin_addr.s_addr = INADDR_ANY;
    addr_inet.sin_port = htons(PORT);
    bind(server_fd_inet, (struct sockaddr*)&addr_inet, sizeof(addr_inet));
    listen(server_fd_inet, 5);

    fd_set readfds; 
    int max_sd = max(server_fd_unix, server_fd_inet);
    
    printf("Super Servidor MULTIPLEXADO com Thread Pool a correr...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd_unix, &readfds);
        FD_SET(server_fd_inet, &readfds);

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            break; 
        }

        int new_socket = -1;
        int is_tcp_conn = 0;

        if (FD_ISSET(server_fd_unix, &readfds)) {
            new_socket = accept(server_fd_unix, NULL, NULL);
            is_tcp_conn = 0;
        }
        else if (FD_ISSET(server_fd_inet, &readfds)) {
            new_socket = accept(server_fd_inet, NULL, NULL);
            is_tcp_conn = 1;
        }

        if (new_socket >= 0) {
            ClientData *cdata = malloc(sizeof(ClientData));
            cdata->client_fd = new_socket;
            cdata->is_tcp = is_tcp_conn;
            
            thread_pool_submit(pool, handle_client, cdata);
        }
    }

    thread_pool_destroy(pool);
    close(server_fd_unix);
    close(server_fd_inet);
    unlink(SOCKET_PATH);
    
    return 0;
}