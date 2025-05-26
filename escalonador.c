// Marcela Issa 2310746 e Dante Navaza 2321406
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_PROCESSOS 50
#define UT 1

typedef enum { RR, PRIO, RT } Tipo;

typedef struct {
    pid_t pid;
    char  nome[20];
    Tipo  tipo;
    int   prioridade;
    int   inicio, duracao;
    int   tempo_executado;
    int   ativo;
} Processo;

Processo processos[MAX_PROCESSOS];
int num_processos = 0;
int tempo_global = 0;
int rr_ultimo = -1;

void exibir_filas(void) {
    printf("[Tempo %d] Filas:\n", tempo_global);
    printf("  RT:   ");
    for (int i = 0; i < num_processos; i++)
        if (processos[i].ativo && processos[i].tipo == RT)
            printf("%s ", processos[i].nome);

    printf("\n  PRIO: ");
    for (int i = 0; i < num_processos; i++)
        if (processos[i].ativo && processos[i].tipo == PRIO)
            printf("%s(P=%d) ", processos[i].nome, processos[i].prioridade);

    printf("\n  RR:   ");
    for (int i = 0; i < num_processos; i++)
        if (processos[i].ativo && processos[i].tipo == RR)
            printf("%s ", processos[i].nome);

    printf("\n");
}

int conflitoRT(int inicio, int duracao) {
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].tipo != RT) continue;
        int i1 = processos[i].inicio;
        int f1 = i1 + processos[i].duracao;
        int i2 = inicio;
        int f2 = i2 + duracao;
        if (i2 < f1 && i1 < f2) return 1;
    }
    return 0;
}

void encerra_tudo(int sig) {
    printf("\n[Escalonador] Encerrando por sinal SIGINT...\n");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].ativo) {
            printf("[Escalonador] Finalizando %s (pid %d)\n",
                   processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }
    exit(EXIT_SUCCESS);
}

int main(void) {
    signal(SIGINT, encerra_tudo);
    char linha[256];
    printf("[Escalonador] Iniciando...\n");

    while (tempo_global < 120) {
        ssize_t n = read(STDIN_FILENO, linha, sizeof(linha));
        if (n > 0) {
            linha[n] = '\0';

            char nome[20];
            int prioridade = -1, inicio = -1, duracao = -1;
            Tipo tipo = RR;
            sscanf(linha, "Run %19s", nome);

            // Verifica duplicidade
            int duplicado = 0;
            for (int i = 0; i < num_processos; i++) {
                if (strcmp(processos[i].nome, nome) == 0 && processos[i].ativo) {
                    printf("[Escalonador] Processo %s já existe. Ignorado.\n", nome);
                    duplicado = 1;
                    break;
                }
            }
            if (duplicado) continue;

            // Detecta tipo de processo
            if (strstr(linha, "P=")) {
                sscanf(linha, "Run %*s P=%d", &prioridade);
                tipo = PRIO;
            } else if (strstr(linha, "I=")) {
                sscanf(linha, "Run %*s I=%d D=%d", &inicio, &duracao);
                tipo = RT;
                if (inicio + duracao > 60 || conflitoRT(inicio, duracao)) {
                    printf("[Escalonador] Conflito no processo %s (RT). Ignorado.\n", nome);
                    continue;
                }
            }

            pid_t pid = fork();
            if (pid == 0) {
                execl(nome, nome, NULL);
                perror("[Escalonador] execl");
                exit(EXIT_FAILURE);
            }
            kill(pid, SIGSTOP);

            Processo p = { pid, "", tipo, prioridade, inicio, duracao, 0, 1 };
            strncpy(p.nome, nome, sizeof(p.nome) - 1);
            processos[num_processos++] = p;

            printf("[Escalonador] %s carregado (PID %d)\n", nome, pid);
        } else if (n == -1 && errno != EAGAIN) {
            perror("[Escalonador] Erro de leitura");
        }

        Processo *atual = NULL;
        int menor_prio = 100;
        int t_rel = tempo_global % 60;

        // 1) RT
        for (int i = 0; i < num_processos; i++) {
            Processo *p = &processos[i];
            if (!p->ativo || p->tipo != RT) continue;
            if (t_rel >= p->inicio && t_rel < p->inicio + p->duracao) {
                atual = p;
                break;
            }
        }

        // 2) PRIO
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

        // 3) RR
        if (!atual) {
            for (int off = 1; off <= num_processos; off++) {
                int idx = (rr_ultimo + off) % num_processos;
                Processo *p = &processos[idx];
                if (p->ativo && p->tipo == RR) {
                    atual = p;
                    rr_ultimo = idx;
                    break;
                }
            }
        }

        // Execução ou ociosidade
        if (atual) {
            int tempo_restante = -1;
            if (atual->tipo == PRIO)
                tempo_restante = 3 - atual->tempo_executado;
            else if (atual->tipo == RT)
                tempo_restante = (atual->inicio + atual->duracao) - t_rel;

            if (tempo_restante >= 0)
                printf("[Tempo %d] Executando %s (restam %d UTs)\n", tempo_global, atual->nome, tempo_restante);
            else
                printf("[Tempo %d] Executando %s\n", tempo_global, atual->nome);

            kill(atual->pid, SIGCONT);
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
        tempo_global++;  // Agora sempre avança, mesmo se ocioso
    }

    // Encerramento natural
    printf("[Escalonador] Tempo máximo atingido.\n");
    for (int i = 0; i < num_processos; i++) {
        if (processos[i].ativo) {
            printf("[Escalonador] Finalizando %s (pid %d)\n", processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }
    return 0;
}
