# Projeto Final MPI - Análise de Dados ENADE 2014

## Descrição

Este projeto implementa uma análise paralela dos microdados do ENADE 2014, focando especificamente nos cursos de **Análise e Desenvolvimento de Sistemas (ADS)**. Utiliza MPI (Message Passing Interface) para distribuir o processamento entre múltiplos processos, permitindo análise eficiente de grandes volumes de dados.

## Objetivo

O programa analisa diversas questões específicas do questionário socioeconômico do ENADE 2014 para estudantes de ADS, incluindo:

- **QE_I15**: Ações Afirmativas para ingresso no Ensino Superior
- **QE_I22**: Número de livros lidos no ano (exceto didáticos)
- **QE_I23**: Horas dedicadas aos estudos por semana (excetuando aulas)
- **TP_SEXO**: Sexo do estudante
- **QE_I18**: Modalidade de ensino médio concluída
- **QE_I19**: Situação de trabalho durante a graduação
- **QE_I21**: Participação em atividades extracurriculares
- **TP_PR_GER**: Nota geral do curso (faixas de desempenho)

Essas questões são analisadas para identificar padrões e tendências entre os estudantes de ADS, considerando também respostas vazias e linhas incompletas.

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
O programa espera 42 arquivos de microdados no diretório `2.DADOS/`. Para baixar os arquivos, acesse o link oficial do INEP:

[Download dos Microdados ENADE 2014](https://download.inep.gov.br/microdados/microdados_enade_2014_LGPD.zip)

**Nota:** Após o download, extraia os arquivos e coloque a pasta `2.DADOS` no diretório raiz do projeto.

```
Projeto MPI/
├── main.c
├── README.md
├── 2.DADOS/
│   ├── microdados2014_arq1.txt
│   ├── microdados2014_arq2.txt
│   ├── ...
│   └── microdados2014_arq42.txt
```

### Formato dos Dados
- Arquivos CSV separados por ponto e vírgula (;)
- Primeira linha contém cabeçalho com nomes das colunas

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

### Linux (com OpenMPI) ou Windows (com MS-MPI)
```bash
mpicc -o main main.c
```

## Execução

### Execução Paralela (múltiplos processos)
```bash
# Exemplo com 4 processos
mpirun -n 4 ./main
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

## Licença

Este projeto está sob licença MIT. Veja o arquivo `LICENSE` para mais detalhes.

## Desenvolvedores

<table align="center">
  <tr>
    <td align="center">
      <a href="https://github.com/JoYoneyama">
        <img src="https://img.shields.io/badge/JoYoneyama-github?style=flat&logo=github&logoColor=white&label=github&labelColor=gray&color=blue" alt="JoYoneyama GitHub">
      </a><br />
      <sub><b>João Vitor Yoneyama</b></sub><br />
    </td>
    <td align="center">
      <a href="https://github.com/KaykyMatos845">
        <img src="https://img.shields.io/badge/github-KaykyMatos845-blue?style=plastic&logo=github&logoColor=white&labelColor=gray&color=blue" alt="KaykyMatos845 GitHub">
      </a><br />
      <sub><b>Kayky Matos</b></sub><br />
    </td>
    <td align="center">
      <a href="https://github.com/Mathlps">
        <img src="https://img.shields.io/badge/github-Mathlps-blue?style=plastic&logo=github&logoColor=white&labelColor=gray&color=blue" alt="Mathlps GitHub">
      </a><br />
      <sub><b>Matheus Lopes</b></sub><br />
    </td>
    <td align="center">
      <a href="https://github.com/PaulingCavalcante">
        <img src="https://img.shields.io/badge/github-PaulingCavalcante-blue?style=plastic&logo=github&logoColor=white&labelColor=gray&color=blue" alt="PaulingCavalcante GitHub">
      </a><br />
      <sub><b>Paulo Cavalcante</b></sub><br />
    </td>
    <td align="center">
      <a href="https://github.com/paolaabrantes">
        <img src="https://img.shields.io/badge/github-PaulingCavalcante-blue?style=plastic&logo=github&logoColor=white&labelColor=gray&color=blue" alt="paolaabrantes GitHub">
      </a><br />
      <sub><b>Paola Abrantes</b></sub><br />
    </td>
  </tr>
</table>

---

**Nota**: Este projeto foi desenvolvido como trabalho final da disciplina de Programação Paralela, demonstrando o uso prático de MPI para análise de dados educacionais.