#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "../Exercicio1/countdown.h"
#include "../Exercicio2/thread_pool.h"

typedef struct
{
    short *vector;       // Ponteiro para o vetor gigante
    unsigned long start; // Onde este trabalho começa
    unsigned long end;   // Onde este trabalho acaba

    int smaller; // Resultado: Menor valor
    int bigger;  // Resultado: Maior valor
    long sum;    // Resultado: Soma

    countdown_t *cd; // O sino para avisar o Chefe que acabámos!
} VectorTaskData;

void math_worker(void *arg)
{
    VectorTaskData *data = (VectorTaskData *)arg;

    int loc_smaller = data->vector[data->start];
    int loc_bigger = data->vector[data->start];
    long loc_sum = data->vector[data->start];

    for (unsigned long i = data->start + 1; i < data->end; i++)
    {
        loc_sum += data->vector[i];
        if (data->vector[i] > loc_bigger)
            loc_bigger = data->vector[i];
        if (data->vector[i] < loc_smaller)
            loc_smaller = data->vector[i];
    }

    data->smaller = loc_smaller;
    data->bigger = loc_bigger;
    data->sum = loc_sum;

    countdown_down(data->cd);
}

void random_init()
{
    srandom((unsigned int)time(NULL));
}

void vector_random_init_short(short *vector, unsigned long max_dim)
{
    if (vector == NULL)
        return;

    for (unsigned long i = 0; i < max_dim; i++)
    {
        vector[i] = (short)(random() % SHRT_MAX);
    }
}

int main(int argc, char *argv[])
{
    random_init();

    unsigned long max_dim = 6e8; // 600 milhões
    int num_tasks = 4;           // Vamos dividir em 4 fatias (trabalhos)

    // Alocar memória RAM para os 600 milhões de números
    short *values = malloc(max_dim * sizeof(short));
    if (values == NULL)
    {
        perror("Erro a alocar memória para o vetor");
        return EXIT_FAILURE;
    }

    // 0.1 Preencher o vetor com números a sério!
    vector_random_init_short(values, max_dim);
    countdown_t cd;
    countdown_init(&cd, num_tasks);

    // 2. Iniciar a Piscina de Trabalhadores (Exercício 2)
    // Vamos contratar 4 trabalhadores para o nosso Pool
    thread_pool_t *pool = thread_pool_create(4);

    // 3. Preparar as fatias de trabalho
    VectorTaskData tasks[num_tasks];
    unsigned long chunk_size = max_dim / num_tasks;

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    // 4. SUBMETER O TRABALHO AO POOL
    for (int i = 0; i < num_tasks; i++)
    {
        tasks[i].vector = values;
        tasks[i].start = i * chunk_size;
        tasks[i].end = (i == num_tasks - 1) ? max_dim : (i + 1) * chunk_size;
        tasks[i].cd = &cd; // Passamos o mecanismo para eles poderem avisar!

        // ATIRAR PARA A FILA! O Chefe não cria threads, apenas submete o pedido.
        thread_pool_submit(pool, math_worker, &tasks[i]);
    }

    // 5. O CHEFE FICA À ESPERA...
    // O Chefe cruza os braços e bloqueia aqui até que o contador chegue a zero
    // (ou seja, até as 4 threads fazerem countdown_down).
    countdown_wait(&cd);

    // 6. TRABALHO CONCLUÍDO! AGREGAR RESULTADOS
    int global_smaller = SHRT_MAX;
    int global_bigger = SHRT_MIN;
    long global_sum = 0;

    for (int i = 0; i < num_tasks; i++)
    {
        global_sum += tasks[i].sum;
        if (tasks[i].bigger > global_bigger)
            global_bigger = tasks[i].bigger;
        if (tasks[i].smaller < global_smaller)
            global_smaller = tasks[i].smaller;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    double elapsed_time = (t_end.tv_sec - t_start.tv_sec) +
                          (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("Menor valor encontrado: %d\n", global_smaller);
    printf("Maior valor encontrado: %d\n", global_bigger);
    printf("Soma total do vetor:    %ld\n", global_sum);
    printf("Tempo total de execucao: %.4f segundos\n", elapsed_time);

    // 7. LIMPEZAS FINAIS
    // Usamos a nossa terminação graciosa do Exercício 2!
    thread_pool_destroy(pool);
    countdown_destroy(&cd);
    free(values);

    return EXIT_SUCCESS;
}