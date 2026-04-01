#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


void fatal_system_error(const char *errorMsg) {
    perror(errorMsg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <nome_do_ficheiro>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Criar o pipe. pipefd[0] é para LER, pipefd[1] é para ESCREVER
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fatal_system_error("Erro ao criar o pipe");
    }

    // Criar o processo filho
    pid_t pid = fork();
    if (pid == -1) {
        fatal_system_error("Erro no fork");
    } 
    else if (pid == 0) {
        // O filho só vai escrever no pipe, logo fechamos a ponta de leitura
        close(pipefd[0]);

        // Redirecionar o Standard Output para a ponta de escrita do pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            fatal_system_error("Erro no dup2");
        }
        // Como o dup2 já duplicou o canal, podemos fechar o original
        close(pipefd[1]);

        execlp("wc", "wc", "-w", argv[1], NULL);
        
        // Se o exec falhar
        fatal_system_error("Erro ao executar o wc");
    } 
    else {
        // O pai só vai ler do pipe, logo fechamos a ponta de escrita
        close(pipefd[1]);

        char buffer[128];
        // Ler os dados que o filho atirou para o pipe
        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read == -1) {
            fatal_system_error("Erro ao ler do pipe");
        }
        
        // Colocar o terminador de string no fim do que lemos
        buffer[bytes_read] = '\0';
        
        close(pipefd[0]);

        // Esperar que o processo filho termine 
        if (waitpid(pid, NULL, 0) == -1) {
            fatal_system_error("Erro no waitpid");
        }

        // O comando 'wc -w ficheiro' devolve algo como "   15 ficheiro"
        int word_count;
        if (sscanf(buffer, "%d", &word_count) == 1) {
            printf("O ficheiro '%s' tem %d palavras.\n", argv[1], word_count);
        } else {
            printf("Erro ao interpretar a resposta do wc: %s\n", buffer);
        }
    }

    return EXIT_SUCCESS;
}