#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 200
#define TOTAL_ARQUIVOS 42
#define MAX_COURSES 100000

int cursos_ads[MAX_COURSES];
int num_cursos_ads = 0;

// Extrai cursos ADS (grupo 72) do primeiro arquivo
void extrair_cursos_ads(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        fprintf(stderr, "Rank 0: Erro ao abrir %s para extrair cursos ADS.\n", filename);
        return;
    }

    char linha[1024];
    fgets(linha, sizeof(linha), fp); // lê cabeçalho

    int linhas_ignoradas_ads = 0; // Renomeado para evitar confusão com outras ignoradas

    while (fgets(linha, sizeof(linha), fp))
    {
        char copia[1024];
        strcpy(copia, linha);

        int col = 0;
        int co_grupo = -1, co_curso = -1;

        // --- Primeira passagem para contar colunas e verificar integridade ---
        char *token_tmp = strtok(copia, ";");
        int num_colunas = 0;
        while (token_tmp != NULL)
        {
            num_colunas++;
            token_tmp = strtok(NULL, ";");
        }

        // Precisamos da coluna 2 (CO_CURSO) e 6 (CO_GRUPO)
        if (num_colunas < 6 || num_colunas < 2)
        {
            linhas_ignoradas_ads++;
            continue; // ignora linha incompleta
        }

        // --- Segunda passagem para extrair valores ---
        strcpy(copia, linha);           // Restaura a linha para strtok novamente
        token_tmp = strtok(copia, ";"); // Re-tokeniza
        col = 0;                        // Reinicia o contador de colunas

        while (token_tmp != NULL)
        {
            col++;
            if (col == 2)
                co_curso = atoi(token_tmp);
            if (col == 6)
                co_grupo = atoi(token_tmp);
            token_tmp = strtok(NULL, ";");
        }

        if (co_grupo == 72 && num_cursos_ads < MAX_COURSES)
        {
            cursos_ads[num_cursos_ads++] = co_curso;
        }
    }

    if (linhas_ignoradas_ads > 0)
        printf("extrair_cursos_ads: Ignoradas %d linhas incompletas no arquivo de cursos ADS.\n", linhas_ignoradas_ads);

    fclose(fp);
}

// Função genérica para contar respostas para uma questão
// Agora inclui um contador para respostas vazias (ultimo_contador_vazio)
void contar_respostas(const char *filename, const char *questao, int tamanho_contadores, int *contadores, int *linhas_ignoradas, int *num_alunos_ads_total_local, int *ultimo_contador_vazio)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        fprintf(stderr, "Erro ao abrir %s\n", filename);
        return;
    }

    char linha[2048];
    int idx_questao = -1;

    // Lê o cabeçalho para encontrar o índice da coluna da questão
    if (fgets(linha, sizeof(linha), fp))
    {
        char *copia = strdup(linha);
        char *token = strtok(copia, ";");
        int col = 0;
        while (token != NULL)
        {
            col++;
            token[strcspn(token, "\r\n")] = 0;
            if (token[0] == '"')
                memmove(token, token + 1, strlen(token));
            if (token[strlen(token) - 1] == '"')
                token[strlen(token) - 1] = '\0';

            if (strcmp(token, questao) == 0)
            {
                idx_questao = col;
                break;
            }
            token = strtok(NULL, ";");
        }
        free(copia);
    }

    // Se a questão não foi encontrada no cabeçalho, não podemos processar
    if (idx_questao == -1)
    {
        fprintf(stderr, "Aviso: Questão '%s' não encontrada no cabeçalho de %s. Arquivo ignorado para esta questão.\n", questao, filename);
        fclose(fp);
        return;
    }

    *linhas_ignoradas = 0; // Reinicia o contador de linhas ignoradas para este arquivo/questão

    // Lê linhas dos dados
    while (fgets(linha, sizeof(linha), fp))
    {
        char copia_linha[2048];
        strcpy(copia_linha, linha); // Trabalha em uma cópia da linha

        // --- Primeira passagem para contar colunas e validar integridade ---
        int num_colunas = 0;
        char *token_tmp = strtok(copia_linha, ";");
        while (token_tmp != NULL)
        {
            num_colunas++;
            token_tmp = strtok(NULL, ";");
        }

        // A linha precisa ter pelo menos 2 colunas (para CO_CURSO) e a coluna da questão
        if (num_colunas < idx_questao || num_colunas < 2)
        {
            (*linhas_ignoradas)++;
            continue; // ignora linha incompleta
        }

        // --- Segunda passagem para extrair valores relevantes ---
        strcpy(copia_linha, linha);             // Restaura a linha original para strtok novamente
        char *token = strtok(copia_linha, ";"); // Re-tokeniza
        int col = 0;                            // Reinicia o contador de colunas
        int co_curso = -1;
        char resposta[20] = {0}; // Buffer para a resposta da questão atual

        while (token != NULL)
        {
            col++;
            if (col == 2)
                co_curso = atoi(token); // CO_CURSO é a 2ª coluna
            if (col == idx_questao)
            {
                strncpy(resposta, token, sizeof(resposta) - 1);
                resposta[sizeof(resposta) - 1] = '\0'; // Garante null-termination
                // Remove quebras de linha e aspas extras
                resposta[strcspn(resposta, "\r\n")] = 0;
                if (resposta[0] == '"')
                    memmove(resposta, resposta + 1, strlen(resposta));
                if (strlen(resposta) > 0 && resposta[strlen(resposta) - 1] == '"')
                    resposta[strlen(resposta) - 1] = '\0';
            }
            token = strtok(NULL, ";");
        }

        // Verifica se o curso é válido (está entre os cursos ADS extraídos)
        int curso_valido = 0;
        for (int i = 0; i < num_cursos_ads; i++)
        {
            if (co_curso == cursos_ads[i])
            {
                curso_valido = 1;
                break;
            }
        }

        // Se for um aluno de ADS, processa a resposta
        if (curso_valido)
        {
            // Incrementa o contador total de alunos ADS uma única vez por linha
            // (Para evitar contagem múltipla se uma linha for processada para várias questões,
            // esta variável deve ser passada com cuidado ou totalizada em outro lugar.)
            // Aqui, estamos assumindo que é somado uma vez por aluno por arquivo
            // e que cada chamada a contar_respostas é para uma questão diferente.
            // Para um total exato de alunos ADS em todos os arquivos, seria melhor
            // ter um contador separado que é incrementado apenas uma vez por aluno.
            // Por simplicidade, vou usar um contador separado para o total de alunos ADS.
            if (strcmp(questao, "QE_I15") == 0)
            {                                    // Incrementa o total de alunos ADS uma vez por linha se for a primeira questão a ser processada.
                                                 // Idealmente, total_alunos_ads seria em uma struct ou contado em process_file().
                (*num_alunos_ads_total_local)++; // Incrementa o total de alunos ADS que passaram pelo filtro
            }

            if (strlen(resposta) == 0)
            {                               // Se a resposta estiver vazia
                (*ultimo_contador_vazio)++; // Incrementa o contador de respostas vazias
            }
            else
            { // Se houver uma resposta, categorize-a
                switch (resposta[0])
                {
                case 'A':
                    if (tamanho_contadores > 0)
                        contadores[0]++;
                    break;
                case 'B':
                    if (tamanho_contadores > 1)
                        contadores[1]++;
                    break;
                case 'C':
                    if (tamanho_contadores > 2)
                        contadores[2]++;
                    break;
                case 'D':
                    if (tamanho_contadores > 3)
                        contadores[3]++;
                    break;
                case 'E':
                    if (tamanho_contadores > 4)
                        contadores[4]++;
                    break;
                case 'F':
                    if (tamanho_contadores > 5)
                        contadores[5]++;
                    break; // Apenas para QE_I15
                default:
                    // Respostas que não se encaixam nas categorias A-F/E, mas não são vazias.
                    // Para este código, elas não são contadas especificamente, mas você pode adicionar
                    // um contador 'outros' ao final do array de contadores para isso.
                    // Exemplo: if (tamanho_contadores > X) contadores[X]++;
                    break;
                }
            }
        }
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    int rank, size;
    char arquivos[TOTAL_ARQUIVOS][MAX_FILENAME];

    // Contadores locais para as questões e vazios
    int contadores_QE_I15_local[6] = {0};  // A-F
    int contadores_QE_I15_vazio_local = 0; // Novo contador para vazios QE_I15
    int contadores_QE_I22_local[5] = {0};  // A-E
    int contadores_QE_I22_vazio_local = 0; // Novo contador para vazios QE_I22
    int contadores_QE_I23_local[5] = {0};  // A-E
    int contadores_QE_I23_vazio_local = 0; // Novo contador para vazios QE_I23

    // Contadores globais
    int contadores_QE_I15_global[6] = {0};
    int contadores_QE_I15_vazio_global = 0;
    int contadores_QE_I22_global[5] = {0};
    int contadores_QE_I22_vazio_global = 0;
    int contadores_QE_I23_global[5] = {0};
    int contadores_QE_I23_vazio_global = 0;

    // Contadores de linhas ignoradas por questão (incompletas)
    int ignoradas_I15_local = 0, ignoradas_I22_local = 0, ignoradas_I23_local = 0;
    int ignoradas_I15_global = 0, ignoradas_I22_global = 0, ignoradas_I23_global = 0;

    // Contador total de alunos ADS analisados (global)
    int total_alunos_ads_local = 0;
    int total_alunos_ads_global = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Monta os nomes dos arquivos a serem lidos
    for (int i = 0; i < TOTAL_ARQUIVOS; i++)
    {
        snprintf(arquivos[i], MAX_FILENAME,
                 "2.DADOS/microdados2014_arq%d.txt", i + 1);
    }

    // Processo 0 extrai cursos ADS para todos os processos usarem
    if (rank == 0)
    {
        extrair_cursos_ads(arquivos[0]);
        printf("Rank 0: Encontrados %d cursos de ADS (CO_GRUPO=72).\n", num_cursos_ads);
    }

    // Distribui dados de cursos para todos processos
    MPI_Bcast(&num_cursos_ads, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(cursos_ads, num_cursos_ads, MPI_INT, 0, MPI_COMM_WORLD);

    // Divide arquivos entre os processos (exceto o arquivo 0, que foi usado para extrair cursos)
    int arquivos_por_proc = (TOTAL_ARQUIVOS - 1) / size;
    int resto = (TOTAL_ARQUIVOS - 1) % size;
    int inicio = rank * arquivos_por_proc + (rank < resto ? rank : resto) + 1; // Começa do arquivo 1 (índice 0)
    int fim = inicio + arquivos_por_proc + (rank < resto ? 1 : 0);

    // Cada processo conta as respostas dos arquivos que lhe foram designados
    for (int i = inicio; i < fim && i < TOTAL_ARQUIVOS; i++)
    {
        contar_respostas(arquivos[i], "QE_I15", 6, contadores_QE_I15_local, &ignoradas_I15_local, &total_alunos_ads_local, &contadores_QE_I15_vazio_local);
        contar_respostas(arquivos[i], "QE_I22", 5, contadores_QE_I22_local, &ignoradas_I22_local, &total_alunos_ads_local, &contadores_QE_I22_vazio_local);
        contar_respostas(arquivos[i], "QE_I23", 5, contadores_QE_I23_local, &ignoradas_I23_local, &total_alunos_ads_local, &contadores_QE_I23_vazio_local);
    }

    // --- Redução MPI: Soma os resultados locais para globais no processo 0 ---

    // Reduz contadores de cada questão
    MPI_Reduce(contadores_QE_I15_local, contadores_QE_I15_global, 6, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I22_local, contadores_QE_I22_global, 5, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I23_local, contadores_QE_I23_global, 5, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Reduz contadores de respostas vazias
    MPI_Reduce(&contadores_QE_I15_vazio_local, &contadores_QE_I15_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I22_vazio_local, &contadores_QE_I22_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I23_vazio_local, &contadores_QE_I23_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Reduz contadores de linhas ignoradas por incompletude
    MPI_Reduce(&ignoradas_I15_local, &ignoradas_I15_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I22_local, &ignoradas_I22_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I23_local, &ignoradas_I23_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Reduz o total de alunos ADS analisados (para garantir que seja somado uma vez por aluno)
    MPI_Reduce(&total_alunos_ads_local, &total_alunos_ads_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Processo 0 imprime os resultados agregados
    if (rank == 0)
    {
        printf("\n");
        printf("-------------------------------------------------------------------\n");
        printf("=== RESULTADOS DA ANÁLISE DE DADOS ENADE PARA ALUNOS DE ADS ===\n");
        printf("-------------------------------------------------------------------\n");

        printf("\nQE_I15 - Ações Afirmativas para ingresso no Ensino Superior:\n");
        printf("  A (Não): %d\n", contadores_QE_I15_global[0]);
        printf("  B (Sim, por crit. étnico-racial): %d\n", contadores_QE_I15_global[1]);
        printf("  C (Sim, por crit. de renda): %d\n", contadores_QE_I15_global[2]);
        printf("  D (Sim, por esc. pública/bolsa): %d\n", contadores_QE_I15_global[3]);
        printf("  E (Sim, por combinação de critérios): %d\n", contadores_QE_I15_global[4]);
        printf("  F (Sim, por sist. de seleção diferente): %d\n", contadores_QE_I15_global[5]);
        printf("  *Respostas Vazias (QE_I15): %d*\n", contadores_QE_I15_vazio_global);
        printf("  Linhas ignoradas por incompletude (QE_I15): %d\n", ignoradas_I15_global);
        printf("  Total de alunos que entraram por ações afirmativas: %d\n", contadores_QE_I15_global[1] + contadores_QE_I15_global[2] + contadores_QE_I15_global[3] + contadores_QE_I15_global[4] + contadores_QE_I15_global[5]);

        printf("\nQE_I22 - Número de livros lidos no ano (exceto didáticos):\n");
        printf("  A (Nenhum): %d\n", contadores_QE_I22_global[0]);
        printf("  B (1 a 2 livros): %d\n", contadores_QE_I22_global[1]);
        printf("  C (3 a 5 livros): %d\n", contadores_QE_I22_global[2]);
        printf("  D (6 a 8 livros): %d\n", contadores_QE_I22_global[3]);
        printf("  E (Mais de 8 livros): %d\n", contadores_QE_I22_global[4]);
        printf("  *Respostas Vazias (QE_I22): %d*\n", contadores_QE_I22_vazio_global);
        printf("  Linhas ignoradas por incompletude (QE_I22): %d\n", ignoradas_I22_global);

        printf("\nQE_I23 - Quantas horas por semana, aproximadamente, você dedicou aos estudos, excetuando as horas de aula?\n");
        printf("  A (Nenhuma, apenas assisto às aulas): %d\n", contadores_QE_I23_global[0]);
        printf("  B (De uma a três): %d\n", contadores_QE_I23_global[1]);
        printf("  C (De quatro a sete): %d\n", contadores_QE_I23_global[2]);
        printf("  D (De oito a doze): %d\n", contadores_QE_I23_global[3]);
        printf("  E (Mais de doze): %d\n", contadores_QE_I23_global[4]);
        printf("  *Respostas Vazias (QE_I23): %d*\n", contadores_QE_I23_vazio_global);
        printf("  Linhas ignoradas por incompletude (QE_I23): %d\n", ignoradas_I23_global);

        printf("\n-------------------------------------------------------------------\n");
        printf("Total de registros de alunos de ADS analisados (para QE_I15, QE_I22, QE_I23): %d\n", total_alunos_ads_global);
        printf("-------------------------------------------------------------------\n");
    }

    MPI_Finalize();
    return 0;
}