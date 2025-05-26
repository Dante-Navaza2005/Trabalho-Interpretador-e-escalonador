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

typedef enum { ROUND_ROBIN, PRIORIDADE, REAL_TIME } Tipo;

typedef struct {
    pid_t pid;
    char  nome[20];
    Tipo  tipo;
    int   prioridade;
    int   inicio;
    int duracao;
    int   tempo_executado;
    int   ativo;
} Processo;

Processo processos[MAX_PROCESSOS];
int num_processos = 0;
int tempo_global = 0;
int round_robin_ultimo = -1;

void exibir_filas(void) {
    printf("[Tempo %d] Filas:\n", tempo_global);
    printf("  REAL_TIME:   ");
    for (int i = 0; i < num_processos; i++)
    {
        if (processos[i].ativo && processos[i].tipo == REAL_TIME)
        {
            printf("%s ", processos[i].nome);
        }
    }

    printf("\n  PRIORIDADE: ");
    for (int i = 0; i < num_processos; i++)
    {
        if (processos[i].ativo && processos[i].tipo == PRIORIDADE)
        {
            printf("%s(P=%d) ", processos[i].nome, processos[i].prioridade);
        }
    }

    printf("\n  ROUND_ROBIN:   ");
    for (int i = 0; i < num_processos; i++)
        if (processos[i].ativo && processos[i].tipo == ROUND_ROBIN)
            printf("%s ", processos[i].nome);

    printf("\n");
}

int conflitoRT(int inicio, int duracao) 
{
    for (int i = 0; i < num_processos; i++) 
    {
        if (processos[i].tipo != REAL_TIME) 
        {
            continue;
        }
        int inicio_processo_existente = processos[i].inicio;
        int fim_processo_existente = inicio_processo_existente + processos[i].duracao;
        int inicio_novo_processo = inicio;
        int fim_novo_processo = inicio_novo_processo + duracao;
        if (inicio_novo_processo < fim_processo_existente && inicio_processo_existente < fim_novo_processo) 
        {
            return 1;
        }
    }
    return 0;
}

void encerra_tudo(int sig) {
    printf("\n[Escalonador] Encerrando por sinal SIGINT...\n");
    for (int i = 0; i < num_processos; i++) 
    {
        if (processos[i].ativo) 
        {
            printf("[Escalonador] Finalizando %s (pid %d)\n", processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }
    exit(0);
}

int lerNumero(char **p) 
{
    int valor = 0;
    while (**p >= '0' && **p <= '9') 
    {
        valor = valor * 10 + (**p - '0');
        (*p)++;
    }
    return valor;
}


int main(void) 
{
    signal(SIGINT, encerra_tudo);
    char linha[256];
    printf("[Escalonador] Iniciando...\n");

    while (tempo_global < 120) {
        int n = read(STDIN_FILENO, linha, sizeof(linha) - 1);
        if (n > 0) 
        {
            linha[n] = '\0';

            char nome[20];
            int prioridade = -1; 
            int inicio = -1;
            int duracao = -1;
            Tipo tipo = ROUND_ROBIN;

            char *p = linha;
            while (*p == ' ') p++;
            if (strncmp(p, "Run", 3) == 0) {
                p += 3;
                while (*p == ' ') p++;

                int i = 0;
                while (*p && *p != ' ') 
                {
                    nome[i++] = *p++;
                }
                nome[i] = '\0';

                while (*p == ' ') 
                {
                    p++;
                }

                if (*p == 'P' && *(p+1) == '=') 
                {
                    p += 2;
                    prioridade = lerNumero(&p);
                    tipo = PRIORIDADE;
                } 
                else if (*p == 'I' && *(p+1) == '=') 
                {
                    p += 2;
                    inicio = lerNumero(&p);
                    while (*p == ' ') p++;
                    if (*p == 'D' && *(p+1) == '=') 
                    {
                        p += 2;
                        duracao = lerNumero(&p);
                        tipo = REAL_TIME;
                        if (inicio + duracao > 60 || conflitoRT(inicio, duracao)) 
                        {
                            printf("[Escalonador] Conflito no processo %s (REAL_TIME). Ignorado.\n", nome);
                            continue;
                        }
                    }
                }
            }

            int duplicado = 0;
            for (int i = 0; i < num_processos; i++) 
            {
                if (strcmp(processos[i].nome, nome) == 0 && processos[i].ativo) 
                {
                    printf("[Escalonador] Processo %s ja existe. Ignorado.\n", nome);
                    duplicado = 1;
                    break;
                }
            }
            if (duplicado) continue;

            pid_t pid = fork();
            if (pid == 0) {
                execl(nome, nome, NULL);
                perror("[Escalonador] execl");
                exit(1);
            }
            kill(pid, SIGSTOP);

            Processo pnovo = { pid, "", tipo, prioridade, inicio, duracao, 0, 1 };
            strncpy(pnovo.nome, nome, sizeof(pnovo.nome) - 1);
            processos[num_processos++] = pnovo;

            printf("[Escalonador] %s carregado (PID %d)\n", nome, pid);
        } 
        else if (n == -1) 
        {
            perror("[Escalonador] Erro de leitura");
        }

        Processo *atual = NULL;
        int menor_prio = 100;
        int segundos = tempo_global % 60;

        for (int i = 0; i < num_processos; i++) {
            Processo *p = &processos[i];
            if (!p->ativo || p->tipo != REAL_TIME) continue;
            if (segundos >= p->inicio && segundos < p->inicio + p->duracao) {
                atual = p;
                break;
            }
        }

        if (!atual) {
            for (int i = 0; i < num_processos; i++) {
                Processo *p = &processos[i];
                if (!p->ativo || p->tipo != PRIORIDADE) continue;
                if (p->tempo_executado < 3 && p->prioridade < menor_prio) {
                    menor_prio = p->prioridade;
                    atual = p;
                }
            }
        }

        if (!atual) {
            for (int off = 1; off <= num_processos; off++) {
                int idx = (round_robin_ultimo + off) % num_processos;
                Processo *p = &processos[idx];
                if (p->ativo && p->tipo == ROUND_ROBIN) {
                    atual = p;
                    round_robin_ultimo = idx;
                    break;
                }
            }
        }

        if (atual) {
            int tempo_restante = -1;
            if (atual->tipo == PRIORIDADE)
                tempo_restante = 3 - atual->tempo_executado;
            else if (atual->tipo == REAL_TIME)
                tempo_restante = (atual->inicio + atual->duracao) - segundos;

            if (tempo_restante >= 0)
                printf("[Tempo %d] Executando %s (restam %d UTs)\n", tempo_global, atual->nome, tempo_restante);
            else
                printf("[Tempo %d] Executando %s\n", tempo_global, atual->nome);

            kill(atual->pid, SIGCONT);
            sleep(UT);
            kill(atual->pid, SIGSTOP);
            printf("[Tempo %d] %s preemptado\n", tempo_global, atual->nome);
            atual->tempo_executado++;

            if (atual->tipo == PRIORIDADE && atual->tempo_executado == 3) {
                atual->ativo = 0;
                kill(atual->pid, SIGKILL);
                printf("[Tempo %d] %s finalizado\n", tempo_global, atual->nome);
            }
        } 
        else 
        {
            printf("[Tempo %d] Nenhum processo para executar\n", tempo_global);
            sleep(UT);
        }

        exibir_filas();
        tempo_global++;
    }

    printf("[Escalonador] Tempo maximo atingido.\n");
    for (int i = 0; i < num_processos; i++) 
    {
        if (processos[i].ativo) 
        {
            printf("[Escalonador] Finalizando %s (pid %d)\n", processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }
    return 0;
}
