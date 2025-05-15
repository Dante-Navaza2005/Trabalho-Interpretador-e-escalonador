#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Executando programa P2 (pid=%d)…\n", getpid());
    while (1) {
        sleep(1); // Simula trabalho infinito
    }
    return 0;
}