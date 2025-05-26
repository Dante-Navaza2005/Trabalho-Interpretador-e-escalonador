// Marcela Issa 2310746 e Dante Navaza 2321406
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UT 1  

// Interpretador que lê comandos de exec.txt e os envia para o escalonador e 
//envia um por um, a cada 1 UT, para o escalonador via stdout (pipe)

int main(void) {
    fprintf(stderr, "[Interpretador] Iniciando leitura de comandos.\n");

    // Abre o arquivo exec.txt para leitura
    FILE *arquivo = fopen("exec.txt", "r");
    if (!arquivo) {
        perror("[Interpretador] Erro ao abrir exec.txt");
        exit(EXIT_FAILURE);
    }

    char linha[256];
    while (fgets(linha, sizeof(linha), arquivo)) {
        linha[strcspn(linha, "\n")] = '\0';  // Remove o \n no final da linha
        fprintf(stderr, "[Interpretador] Enviando: %s\n", linha);
        // Envia o comando para o escalonador via pipe (stdout)
        // Enviamos strlen(linha) + 1 para incluir o '\0' no final
        write(STDOUT_FILENO, linha, strlen(linha) + 1); 
        // Aguarda 1 unidade de tempo antes de enviar o próximo comando
        sleep(UT);
    }

    fclose(arquivo);
    fprintf(stderr, "[Interpretador] Fim da execução.\n");
    return 0;
}
