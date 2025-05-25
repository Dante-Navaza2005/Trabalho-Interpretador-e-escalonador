// escalonador.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <termios.h>

#define FIFO_PATH "/tmp/so_fifo"
#define MAX_PROCESSOS 50
#define UT 1

typedef enum { RR, PRIO, RT } Tipo;

typedef struct {
    pid_t pid;
    char nome[20];
    Tipo tipo;
    int prioridade;
    int inicio, duracao;
    int tempo_executado;
    int ativo;
} Processo;

Processo processos[MAX_PROCESSOS];
int num_processos = 0;
int tempo_global = 0;
int rr_ultimo = -1;  // novo: índice do último RR executado
int fd;

void exibir_filas() {
    printf("[Tempo %d] Filas:\n", tempo_global);
    printf("  RT:   ");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].tipo == RT && processos[i].ativo)
            printf("%s ", processos[i].nome);
    }
    printf("\n  PRIO: ");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].tipo == PRIO && processos[i].ativo)
            printf("%s(P=%d) ", processos[i].nome, processos[i].prioridade);
    }
    printf("\n  RR:   ");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].tipo == RR && processos[i].ativo)
            printf("%s ", processos[i].nome);
    }
    printf("\n");
}

int conflitoRT(int inicio, int duracao) {
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].tipo == RT) {
            int i1 = processos[i].inicio;
            int f1 = i1 + processos[i].duracao;
            int i2 = inicio;
            int f2 = i2 + duracao;
            if ((i2 < f1) && (i1 < f2)) return 1;
        }
    }
    return 0;
}

void encerrar(int sig) {
    printf("\n[Escalonador] Encerrando por sinal SIGINT...\n");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].ativo) {
            printf("[Escalonador] Finalizando processo %s (pid %d)\n", processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }
    close(fd);
    unlink(FIFO_PATH);
    exit(0);
}

int main() {
    signal(SIGINT, encerrar);
    setpgid(0, 0);
    tcsetpgrp(STDIN_FILENO, getpgrp());

    mkfifo(FIFO_PATH, 0666);
    fd = open(FIFO_PATH, O_RDONLY);
    char linha[256];

    printf("[Escalonador] Iniciando...\n");

    while (tempo_global < 120) {
        int n = read(fd, linha, sizeof(linha));
        if (n > 0) {
            linha[n] = '\0';
            char nome[20];
            int prioridade = -1, inicio = -1, duracao = -1;
            Tipo tipo = RR;
            sscanf(linha, "Run %s", nome);

            if (strstr(linha, "P=")) {
                sscanf(linha, "Run %*s P=%d", &prioridade);
                tipo = PRIO;
            } else if (strstr(linha, "I=")) {
                sscanf(linha, "Run %*s I=%d D=%d", &inicio, &duracao);
                tipo = RT;
                if (inicio + duracao > 60 || conflitoRT(inicio, duracao)) {
                    printf("[Escalonador] Conflito em processo %s (RT). Ignorado.\n", nome);
                    continue;
                }
            }

            pid_t pid = fork();
            if (pid == 0) {
                execl(nome, nome, NULL);
                perror("[Erro] execl");
                exit(1);
            }

            kill(pid, SIGSTOP);
            Processo p = { pid, "", tipo, prioridade, inicio, duracao, 0, 1 };
            strcpy(p.nome, nome);
            processos[num_processos++] = p;
            printf("[Escalonador] %s carregado com PID %d\n", nome, pid);
        }

        Processo *atual = NULL;
        int menor_prio = 100;
        int tempo_relativo = tempo_global % 60;

        // REAL-TIME
        for (int i = 0; i < num_processos; i++) {
            Processo *p = &processos[i];
            if (!p->ativo) continue;
            if (p->tipo == RT && tempo_relativo >= p->inicio && tempo_relativo < p->inicio + p->duracao) {
                atual = p;
                break;
            }
        }

        // PRIORIDADE
        if (!atual) {
            for (int i = 0; i < num_processos; i++) {
                Processo *p = &processos[i];
                if (!p->ativo || p->tipo != PRIO) continue;
                if (p->tempo_executado < 3 && p->prioridade < menor_prio) {
                    menor_prio = p->prioridade;
                    atual = p;
                }
            }
        }

        // ROUND-ROBIN com alternância
        if (!atual) {
            for (int offset = 1; offset <= num_processos; offset++) {
                int idx = (rr_ultimo + offset) % num_processos;
                Processo *p = &processos[idx];
                if (p->ativo && p->tipo == RR) {
                    atual = p;
                    rr_ultimo = idx;
                    break;
                }
            }
        }

        // Execução
        if (atual) {
            kill(atual->pid, SIGCONT);
            printf("[Tempo %d] Executando %s\n", tempo_global, atual->nome);
            sleep(UT);
            kill(atual->pid, SIGSTOP);
            printf("[Tempo %d] %s preemptado\n", tempo_global, atual->nome);
            atual->tempo_executado++;

            if (atual->tipo == PRIO && atual->tempo_executado == 3) {
                atual->ativo = 0;
                kill(atual->pid, SIGKILL);
                printf("[Tempo %d] %s finalizado\n", tempo_global, atual->nome);
            }
        } else {
            printf("[Tempo %d] Nenhum processo para executar\n", tempo_global);
            sleep(UT);
        }

        exibir_filas();
        tempo_global++;
    }

    printf("[Escalonador] Tempo máximo atingido. Encerrando processos restantes...\n");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].ativo) {
            printf("[Escalonador] Finalizando processo %s (pid %d)\n", processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }

    close(fd);
    unlink(FIFO_PATH);
    return 0;
}
