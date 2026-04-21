#define _GNU_SOURCE

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
#include <limits.h>

#define PORT 8080
#define NUM_WORKER_THREADS 4


typedef struct {
    uint16_t *vector;
    unsigned long start;
    unsigned long end;
    uint16_t smaller;
    uint16_t bigger;
    uint64_t sum;
} WorkerData;


ssize_t read_all(int fd, void *buffer, size_t n) {
    size_t bytes_read = 0;
    while (bytes_read < n) {
        ssize_t res = read(fd, (char *)buffer + bytes_read, n - bytes_read);
        if (res <= 0) return res; 
        bytes_read += res;
    }
    return bytes_read;
}


// Calculam as estatísticas
void *worker_thread_func(void *arg) {
    WorkerData *data = (WorkerData *)arg;
    
    uint16_t loc_smaller = data->vector[data->start];
    uint16_t loc_bigger = data->vector[data->start];
    uint64_t loc_sum = data->vector[data->start];

    for (unsigned long i = data->start + 1; i < data->end; i++) {
        loc_sum += data->vector[i];
        if (data->vector[i] > loc_bigger) loc_bigger = data->vector[i];
        if (data->vector[i] < loc_smaller) loc_smaller = data->vector[i];
    }

    data->smaller = loc_smaller;
    data->bigger = loc_bigger;
    data->sum = loc_sum;
    return NULL;
}

// Recebe dados e coordena
void *client_handler(void *client_socket_ptr) {
    int client_fd = *(int *)client_socket_ptr;
    free(client_socket_ptr); 

    uint16_t *vector = NULL;
    uint32_t total_elements = 0;
    uint8_t status = 0; 

    // Receber o vetor em blocos
    while (1) {
        uint32_t net_block_size;
        if (read_all(client_fd, &net_block_size, sizeof(net_block_size)) <= 0) {
            status = 1; break; 
        }

        uint32_t block_bytes = ntohl(net_block_size);
        if (block_bytes == 0) break; 

        // Validar se os bytes são múltiplos do tamanho do uint16_t
        if (block_bytes % sizeof(uint16_t) != 0) {
            status = 1; break;
        }

        uint32_t new_elements = block_bytes / sizeof(uint16_t);
        
        // Realocar memória para o vetor crescer
        uint16_t *temp = realloc(vector, (total_elements + new_elements) * sizeof(uint16_t));
        if (!temp) {
            status = 2; break;
        }
        vector = temp;

        // Ler os dados do bloco
        if (read_all(client_fd, vector + total_elements, block_bytes) <= 0) {
            status = 1; break;
        }

        // Converter todos os elementos recebidos para a arquitetura local
        for (uint32_t i = 0; i < new_elements; i++) {
            vector[total_elements + i] = ntohs(vector[total_elements + i]);
        }
        
        total_elements += new_elements;
    }

    if (total_elements == 0 && status == 0) status = 1;

    // Processamento paralelo
    if (status == 0) {
        pthread_t workers[NUM_WORKER_THREADS];
        WorkerData w_data[NUM_WORKER_THREADS];
        
        uint32_t chunk_size = total_elements / NUM_WORKER_THREADS;
        int threads_created = 0;

        for (int i = 0; i < NUM_WORKER_THREADS; i++) {
            w_data[i].vector = vector;
            w_data[i].start = i * chunk_size;
            w_data[i].end = (i == NUM_WORKER_THREADS - 1) ? total_elements : (i + 1) * chunk_size;

            if (pthread_create(&workers[i], NULL, worker_thread_func, &w_data[i]) != 0) {
                status = 2;
                break;
            }
            threads_created++;
        }

        if (status == 0) {
            uint16_t global_min = UINT16_MAX;
            uint16_t global_max = 0;
            uint64_t global_sum = 0;

            for (int i = 0; i < threads_created; i++) {
                pthread_join(workers[i], NULL);
                global_sum += w_data[i].sum;
                if (w_data[i].bigger > global_max) global_max = w_data[i].bigger;
                if (w_data[i].smaller < global_min) global_min = w_data[i].smaller;
            }

            // Responder ao cliente em caso de sucesso
            write(client_fd, &status, 1);
            
            uint16_t net_min = htons(global_min);
            uint16_t net_max = htons(global_max);
            uint64_t net_sum = htobe64(global_sum); // Converter 8 bytes para a rede

            write(client_fd, &net_min, sizeof(net_min));
            write(client_fd, &net_max, sizeof(net_max));
            write(client_fd, &net_sum, sizeof(net_sum));
            
            printf("[SERVER] Pedido concluído. %u elementos processados.\n", total_elements);
        }
    }

    
    if (status != 0) {
        write(client_fd, &status, 1);
        char *err_msg = (status == 1) ? "Pedido invalido/truncado" : "Erro interno no servidor";
        uint32_t err_len_net = htonl(strlen(err_msg) + 1);
        
        write(client_fd, &err_len_net, sizeof(err_len_net));
        write(client_fd, err_msg, strlen(err_msg) + 1);
        printf("[SERVER] Pedido falhou com status %d.\n", status);
    }

    free(vector);
    close(client_fd);
    return NULL;
}


int main() {
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Erro no bind"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) == -1) {
        perror("Erro no listen"); exit(EXIT_FAILURE);
    }

    printf("Servidor de Estatística TCP a escutar no porto %d...\n", PORT);

    while (1) {
        int *client_fd = malloc(sizeof(int)); 
        *client_fd = accept(server_fd, NULL, NULL);
        
        if (*client_fd == -1) {
            free(client_fd);
            continue;
        }

        pthread_t client_thread;
        // Cria a thread e liberta os seus recursos assim que terminar
        pthread_create(&client_thread, NULL, client_handler, client_fd);
        pthread_detach(client_thread); 
    }

    close(server_fd);
    return 0;
}