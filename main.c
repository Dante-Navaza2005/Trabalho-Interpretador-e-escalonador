// Marcela Issa 2310746 e Dante Navaza 2321406
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Cria um pipe e inicializa dois processos filhos: um para o interpretador e outro para o escalonador.
int main(void) {
    int pipefd[2]; // pipefd[0] = leitura, pipefd[1] = escrita

    // Cria o pipe para comunicação entre interpretador e escalonador
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid_int = fork();
    if (pid_int == 0) {
        close(pipefd[0]);                       // Fecha a extremidade de leitura
        dup2(pipefd[1], STDOUT_FILENO);         // Redireciona stdout para o pipe
        close(pipefd[1]);                       // Fecha a extremidade original após duplicar

        execl("./interpretador", "interpretador", NULL); // Executa o programa interpretador
        perror("execl interpretador");                   // Só chega aqui se execl falhar
        exit(EXIT_FAILURE);
    }

    pid_t pid_esc = fork();
    if (pid_esc == 0) {
        close(pipefd[1]);                      // Fecha a extremidade de escrita
        dup2(pipefd[0], STDIN_FILENO);         // Redireciona stdin para o pipe
        close(pipefd[0]);                      // Fecha a extremidade original após duplicar

        execl("./escalonador", "escalonador", NULL); // Executa o programa escalonador
        perror("execl escalonador");                 // Só chega aqui se execl falhar
        exit(EXIT_FAILURE);
    }

    // Fecha os lados do pipe
    close(pipefd[0]);
    close(pipefd[1]);

    printf("[Main] Interpretador (PID %d) e Escalonador (PID %d) iniciados.\n",
           pid_int, pid_esc);
    printf("[Main] Processo principal encerrado.\n");
    return 0;
}
