// interpretador.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UT 1                /* unidade de tempo entre comandos */

int main(void) {
    fprintf(stderr, "[Interpretador] Iniciando leitura de comandos.\n");

    FILE *arquivo = fopen("exec.txt", "r");
    if (!arquivo) {
        perror("[Interpretador] Erro ao abrir exec.txt");
        exit(EXIT_FAILURE);
    }

    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        linha[strcspn(linha, "\n")] = '\0';          /* remove '\n' */
        fprintf(stderr, "[Interpretador] Enviando: %s\n", linha);
        write(STDOUT_FILENO, linha, strlen(linha) + 1); /* +1 => inclui '\0' */
        sleep(UT);
    }

    fclose(arquivo);
    fprintf(stderr, "[Interpretador] Fim da execução.\n");
    return 0;
}
