// main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("=== Iniciando o sistema ===\n");

    pid_t pid1 = fork();
    if (pid1 == 0) {
        execl("./interpretador", "interpretador", NULL);
        perror("Erro ao executar interpretador");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./escalonador", "escalonador", NULL);
        perror("Erro ao executar escalonador");
        exit(1);
    }

    printf("[Main] Interpretador e Escalonador iniciados.\n");
    printf("[Main] Finalizando processo principal.\n");
    return 0;
}