#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    FILE *file = fopen("exec.txt", "r");
    if (!file) 
    {
    perror("Erro ao abrir exec.txt");
    exit(1);
    }
    char linha[256];

    while (fgets(linha, sizeof(linha), file)) {
        // Remove quebra de linha
        linha[strcspn(linha, "\n")] = 0;

        printf("[Interpretador] Enviando comando: %s\n", linha);

        // Cria processo filho para enviar comando ao escalonador
        pid_t pid = fork();
        if (pid == 0) {
            execl("./escalonador", "escalonador", linha, NULL);
            perror("Erro ao executar escalonador");
            exit(1);
        }

        sleep(1); // Espera 1 segundo entre cada comando
    }

fclose(file);
return 0;
}