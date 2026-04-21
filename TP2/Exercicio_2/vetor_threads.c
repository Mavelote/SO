#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h> 
#include <time.h>

typedef struct {
    unsigned long start;
    unsigned long end;
    int smaller;
    int bigger;
    long sum;
} ThreadData;


short *values = NULL;

void fatal_system_error(const char *errorMsg){
    perror(errorMsg);
    exit(EXIT_FAILURE);
}

void random_init(){
    srandom(2026);
}

long random_get_value(long min, long max){
    return min + random() % (max - min + 1);
}

short *vector_create_short(unsigned long dim){
    return malloc(dim * sizeof(short));
}

void vector_random_init_short(short values[], unsigned long dim) {
    for (unsigned long i = 0; i < dim; ++i) {
        values[i] = random_get_value(SHRT_MIN, SHRT_MAX);
    } 
}

//o que cada thread vai fazer
void* process_chunk(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // usar variáveis locais rápidas
    int local_smaller = values[data->start];
    int local_bigger = values[data->start];
    long local_sum = values[data->start];

    //processar o resto da fatia usando apenas as variáveis locais
    for (unsigned long i = data->start + 1; i < data->end; ++i) {
        local_sum += values[i];
        if (values[i] > local_bigger) local_bigger = values[i];
        if (values[i] < local_smaller) local_smaller = values[i];
    }
    
    // só no fim é que guardamos o resultado na memória partilhada
    data->smaller = local_smaller;
    data->bigger = local_bigger;
    data->sum = local_sum;

    return NULL;
}

int main(int argc, char *argv[]) {
    random_init();

    unsigned long max_dim = 6e8;
    int num_threads = 1;

    if(argc > 1) max_dim = atol(argv[1]);
    if(argc > 2) num_threads = atoi(argv[2]);

    if (num_threads <= 0){
        fprintf(stderr, "O número de threads deve ser maior que 0.\n");
        return EXIT_FAILURE;
    }

    values = vector_create_short(max_dim);
    if (values == NULL) fatal_system_error("Erro ao alocar memória.");

    vector_random_init_short(values, max_dim);

    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    unsigned long chunk_size = max_dim / num_threads;

    for (int i = 0; i < num_threads; i++){
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == num_threads - 1) ? max_dim : (i + 1) * chunk_size;

        if (pthread_create(&threads[i], NULL, process_chunk, &thread_data[i]) != 0) {
            fatal_system_error("Erro ao criar thread.");
        }
    }

    int global_smaller = SHRT_MAX;
    int global_bigger = SHRT_MIN;
    long global_sum = 0;

    for (int i = 0; i < num_threads; i++) {
    
        if (pthread_join(threads[i], NULL) != 0) {
            fatal_system_error("Erro ao fazer join da thread");
        }

        
        global_sum += thread_data[i].sum;
        if (thread_data[i].bigger > global_bigger) global_bigger = thread_data[i].bigger;
        if (thread_data[i].smaller < global_smaller) global_smaller = thread_data[i].smaller;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end); 
    double elapsed_time = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) * 1e-9;

    printf("smaller is %d\n", global_smaller);
    printf("bigger  is %d\n", global_bigger);
    printf("The sum is %ld\n", global_sum);
    printf("Total elapsed time = %9.6fs\n", elapsed_time);

    free(values);
    return EXIT_SUCCESS;
}
