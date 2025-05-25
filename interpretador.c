// interpretador.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/so_fifo"
#define UT 1

int main() {
    printf("[Interpretador] Iniciando leitura de comandos.\n");

    mkfifo(FIFO_PATH, 0666);
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("[Interpretador] Erro ao abrir FIFO");
        exit(1);
    }

    FILE *arquivo = fopen("exec.txt", "r");
    if (!arquivo) {
        perror("[Interpretador] Erro ao abrir exec.txt");
        exit(1);
    }

    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        linha[strcspn(linha, "\n")] = '\0';
        printf("[Interpretador] Enviando: %s\n", linha);
        write(fd, linha, strlen(linha) + 1);
        sleep(UT);
    }

    fclose(arquivo);
    close(fd);

    printf("[Interpretador] Fim da execução.\n");
    return 0;
}
