// main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* ---------------- Interpretador (escreve) ---------------- */
    pid_t pid_int = fork();
    if (pid_int == 0) {
        /* filho: fecha leitura, duplica escrita em stdout */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execl("./interpretador", "interpretador", NULL);
        perror("execl interpretador");
        exit(EXIT_FAILURE);
    }

    /* ---------------- Escalonador (lê) ---------------- */
    pid_t pid_esc = fork();
    if (pid_esc == 0) {
        /* filho: fecha escrita, duplica leitura em stdin */
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execl("./escalonador", "escalonador", NULL);
        perror("execl escalonador");
        exit(EXIT_FAILURE);
    }

    /* pai */
    close(pipefd[0]);
    close(pipefd[1]);

    printf("[Main] Interpretador (PID %d) e Escalonador (PID %d) iniciados.\n",
           pid_int, pid_esc);
    printf("[Main] Processo principal encerrado.\n");
    return 0;
}
