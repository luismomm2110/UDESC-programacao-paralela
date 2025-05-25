# Mandelbrot Híbrido MPI+OpenMP

Este programa implementa o cálculo do conjunto de Mandelbrot usando uma abordagem híbrida que combina MPI e OpenMP para ambientes heterogêneos.

## Características

- **MPI**: Paralelização entre nós (processos distribuídos)
- **OpenMP**: Paralelização dentro de cada nó (threads compartilhadas)
- **Balanceamento Automático**: Distribui trabalho baseado no número de cores de cada nó
- **Ambiente Heterogêneo**: Suporta nós com diferentes números de cores

## Ambiente Heterogêneo Suportado

O programa foi projetado para funcionar em clusters com:
- 3 nós com 6 cores cada
- 2 nós com 4 cores cada
- Total: 26 cores distribuídos

## Arquivos

- `mandelbrot.cpp`: Código principal híbrido MPI+OpenMP
- `Makefile`: Script de compilação com flags apropriadas
- `hostfile.txt`: Configuração dos nós do cluster
- `run_mandelbrot.sh`: Script para execução facilitada
- `README_hibrido.md`: Esta documentação

## Compilação

```bash
make
```

O Makefile usa:
- `mpicxx`: Compilador MPI para C++
- `-fopenmp`: Flag para suporte OpenMP
- `-O3`: Otimização máxima
- `-std=c++11`: Padrão C++11

## Configuração do Hostfile

Edite o arquivo `hostfile.txt` com os nomes reais dos seus nós:

```
# Substitua pelos nomes/IPs reais dos seus nós
no1.cluster.local slots=6
no2.cluster.local slots=6
no3.cluster.local slots=6
no4.cluster.local slots=4
no5.cluster.local slots=4
```

## Execução

### Método 1: Script Automatizado
```bash
./run_mandelbrot.sh
```

### Método 2: Comando Direto
```bash
echo "1000 1000 100" | mpirun -np 5 --hostfile hostfile.txt --map-by ppr:1:node ./mandelbrot
```

### Método 3: Via Makefile
```bash
make run
```

## Como Funciona

### 1. Inicialização MPI
- Cada nó executa um processo MPI
- `MPI_Init_thread()` permite threads OpenMP

### 2. Detecção Automática de Cores
- Cada processo detecta cores disponíveis com `omp_get_max_threads()`
- Informações são coletadas pelo processo mestre

### 3. Distribuição Ponderada de Trabalho
- Trabalho é distribuído proporcionalmente aos cores disponíveis
- Nós com mais cores recebem mais linhas para processar

### 4. Computação Paralela
- **MPI**: Cada nó processa sua fatia de linhas
- **OpenMP**: Threads paralelas processam linhas simultaneamente
- `#pragma omp parallel for schedule(dynamic)`

### 5. Coleta de Resultados
- Processo mestre coleta resultados de todos os nós
- Matriz final é montada e impressa

## Exemplo de Balanceamento

Para uma matriz 1000x1000:
- **Nó 1 (6 cores)**: ~231 linhas (23.1%)
- **Nó 2 (6 cores)**: ~231 linhas (23.1%) 
- **Nó 3 (6 cores)**: ~231 linhas (23.1%)
- **Nó 4 (4 cores)**: ~154 linhas (15.4%)
- **Nó 5 (4 cores)**: ~153 linhas (15.3%)

## Variáveis de Ambiente OpenMP

O script configura automaticamente:
```bash
export OMP_PROC_BIND=true      # Fixar threads aos cores
export OMP_PLACES=cores        # Usar cores físicos
export OMP_DISPLAY_ENV=true    # Mostrar configuração OpenMP
```

## Entrada

O programa espera três valores:
```
max_row max_column max_n
```

Exemplo:
```
1000 1000 100
```

## Saída

- Informações sobre nós e cores detectados
- Matriz ASCII representando o conjunto de Mandelbrot
- `#` para pontos no conjunto
- `.` para pontos fora do conjunto

## Requisitos

- OpenMPI ou MPICH instalado
- Compilador com suporte OpenMP (gcc/g++)
- Acesso SSH entre nós (se usando múltiplas máquinas)
- Arquivo executável acessível em todos os nós

## Troubleshooting

### Problema: "cannot open source file mpi.h"
- Instale OpenMPI: `sudo apt-get install libopenmpi-dev`

### Problema: "cannot open source file omp.h"  
- Instale GCC com OpenMP: `sudo apt-get install gcc-multilib`

### Problema: Nós não encontrados
- Verifique conectividade SSH
- Confirme nomes dos nós no hostfile
- Use IPs se necessário

## Performance

A combinação MPI+OpenMP oferece:
- **Escalabilidade**: Entre e dentro dos nós
- **Eficiência**: Uso máximo de recursos disponíveis
- **Flexibilidade**: Adapta-se a ambientes heterogêneos
- **Balanceamento**: Distribui trabalho proporcionalmente 