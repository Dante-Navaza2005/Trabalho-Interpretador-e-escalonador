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
        if (processos[i].ativo && processos[i].tipo == REAL_TIME) // pega os processos real time ativos
        {
            printf("%s ", processos[i].nome);
        }
    }

    printf("\n  PRIORIDADE: ");
    for (int i = 0; i < num_processos; i++)
    {
        if (processos[i].ativo && processos[i].tipo == PRIORIDADE)
        {
            printf("%s(P=%d) ", processos[i].nome, processos[i].prioridade); // pega os processos de prioridade
        }
    }

    printf("\n  ROUND_ROBIN:   ");
    for (int i = 0; i < num_processos; i++)
        if (processos[i].ativo && processos[i].tipo == ROUND_ROBIN) // pega os processos round robin
            printf("%s ", processos[i].nome);

    printf("\n");
}

int conflitoRT(int inicio, int duracao) 
{
    for (int i = 0; i < num_processos; i++) 
    {
        if (processos[i].tipo != REAL_TIME)  // ignora os que nao sao real time
        {
            continue;
        }

        // calcula o intervalo de tempo deles
        int inicio_processo_existente = processos[i].inicio;
        int fim_processo_existente = inicio_processo_existente + processos[i].duracao;

        // Calcula o intervalo de tempo do novo processo
        int inicio_novo_processo = inicio;
        int fim_novo_processo = inicio_novo_processo + duracao;

        // tem conflito se o início do novo for antes do fim do existente e o início do existente for antes do fim do novo
        if (inicio_novo_processo < fim_processo_existente && inicio_processo_existente < fim_novo_processo) 
        {
            return 1; 
        }
    }

    return 0;
}

void encerra_tudo(int sig) 
{
    printf("\n[Escalonador] Encerrando por sinal SIGINT...\n");
    for (int i = 0; i < num_processos; i++) 
    {
        if (processos[i].ativo) // finalizando os processos ativos
        {
            printf("[Escalonador] Finalizando %s (pid %d)\n", processos[i].nome, processos[i].pid);
            kill(processos[i].pid, SIGKILL);
            waitpid(processos[i].pid, NULL, 0);
        }
    }
    exit(0);
}

int lerNumero(char **p) // transforma inteiro positivo de string para numero
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
    // Define  (Ctrl+C)
    signal(SIGINT, encerra_tudo);

    char linha[256];
    printf("[Escalonador] Iniciando...\n");

    // simula 120 UT
    while (tempo_global < 120) {
        // Lê comandos enviados pelo STDIN (pipe do interpretador)
        int n = read(STDIN_FILENO, linha, sizeof(linha) - 1);
        if (n > 0) 
        {
            linha[n] = '\0';  // adicionando final a uma string

            // variaveis para inicializar os processos
            char nome[20];
            int prioridade = -1; 
            int inicio = -1;
            int duracao = -1;
            Tipo tipo = ROUND_ROBIN;  

            // Parsing do comando
            char *p = linha;
            while (*p == ' ')
            {
                p++;  // pula espaços
            } 

            if (strncmp(p, "Run", 3) == 0) {
                p += 3;
                while (*p == ' ') p++;

                // Lê o nome do processo
                int i = 0;
                while (*p && *p != ' ') 
                    nome[i++] = *p++;
                nome[i] = '\0';

                while (*p == ' ') 
                {
                    p++;
                }

                // Verifica se é PRIORIDADE ou REAL_TIME
                if (*p == 'P' && *(p+1) == '=') {
                    p += 2;
                    prioridade = lerNumero(&p);
                    tipo = PRIORIDADE; // porque tem P =
                } 
                else if (*p == 'I' && *(p+1) == '=') {
                    p += 2;
                    inicio = lerNumero(&p);
                    while (*p == ' ') 
                    {
                        p++;
                    }
                    if (*p == 'D' && *(p+1) == '=') {
                        p += 2;
                        duracao = lerNumero(&p);
                        tipo = REAL_TIME; // pq tem I = 

                        // Verifica se há conflito de tempo
                        if (inicio + duracao > 60 || conflitoRT(inicio, duracao)) {
                            printf("[Escalonador] Conflito no processo %s (REAL_TIME). Ignorado.\n", nome);
                            continue;
                        }
                    }
                }
            }

            // Verifica se o processo já foi criado
            int duplicado = 0;
            for (int i = 0; i < num_processos; i++) {
                if (strcmp(processos[i].nome, nome) == 0 && processos[i].ativo) {
                    printf("[Escalonador] Processo %s ja existe. Ignorado.\n", nome);
                    duplicado = 1;
                    break;
                }
            }
            if (duplicado)
            {
                continue;
            } 

            // Cria processo filho e interrompe (aguarda escalonamento)
            pid_t pid = fork();
            if (pid == 0) {
                execl(nome, nome, NULL);
                perror("[Escalonador] execl");
                exit(1);
            }
            kill(pid, SIGSTOP);  // pausa imediatamente após criação

            // Registra o processo na lista
            Processo pnovo = { pid, "", tipo, prioridade, inicio, duracao, 0, 1 };
            strncpy(pnovo.nome, nome, sizeof(pnovo.nome) - 1);
            processos[num_processos++] = pnovo;

            printf("[Escalonador] %s carregado (PID %d)\n", nome, pid);
        } 
        else if (n == -1) 
        {
            perror("[Escalonador] Erro de leitura");
        }

        // escolhendo o processo atual
        Processo *atual = NULL;
        int menor_prio = 100; // menor prioridade 
        int segundos = tempo_global % 60;

        // Prioridade para processos REAL_TIME dentro da janela de execução
        for (int i = 0; i < num_processos; i++) {
            Processo *p = &processos[i];
            if (!p->ativo || p->tipo != REAL_TIME) 
            {
                continue;
            }
            if (segundos >= p->inicio && segundos < p->inicio + p->duracao) {
                atual = p;
                break;
            }
        }

        // Depois, PRIORIDADE com menos prioridade (menor valor) e menos de 3 UTs
        if (!atual) {
            for (int i = 0; i < num_processos; i++) {
                Processo *p = &processos[i];
                if (!p->ativo || p->tipo != PRIORIDADE) 
                {
                    continue;
                }
                if (p->tempo_executado < 3 && p->prioridade < menor_prio) {
                    menor_prio = p->prioridade;
                    atual = p;
                }
            }
        }

        // ultimo, ROUND_ROBIN em ordem circular
        if (!atual) {
            for (int off = 1; off <= num_processos; off++) 
            {
                int index = (round_robin_ultimo + off) % num_processos;
                Processo *p = &processos[index];
                if (p->ativo && p->tipo == ROUND_ROBIN) {
                    atual = p;
                    round_robin_ultimo = index;
                    break;
                }
            }
        }

        // executando os processos
        if (atual) {
            int tempo_restante = -1;

            // Calcula tempo restante com base no tipo
            if (atual->tipo == PRIORIDADE)
            {
                tempo_restante = 3 - atual->tempo_executado;
            }
            else if (atual->tipo == REAL_TIME)
            {
                tempo_restante = (atual->inicio + atual->duracao) - segundos;
            }

            // Mostra mensagem com tempo restante
            if (tempo_restante >= 0)
                printf("\n\n[Tempo %d] Executando %s (restam %d UTs)\n", tempo_global, atual->nome, tempo_restante);
            else
                printf("\n\n[Tempo %d] Executando %s\n", tempo_global, atual->nome);

            // Executa 1 UT
            kill(atual->pid, SIGCONT);
            sleep(UT);
            kill(atual->pid, SIGSTOP);
            atual->tempo_executado++;

            // Finaliza PRIORIDADE após 3 UTs
            if (atual->tipo == PRIORIDADE && atual->tempo_executado == 3) 
            {
                atual->ativo = 0;
                kill(atual->pid, SIGKILL);
                printf("[Tempo %d] %s finalizado\n", tempo_global, atual->nome);
            }
        } 
        else 
        {
            // Nenhum processo pronto para execução
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
