#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 256

int main() {
    FILE *file = fopen("exec.txt", "r");
    if (!file) {
        perror("Erro ao abrir exec.txt");
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Filho: escalonador
        close(pipefd[1]); // fecha escrita no filho
        dup2(pipefd[0], STDIN_FILENO); // redireciona pipe para stdin
        close(pipefd[0]);

        // Executa escalonador (supondo escalonador está compilado como ./escalonador)
        execl("./escalonador", "escalonador", NULL);
        perror("execl");
        exit(1);
    } else {
        // Pai: interpretador
        close(pipefd[0]); // fecha leitura no pai

        char line[BUF_SIZE];
        while (fgets(line, BUF_SIZE, file)) {
            // Envia linha para escalonador
            write(pipefd[1], line, strlen(line));
            // Envia newline para garantir leitura correta
            write(pipefd[1], "\n", 1);

            // Aguarda 1 segundo (simula 1 UT)
            sleep(1);
        }

        close(pipefd[1]);
        fclose(file);

        // Espera filho terminar
        wait(NULL);
    }

    return 0;
}
