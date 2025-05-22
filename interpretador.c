#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// 

int main() {
    FILE *file = fopen("exec.txt", "r");
    if (!file) 
    {
    perror("Erro ao abrir exec.txt");
    exit(1);
    }
    char linha[256];

    while (fgets(linha, sizeof(linha), file)) {
        linha[strcspn(linha, "\n")] = 0;

        printf("[Interpretador] Enviando comando: %s\n", linha);

        pid_t pid = fork();
        if (pid == 0) {
            execl("./escalonador", "escalonador", linha, NULL);
            perror("Erro ao executar escalonador");
            exit(1);
        }

        sleep(1); 
    }

fclose(file);
return 0;
}