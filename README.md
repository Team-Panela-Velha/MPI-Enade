# Projeto Final MPI - Análise de Dados ENADE 2014

## Descrição

Este projeto implementa uma análise paralela dos microdados do ENADE 2014, focando especificamente nos cursos de **Análise e Desenvolvimento de Sistemas (ADS)**. Utiliza MPI (Message Passing Interface) para distribuir o processamento entre múltiplos processos, permitindo análise eficiente de grandes volumes de dados.

## Objetivo

O programa analisa três questões específicas do questionário socioeconômico do ENADE 2014 para estudantes de ADS:

- **QE_I15**: Ações Afirmativas para ingresso no Ensino Superior
- **QE_I22**: Número de livros lidos no ano (exceto didáticos)
- **QE_I23**: Horas dedicadas aos estudos por semana (excetuando aulas)

## Funcionalidades

### Processamento Paralelo
- Utiliza MPI para distribuir arquivos entre processos
- Processo 0 (master) extrai informações dos cursos ADS
- Demais processos analisam subconjuntos dos dados
- Redução MPI para agregar resultados finais

### Análise de Dados
- Identifica cursos de ADS através do código de grupo (CO_GRUPO = 72)
- Conta respostas por categoria para cada questão
- Contabiliza respostas vazias e linhas incompletas
- Gera relatório detalhado dos resultados

## Estrutura dos Dados

### Arquivos de Entrada
O programa espera 42 arquivos de microdados no diretório `2.DADOS/`:
```
2.DADOS/
├── microdados2014_arq1.txt
├── microdados2014_arq2.txt
├── ...
└── microdados2014_arq42.txt
```

### Formato dos Dados
- Arquivos CSV separados por ponto e vírgula (;)
- Primeira linha contém cabeçalho com nomes das colunas
- Coluna 2: CO_CURSO (código do curso)
- Coluna 6: CO_GRUPO (código do grupo - 72 para ADS)

## Pré-requisitos

### Software Necessário
- Compilador C (GCC recomendado)
- Implementação MPI (OpenMPI ou MPICH)
- Sistema operacional compatível com MPI

### Instalação no Windows
```powershell
# Instalar Microsoft MPI
# Download: https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi

# Ou usando chocolatey
choco install msmpi
```

### Instalação no Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential
sudo apt install libopenmpi-dev openmpi-bin
```

## Compilação

### Windows (com MS-MPI)
```cmd
mpicc -o main main.c
```

### Linux (com OpenMPI)
```bash
mpicc -o main main.c
```

## Execução

### Execução Sequencial (1 processo)
```bash
mpirun -n 1 ./main
```

### Execução Paralela (múltiplos processos)
```bash
# Exemplo com 4 processos
mpirun -n 4 ./main

# Em cluster ou múltiplas máquinas
mpirun -n 8 -hostfile hosts ./main
```

## Resultados

O programa gera um relatório detalhado incluindo:

### QE_I15 - Ações Afirmativas
- A: Não utilizou ações afirmativas
- B: Sim, por critério étnico-racial
- C: Sim, por critério de renda
- D: Sim, por escola pública/bolsa
- E: Sim, por combinação de critérios
- F: Sim, por sistema de seleção diferente

### QE_I22 - Livros Lidos (exceto didáticos)
- A: Nenhum
- B: 1 a 2 livros
- C: 3 a 5 livros
- D: 6 a 8 livros
- E: Mais de 8 livros

### QE_I23 - Horas de Estudo por Semana
- A: Nenhuma (apenas assiste às aulas)
- B: De uma a três horas
- C: De quatro a sete horas
- D: De oito a doze horas
- E: Mais de doze horas

### Estatísticas Adicionais
- Total de alunos ADS analisados
- Número de respostas vazias por questão
- Linhas ignoradas por incompletude de dados

## Exemplo de Saída

```
-------------------------------------------------------------------
=== RESULTADOS DA ANÁLISE DE DADOS ENADE PARA ALUNOS DE ADS ===
-------------------------------------------------------------------

QE_I15 - Ações Afirmativas para ingresso no Ensino Superior:
  A (Não): 15420
  B (Sim, por crit. étnico-racial): 2341
  C (Sim, por crit. de renda): 3102
  ...

Total de registros de alunos de ADS analisados: 22847
-------------------------------------------------------------------
```

## Estrutura do Código

### Funções Principais
- `extrair_cursos_ads()`: Extrai códigos de cursos ADS do primeiro arquivo
- `contar_respostas()`: Conta respostas para uma questão específica
- `main()`: Coordena o processamento paralelo e agregação de resultados

### Constantes
- `TOTAL_ARQUIVOS`: 42 arquivos de dados
- `MAX_COURSES`: Limite máximo de cursos (100.000)
- `MAX_FILENAME`: Tamanho máximo do nome do arquivo (200 chars)

## Performance

### Benefícios do Paralelismo
- Redução significativa do tempo de processamento
- Escalabilidade horizontal (mais processos = menor tempo)
- Eficiência na análise de grandes volumes de dados

### Recomendações
- Use número de processos múltiplo do número de cores disponíveis
- Para datasets pequenos, overhead do MPI pode não compensar
- Teste diferentes configurações para encontrar o ponto ótimo

## Limitações

- Arquivos devem estar no formato CSV com separador `;`
- Assume estrutura específica das colunas dos microdados ENADE
- Focado apenas em cursos de ADS (CO_GRUPO = 72)
- Requer todos os 42 arquivos de dados para execução completa

## Contribuição

Para contribuir com o projeto:

1. Fork o repositório
2. Crie uma branch para sua feature (`git checkout -b feature/nova-analise`)
3. Commit suas mudanças (`git commit -am 'Adiciona nova análise'`)
4. Push para a branch (`git push origin feature/nova-analise`)
5. Abra um Pull Request

## Licença

Este projeto está sob licença MIT. Veja o arquivo `LICENSE` para mais detalhes.

## Contato

Para dúvidas ou sugestões, entre em contato através do GitHub.

---

**Nota**: Este projeto foi desenvolvido como trabalho final da disciplina de Programação Paralela, demonstrando o uso prático de MPI para análise de dados educacionais.
