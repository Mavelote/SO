#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>



typedef struct {
    int smaller;
    int bigger;
    long sum;
} PartialResult;

void fatal_system_error(const char *errorMsg) {
    perror(errorMsg);
    exit(EXIT_FAILURE);
}

void random_init() {
    srandom(2026); 
}

long random_get_value(long min, long max) {
    return min + random() % (max - min + 1);
}

short *vector_create_short(unsigned long dim) {
    return malloc(dim * sizeof(short));
}

void vector_random_init_short(short values[], unsigned long dim) {
    for (unsigned long i = 0; i < dim; ++i) {
        values[i] = random_get_value(SHRT_MIN, SHRT_MAX);
    } 
}

int main(int argc, char *argv[]){
    random_init();

    unsigned long max = 6e8;
    int num_processes = 1;

    if (argc > 1)
    {
        max = atol(argv[1]);
    }
    if (argc > 2)
    {
        num_processes = atoi(argv[2]);
    }

    if (num_processes <= 0)
    {
        fprintf(stderr, "O número de processos deve ser maior que 0. \n");
        return EXIT_FAILURE;
    }

    short *values = vector_create_short(max);
    if (values == NULL) fatal_system_error("Falha a alocar memória");

    vector_random_init_short(values, max);

    int pipes[num_processes][2];

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    unsigned long chunk_size = max / num_processes;

    for (int i = 0; i < num_processes; i++)
    {
        if (pipe(pipes[i]) == -1) 
        {
            fatal_system_error("Erro no pipe");
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            fatal_system_error("Erro no fork");
        }

        if (pid == 0)
        {
            close(pipes[i][0]);

            unsigned long start = i * chunk_size;
            unsigned long end = (i == num_processes - 1) ? max : (i+1) * chunk_size;

            PartialResult res;
            res.smaller = values[start];
            res.bigger = values[start];
            res.sum = values[start];

            for (unsigned long j = start + 1; j < end; j++)
            {
                res.sum += values[j];
                if (values[j] > res.bigger) res.bigger = values[j];
                if (values[j] < res.smaller) res.smaller = values[j];
            }
            if (write(pipes[i][1], &res, sizeof(PartialResult)) == -1)
            {
                fatal_system_error("Erro na escrita do pipe");

            }

            close(pipes[i][1]);
            free(values);
            exit(EXIT_SUCCESS);
            
        }else
        {
            close(pipes[i][1]);
        }
        
        
        
    }
    
    PartialResult global_res;
    global_res.smaller = SHRT_MAX;
    global_res.bigger = SHRT_MIN;
    global_res.sum = 0;

    for (int i = 0; i < num_processes; i++)
    {
       PartialResult child_res;
       
       if (read(pipes[i][0], &child_res, sizeof(PartialResult)) == -1)
       {
        fatal_system_error("Erro na leitura do pipe");
       }
       close(pipes[i][0]);

       wait(NULL);

       global_res.sum += child_res.sum;
        if (child_res.bigger > global_res.bigger) global_res.bigger = child_res.bigger;
        if (child_res.smaller < global_res.smaller) global_res.smaller = child_res.smaller;
       
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed_time = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("smaller is %d\n",  global_res.smaller);
    printf("bigger  is %d\n",  global_res.bigger);
    printf("The sum is %ld\n", global_res.sum);
    printf("Elapsed time: %.2f seconds\n", elapsed_time);

    free(values);
    return EXIT_SUCCESS;
    
    
    
}