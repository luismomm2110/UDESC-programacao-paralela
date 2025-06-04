# Map-Reduce com MPI

Este projeto implementa um sistema Map-Reduce usando MPI (Message Passing Interface) para processamento paralelo distribuído.

## Características

- **Algoritmo Map-Reduce**: Implementação clássica do paradigma Map-Reduce
- **Comunicação MPI**: Usa `MPI_Allgather` e `MPI_Allgatherv` para coordenação
- **Contagem de Palavras**: Exemplo prático que conta frequência de palavras
- **Escalabilidade**: Funciona com qualquer número de processos MPI
- **Tipos Customizados**: Usa tipos de dados MPI personalizados para estruturas

## Arquivos

- `mpreducer.cpp`: Implementação principal do Map-Reduce
- `mpmapper.cpp`: Versão mais complexa com funcionalidades estendidas
- `Makefile`: Compilação simples com mpic++
- `CMakeLists.txt`: Build system alternativo com CMake

## Pré-requisitos

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y openmpi-bin openmpi-common libopenmpi-dev
```

### CentOS/RHEL
```bash
sudo yum install openmpi openmpi-devel
# ou para versões mais recentes:
sudo dnf install openmpi openmpi-devel
```

### Verificar Instalação
```bash
make check-mpi
```

## Compilação

### Usando Makefile (Recomendado)
```bash
# Compilar todos os programas
make all

# Compilar apenas o reducer
make mpreducer

# Compilar com debug
make debug

# Ver todas as opções
make help
```

### Usando CMake
```bash
mkdir build
cd build
cmake ..
make
```

## Execução

### Execução Básica
```bash
# Com 2 processos
mpirun -np 2 ./mpreducer

# Com 4 processos
mpirun -np 4 ./mpreducer

# Com 8 processos
mpirun -np 8 ./mpreducer
```

### Usando Makefile
```bash
# Executar com 2 processos
make run-2

# Executar com 4 processos
make run-4

# Executar com 8 processos
make run-8
```

## Exemplo de Saída

```
=== MAP-REDUCE COM MPI ===
Processos MPI: 4
Iniciando processamento...

Fase MAP concluída em todos os processos.

Fase REDUCE concluída.

=== RESULTADOS FINAIS ===
Palavra                    | Contagem
---------------------------|----------
mpi                        | 4
paralelo                   | 3
processamento              | 2
dados                      | 2
distribuído                | 2
...

Estatísticas:
- Total de palavras únicas: 45
- Total de palavras processadas: 78
```

## Como Funciona

### Fase MAP
1. Cada processo MPI recebe um conjunto de dados único
2. Processa localmente contando palavras
3. Remove pontuação e normaliza para minúsculas
4. Cria um mapa local palavra → contagem

### Fase REDUCE
1. Cada processo converte seu mapa local para estrutura serializável
2. `MPI_Allgather` coleta tamanhos de todos os processos
3. `MPI_Allgatherv` coleta todos os dados de palavra-contagem
4. Combina contagens de palavras idênticas
5. Processo master (rank 0) exibe resultados finais

## Estruturas de Dados

### KeyValue
```cpp
struct KeyValue {
    char key[128];  // Palavra (máximo 128 caracteres)
    int value;      // Contagem
};
```

### Tipo MPI Customizado
O programa define um tipo MPI personalizado para transmitir eficientemente a estrutura `KeyValue` entre processos.

## Personalização

### Modificar Dados de Entrada
Edite a função `generate_test_data()` em `mpreducer.cpp` para usar seus próprios dados:

```cpp
std::vector<std::string> generate_test_data(int rank) {
    // Seus dados aqui
    return data;
}
```

### Processar Arquivos
Para processar arquivos reais, modifique o código para ler de arquivos:

```cpp
std::ifstream file("dados.txt");
std::string line;
std::vector<std::string> data;
while (std::getline(file, line)) {
    data.push_back(line);
}
```

## Otimizações

- **Balanceamento de Carga**: Distribua dados uniformemente entre processos
- **Compressão**: Para grandes volumes, considere compressão dos dados
- **I/O Paralelo**: Use MPI-IO para ler arquivos grandes em paralelo
- **Memory Mapping**: Para arquivos muito grandes, considere memory mapping

## Troubleshooting

### Erro "mpirun não encontrado"
```bash
# Instalar MPI
make install-deps

# Verificar PATH
echo $PATH
which mpirun
```

### Erro de Compilação
```bash
# Verificar se mpic++ está disponível
which mpic++

# Compilar com debug para mais informações
make debug
```

### Problemas de Performance
- Use número de processos igual ao número de cores CPU
- Para clusters, configure adequadamente o arquivo de hosts MPI
- Monitore uso de memória com programas grandes

## Conceitos MPI Utilizados

- **MPI_Init/MPI_Finalize**: Inicialização e finalização
- **MPI_Comm_rank/MPI_Comm_size**: Identificação de processos
- **MPI_Barrier**: Sincronização
- **MPI_Allgather**: Coleta de dados de todos os processos
- **MPI_Allgatherv**: Coleta de dados de tamanhos variáveis
- **MPI_Type_create_struct**: Tipos de dados customizados

## Licença

Este projeto é para fins educacionais e pode ser usado livremente. 