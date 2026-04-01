#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Função sugerida no enunciado para simplificar a verificação de erros
void fatal_system_error(const char *errorMsg) {
    perror(errorMsg);
    exit(EXIT_FAILURE);
}

int main() {
    pid_t pid_date, pid_ping;
    int status_date, status_ping;

    //primeiro processo filho
    pid_date = fork();
    if (pid_date < 0) {
        fatal_system_error("Erro ao fazer o fork para o date");
    } else if (pid_date == 0) {
        
        execl("/bin/date", "date", NULL);
        
        // Se chegar aqui, é porque falhou.
        fatal_system_error("Erro no exec do date"); 
    }

    // segundo processo filho
    pid_ping = fork();
    if (pid_ping < 0) {
        fatal_system_error("Erro ao fazer o fork para o ping");
    } else if (pid_ping == 0) {
        
        execl("/bin/ping", "ping", "-c", "4", "www.google.com", NULL);
        
        fatal_system_error("Erro no exec do ping");
    }

    // CÓDIGO DO PAI
    
    if (waitpid(pid_date, &status_date, 0) == -1) {
        fatal_system_error("Erro ao esperar pelo date");
    }
    
    if (waitpid(pid_ping, &status_ping, 0) == -1) {
        fatal_system_error("Erro ao esperar pelo ping");
    }

    
    printf("\n--- Resultados da Execução ---\n");
    
    // WIFEXITED verifica se o processo terminou normalmente.
    // WEXITSTATUS extrai o código de saída.
    if (WIFEXITED(status_date)) {
        printf("O comando 'date' terminou com estado: %d\n", WEXITSTATUS(status_date));
    }
    
    if (WIFEXITED(status_ping)) {
        printf("O comando 'ping' terminou com estado: %d\n", WEXITSTATUS(status_ping));
    }

    return EXIT_SUCCESS;
}