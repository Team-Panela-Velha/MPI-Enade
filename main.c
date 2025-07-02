#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 200
#define TOTAL_ARQUIVOS 42
#define MAX_COURSES 100000

int cursos_ads[MAX_COURSES];
int num_cursos_ads = 0;

// Extracts ADS courses (group 72) from the first file
void extrair_cursos_ads(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        fprintf(stderr, "Rank 0: Erro ao abrir %s para extrair cursos ADS.\n", filename);
        return;
    }

    char linha[1024];
    fgets(linha, sizeof(linha), fp); // Reads the header

    int linhas_ignoradas_ads = 0; // Renamed to avoid confusion with other ignored lines

    while (fgets(linha, sizeof(linha), fp))
    {
        char copia[1024];
        strcpy(copia, linha);

        int col = 0;
        int co_grupo = -1, co_curso = -1;

        // --- First pass to count columns and check integrity ---
        char *token_tmp = strtok(copia, ";");
        int num_colunas = 0;
        while (token_tmp != NULL)
        {
            num_colunas++;
            token_tmp = strtok(NULL, ";");
        }

        // We need columns 2 (CO_CURSO) and 6 (CO_GRUPO)
        if (num_colunas < 6 || num_colunas < 2)
        {
            linhas_ignoradas_ads++;
            continue; // Ignores incomplete lines
        }

        // --- Second pass to extract values ---
        strcpy(copia, linha);           // Restores the line for strtok again
        token_tmp = strtok(copia, ";"); // Re-tokenizes
        col = 0;                        // Resets the column counter

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

// Generic function to count responses for a question
// Now includes a counter for empty responses (ultimo_contador_vazio)
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
    int idx_co_curso = -1;

    // Reads the header to find the index of the question column
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
            }
            if (strcmp(token, "CO_CURSO") == 0)
            {
                idx_co_curso = col;
            }
            token = strtok(NULL, ";");
        }
        free(copia);
    }

    // If the question or CO_CURSO was not found in the header, we cannot process
    if (idx_questao == -1)
    {
        fprintf(stderr, "Aviso: Questão '%s' não encontrada no cabeçalho de %s. Arquivo ignorado para esta questão.\n", questao, filename);
        fclose(fp);
        return;
    }

    // Check if CO_CURSO is found. It's essential for filtering.
    if (idx_co_curso == -1)
    {
        fprintf(stderr, "Aviso: Coluna 'CO_CURSO' não encontrada no cabeçalho de %s. Arquivo ignorado.\n", filename);
        fclose(fp);
        return;
    }

    *linhas_ignoradas = 0; // Resets the counter of ignored lines for this file/question

    // Reads data lines
    while (fgets(linha, sizeof(linha), fp))
    {
        char copia_linha[2048];
        strcpy(copia_linha, linha); // Works on a copy of the line

        // --- First pass to count columns and validate integrity ---
        int num_colunas = 0;
        char *token_tmp = strtok(copia_linha, ";");
        while (token_tmp != NULL)
        {
            num_colunas++;
            token_tmp = strtok(NULL, ";");
        }

        // The line must have at least the course and question columns
        if (num_colunas < idx_questao || num_colunas < idx_co_curso)
        {
            (*linhas_ignoradas)++;
            continue; // Ignores incomplete line
        }

        // --- Second pass to extract relevant values ---
        strcpy(copia_linha, linha);             // Restores the original line for strtok again
        char *token = strtok(copia_linha, ";"); // Re-tokenizes
        int col = 0;                            // Resets the column counter
        int co_curso = -1;
        char resposta[20] = {0}; // Buffer for the current question's response

        while (token != NULL)
        {
            col++;
            if (col == idx_co_curso)
                co_curso = atoi(token); // CO_CURSO is the 2nd column
            if (col == idx_questao)
            {
                strncpy(resposta, token, sizeof(resposta) - 1);
                resposta[sizeof(resposta) - 1] = '\0'; // Ensures null-termination
                // Removes newlines and extra quotes
                resposta[strcspn(resposta, "\r\n")] = 0;
                if (resposta[0] == '"')
                    memmove(resposta, resposta + 1, strlen(resposta));
                if (strlen(resposta) > 0 && resposta[strlen(resposta) - 1] == '"')
                    resposta[strlen(resposta) - 1] = '\0';
            }
            token = strtok(NULL, ";");
        }

        // Checks if the course is valid (is among the extracted ADS courses)
        int curso_valido = 0;
        for (int i = 0; i < num_cursos_ads; i++)
        {
            if (co_curso == cursos_ads[i])
            {
                curso_valido = 1;
                break;
            }
        }

        // If it is an ADS student, process the response
        if (curso_valido)
        {
            // Increments the total ADS student counter once per line
            if (strcmp(questao, "QE_I15") == 0)
            {
                (*num_alunos_ads_total_local)++; // Increments the total ADS students who passed the filter
            }

            if (strlen(resposta) == 0)
            {                               // If the response is empty
                (*ultimo_contador_vazio)++; // Increments the counter for empty responses
            }
            else
            { // If there is a response, categorize it
                if (strcmp(questao, "TP_SEXO") == 0)
                {
                    if (strcmp(resposta, "M") == 0 && tamanho_contadores > 0)
                        contadores[0]++;
                    else if (strcmp(resposta, "F") == 0 && tamanho_contadores > 1)
                        contadores[1]++;
                }
                else if (strcmp(questao, "QE_I21") == 0)
                {
                    if (strcmp(resposta, "A") == 0 && tamanho_contadores > 0)
                        contadores[0]++;
                    else if (strcmp(resposta, "B") == 0 && tamanho_contadores > 1)
                        contadores[1]++;
                }
                else if (strcmp(questao, "TP_PR_GER") == 0)
                {
                    int resp_int = atoi(resposta);
                    if (resp_int == 222 && tamanho_contadores > 0)
                        contadores[0]++;
                    else if (resp_int == 333 && tamanho_contadores > 1)
                        contadores[1]++;
                    else if (resp_int == 444 && tamanho_contadores > 2)
                        contadores[2]++;
                    else if (resp_int == 555 && tamanho_contadores > 3)
                        contadores[3]++;
                    else if (resp_int == 556 && tamanho_contadores > 4)
                        contadores[4]++;
                }
                else
                {
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
                        break;
                    case 'G':
                        if (tamanho_contadores > 6)
                            contadores[6]++;
                        break;
                    default:
                        break;
                    }
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

    // Local counters for questions and empty responses
    int contadores_QE_I15_local[6] = {0}; // A-F
    int contadores_QE_I15_vazio_local = 0;
    int contadores_QE_I22_local[5] = {0}; // A-E
    int contadores_QE_I22_vazio_local = 0;
    int contadores_QE_I23_local[5] = {0}; // A-E
    int contadores_QE_I23_vazio_local = 0;
    int contadores_TP_SEXO_local[2] = {0}; // M, F
    int contadores_TP_SEXO_vazio_local = 0;
    int contadores_QE_I18_local[5] = {0}; // A-E
    int contadores_QE_I18_vazio_local = 0;
    int contadores_QE_I19_local[7] = {0}; // A-G
    int contadores_QE_I19_vazio_local = 0;
    int contadores_QE_I21_local[2] = {0}; // A, B
    int contadores_QE_I21_vazio_local = 0;
    int contadores_TP_PR_GER_local[5] = {0}; // 222, 333, 444, 555, 556
    int contadores_TP_PR_GER_vazio_local = 0;

    // Global counters
    int contadores_QE_I15_global[6] = {0};
    int contadores_QE_I15_vazio_global = 0;
    int contadores_QE_I22_global[5] = {0};
    int contadores_QE_I22_vazio_global = 0;
    int contadores_QE_I23_global[5] = {0};
    int contadores_QE_I23_vazio_global = 0;
    int contadores_TP_SEXO_global[2] = {0};
    int contadores_TP_SEXO_vazio_global = 0;
    int contadores_QE_I18_global[5] = {0};
    int contadores_QE_I18_vazio_global = 0;
    int contadores_QE_I19_global[7] = {0};
    int contadores_QE_I19_vazio_global = 0;
    int contadores_QE_I21_global[2] = {0};
    int contadores_QE_I21_vazio_global = 0;
    int contadores_TP_PR_GER_global[5] = {0};
    int contadores_TP_PR_GER_vazio_global = 0;

    // Counters for ignored incomplete lines
    int ignoradas_I15_local = 0, ignoradas_I22_local = 0, ignoradas_I23_local = 0;
    int ignoradas_SEXO_local = 0, ignoradas_I18_local = 0, ignoradas_I19_local = 0, ignoradas_I21_local = 0;
    int ignoradas_I15_global = 0, ignoradas_I22_global = 0, ignoradas_I23_global = 0;
    int ignoradas_SEXO_global = 0, ignoradas_I18_global = 0, ignoradas_I19_global = 0, ignoradas_I21_global = 0;
    int ignoradas_PR_GER_local = 0, ignoradas_PR_GER_global = 0;

    // Total counter for analyzed ADS students (global)
    int total_alunos_ads_local = 0;
    int total_alunos_ads_global = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Mounts the names of the files to be read
    for (int i = 0; i < TOTAL_ARQUIVOS; i++)
    {
        snprintf(arquivos[i], MAX_FILENAME,
                 "2.DADOS/microdados2014_arq%d.txt", i + 1);
    }

    // Process 0 extracts ADS courses for all processes to use
    if (rank == 0)
    {
        extrair_cursos_ads(arquivos[0]);
        printf("Rank 0: Encontrados %d cursos de ADS (CO_GRUPO=72).\n", num_cursos_ads);
    }

    // Distributes course data to all processes
    MPI_Bcast(&num_cursos_ads, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(cursos_ads, num_cursos_ads, MPI_INT, 0, MPI_COMM_WORLD);

    // Divides files among processes (except file 0, which was used to extract courses)
    int arquivos_por_proc = (TOTAL_ARQUIVOS - 1) / size;
    int resto = (TOTAL_ARQUIVOS - 1) % size;
    int inicio = rank * arquivos_por_proc + (rank < resto ? rank : resto) + 1; // Starts from file 1 (index 0)
    int fim = inicio + arquivos_por_proc + (rank < resto ? 1 : 0);

    // Each process counts the responses of the files assigned to it
    for (int i = inicio; i < fim && i < TOTAL_ARQUIVOS; i++)
    {
        contar_respostas(arquivos[i], "QE_I15", 6, contadores_QE_I15_local, &ignoradas_I15_local, &total_alunos_ads_local, &contadores_QE_I15_vazio_local);
        contar_respostas(arquivos[i], "QE_I22", 5, contadores_QE_I22_local, &ignoradas_I22_local, &total_alunos_ads_local, &contadores_QE_I22_vazio_local);
        contar_respostas(arquivos[i], "QE_I23", 5, contadores_QE_I23_local, &ignoradas_I23_local, &total_alunos_ads_local, &contadores_QE_I23_vazio_local);
        contar_respostas(arquivos[i], "TP_SEXO", 2, contadores_TP_SEXO_local, &ignoradas_SEXO_local, &total_alunos_ads_local, &contadores_TP_SEXO_vazio_local);
        contar_respostas(arquivos[i], "QE_I18", 5, contadores_QE_I18_local, &ignoradas_I18_local, &total_alunos_ads_local, &contadores_QE_I18_vazio_local);
        contar_respostas(arquivos[i], "QE_I19", 7, contadores_QE_I19_local, &ignoradas_I19_local, &total_alunos_ads_local, &contadores_QE_I19_vazio_local);
        contar_respostas(arquivos[i], "QE_I21", 2, contadores_QE_I21_local, &ignoradas_I21_local, &total_alunos_ads_local, &contadores_QE_I21_vazio_local);
        contar_respostas(arquivos[i], "TP_PR_GER", 5, contadores_TP_PR_GER_local, &ignoradas_PR_GER_local, &total_alunos_ads_local, &contadores_TP_PR_GER_vazio_local);
    }

    // --- MPI Reduction: Sums local results to global in process 0 ---

    // Reduces counters for each question
    MPI_Reduce(contadores_QE_I15_local, contadores_QE_I15_global, 6, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I22_local, contadores_QE_I22_global, 5, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I23_local, contadores_QE_I23_global, 5, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_TP_SEXO_local, contadores_TP_SEXO_global, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I18_local, contadores_QE_I18_global, 5, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I19_local, contadores_QE_I19_global, 7, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_QE_I21_local, contadores_QE_I21_global, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(contadores_TP_PR_GER_local, contadores_TP_PR_GER_global, 5, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Reduces counters for empty responses
    MPI_Reduce(&contadores_QE_I15_vazio_local, &contadores_QE_I15_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I22_vazio_local, &contadores_QE_I22_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I23_vazio_local, &contadores_QE_I23_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_TP_SEXO_vazio_local, &contadores_TP_SEXO_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I18_vazio_local, &contadores_QE_I18_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I19_vazio_local, &contadores_QE_I19_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_QE_I21_vazio_local, &contadores_QE_I21_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&contadores_TP_PR_GER_vazio_local, &contadores_TP_PR_GER_vazio_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Reduces the total number of analyzed ADS students (to ensure it is summed once per student)
    MPI_Reduce(&total_alunos_ads_local, &total_alunos_ads_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Reduces counters for ignored incomplete lines
    MPI_Reduce(&ignoradas_I15_local, &ignoradas_I15_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I22_local, &ignoradas_I22_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I23_local, &ignoradas_I23_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_SEXO_local, &ignoradas_SEXO_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I18_local, &ignoradas_I18_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I19_local, &ignoradas_I19_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_I21_local, &ignoradas_I21_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ignoradas_PR_GER_local, &ignoradas_PR_GER_global, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Process 0 prints the aggregated results
    if (rank == 0)
    {
        printf("\n");
        printf("-------------------------------------------------------------------\n");
        printf("=== RESULTADOS DA ANÁLISE DE DADOS ENADE PARA ALUNOS DE ADS ===\n");
        printf("-------------------------------------------------------------------\n");

        printf("\n- Quantos alunos se matricularam no curso?\n");
        printf("   Total de alunos matriculados: %d\n", total_alunos_ads_global);

        printf("\nQE_I22 - Número de livros lidos no ano (exceto didáticos):\n");
        printf("   A (Nenhum): %d\n", contadores_QE_I22_global[0]);
        printf("   B (1 a 2 livros): %d\n", contadores_QE_I22_global[1]);
        printf("   C (3 a 5 livros): %d\n", contadores_QE_I22_global[2]);
        printf("   D (6 a 8 livros): %d\n", contadores_QE_I22_global[3]);
        printf("   E (Mais de 8 livros): %d\n", contadores_QE_I22_global[4]);
        printf("   Respostas Vazias (QE_I22): %d\n", contadores_QE_I22_vazio_global);
        printf("   Linhas ignoradas por incompletude (QE_I22): %d\n", ignoradas_I22_global);

        printf("\nQE_I23 - Quantas horas por semana, aproximadamente, você dedicou aos estudos, excetuando as horas de aula?\n");
        printf("   A (Nenhuma, apenas assisto às aulas): %d\n", contadores_QE_I23_global[0]);
        printf("   B (De uma a três): %d\n", contadores_QE_I23_global[1]);
        printf("   C (De quatro a sete): %d\n", contadores_QE_I23_global[2]);
        printf("   D (De oito a doze): %d\n", contadores_QE_I23_global[3]);
        printf("   E (Mais de doze): %d\n", contadores_QE_I23_global[4]);
        printf("   Respostas Vazias (QE_I23): %d\n", contadores_QE_I23_vazio_global);
        printf("   Linhas ignoradas por incompletude (QE_I23): %d\n", ignoradas_I23_global);

        // --- Added new questions ---
        printf("\nTP_SEXO - Sexo:\n");
        printf("   M (Masculino): %d\n", contadores_TP_SEXO_global[0]);
        printf("   F (Feminino): %d\n", contadores_TP_SEXO_global[1]);
        printf("   Respostas Vazias (TP_SEXO): %d\n", contadores_TP_SEXO_vazio_global);
        printf("   Linhas ignoradas por incompletude (TP_SEXO): %d\n", ignoradas_SEXO_global);
        printf("Porcentagem de estudantes do sexo Feminino: %.2f%%\n", ((double)contadores_TP_SEXO_global[1] / total_alunos_ads_global * 100.0));

        printf("\nQE_I18 - Qual modalidade de ensino médio você concluiu?\n");
        printf("   A (Ensino médio tradicional): %d\n", contadores_QE_I18_global[0]);
        printf("   B (Profissionalizante técnico): %d\n", contadores_QE_I18_global[1]);
        printf("   C (Profissionalizante magistério): %d\n", contadores_QE_I18_global[2]);
        printf("   D (Educação de Jovens e Adultos (EJA) e/ou Supletivo): %d\n", contadores_QE_I18_global[3]);
        printf("   E (Outra modalidade): %d\n", contadores_QE_I18_global[4]);
        printf("   Respostas Vazias (QE_I18): %d\n", contadores_QE_I18_vazio_global);
        printf("   Linhas ignoradas por incompletude (QE_I18): %d\n", ignoradas_I18_global);
        printf("Porcentagem de estudantes que cursaram o ensino técnico no ensino médio: %.2f%%\n", ((double)contadores_QE_I18_global[1] / total_alunos_ads_global * 100.0));

        printf("\nQE_I19 - Quem mais lhe incentivou a cursar a graduação?\n");
        printf("   A (Ninguém): %d\n", contadores_QE_I19_global[0]);
        printf("   B (Pais): %d\n", contadores_QE_I19_global[1]);
        printf("   C (Outros membros da família que não os pais): %d\n", contadores_QE_I19_global[2]);
        printf("   D (Professores): %d\n", contadores_QE_I19_global[3]);
        printf("   E (Líder ou representante religioso): %d\n", contadores_QE_I19_global[4]);
        printf("   F (Colegas/Amigos): %d\n", contadores_QE_I19_global[5]);
        printf("   G (Outras pessoas): %d\n", contadores_QE_I19_global[6]);
        printf("   Respostas Vazias (QE_I19): %d\n", contadores_QE_I19_vazio_global);
        printf("   Linhas ignoradas por incompletude (QE_I19): %d\n", ignoradas_I19_global);

        printf("\nQE_I21 - Alguém em sua família concluiu um curso superior?\n");
        printf("   A (Sim): %d\n", contadores_QE_I21_global[0]);
        printf("   B (Não): %d\n", contadores_QE_I21_global[1]);
        printf("   Respostas Vazias (QE_I21): %d\n", contadores_QE_I21_vazio_global);
        printf("   Linhas ignoradas por incompletude (QE_I21): %d\n", ignoradas_I21_global);

        printf("\nTP_PR_GER - Tipo de presença na prova:\n");
        printf("   222 (Não se aplica - ausente): %d\n", contadores_TP_PR_GER_global[0]);
        printf("   333 (Prova em branco): %d\n", contadores_TP_PR_GER_global[1]);
        printf("   444 (Participação indevida por protesto): %d\n", contadores_TP_PR_GER_global[2]);
        printf("   555 (Respostas válidas): %d\n", contadores_TP_PR_GER_global[3]);
        printf("   556 (Resultado desconsiderado): %d\n", contadores_TP_PR_GER_global[4]);
        printf("   Respostas Vazias (TP_PR_GER): %d\n", contadores_TP_PR_GER_vazio_global);
        printf("   Linhas ignoradas por incompletude (TP_PR_GER): %d\n", ignoradas_PR_GER_global);

        printf("\n-------------------------------------------------------------------\n");
        printf("Total de registros de alunos de ADS analisados (para todas as questões): %d\n", total_alunos_ads_global);
        printf("-------------------------------------------------------------------\n");
    }

    MPI_Finalize();
    return 0;
}