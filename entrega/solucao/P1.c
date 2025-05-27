// Marcela Issa 2310746 e Dante Navaza 2321406
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Executando programa P1 (pid=%d)…\n", getpid());
    while (1) {
        sleep(1); // Simula trabalho infinito
    }
    return 0;
}
