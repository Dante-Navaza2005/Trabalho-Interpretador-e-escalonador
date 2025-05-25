#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void parse_exec(char *linha, char *programa, int *prioridade, int *inicio, int *duracao) {
    sscanf(linha, "Run %s", programa);

    char *ptr = strchr(linha, ' ');
    while (ptr != NULL) {
        if (strstr(ptr, "P=")) {
            sscanf(ptr, " P=%d", prioridade);
        } else if (strstr(ptr, "I=")) {
            sscanf(ptr, " I=%d", inicio);
        } else if (strstr(ptr, "D=")) {
            sscanf(ptr, " D=%d", duracao);
        }
        ptr = strchr(ptr + 1, ' ');
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s \"Run <programa> [P=<prioridade>] [I=<inicio>] [D=<duracao>]\"\n", argv[0]);
        return 1;
    }

    char programa[16] = {0};
    int prioridade = -1;
    int inicio = -1;
    int duracao = -1;

    parse_exec(argv[1], programa, &prioridade, &inicio, &duracao);

    printf("[Escalonador] Programa: %s, P=%d, I=%d, D=%d\n",
           programa, prioridade, inicio, duracao);

    if (inicio >= 0) {
        printf("[Escalonador] Esperando até o tempo %d...\n", inicio);
        sleep(inicio);
    }

    pid_t pid = fork();
    if (pid == 0) {
        execl(programa, programa, NULL);
        perror("Erro ao iniciar programa");
        exit(1);
    } else if (pid > 0) {
        if (duracao > 0) {
            sleep(duracao);
            kill(pid, SIGSTOP);
            printf("[Escalonador] Parando %s após %d segundos\n", programa, duracao);
        } else {
            sleep(2); 
            kill(pid, SIGSTOP);
            printf("[Escalonador] Parando %s após 2 segundos\n", programa);
        }

        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
        return 1;
    }

    return 0;
}
