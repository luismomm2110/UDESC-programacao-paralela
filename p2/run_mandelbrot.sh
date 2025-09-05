#!/bin/bash

# Script para executar o programa Mandelbrot híbrido MPI+OpenMP
# em ambiente heterogêneo

# Configurações padrão
HOSTFILE="hostfile.txt"
NP=5  # Número de processos MPI (um por nó)

# Verificar se o executável existe
if [ ! -f "mandelbrot" ]; then
    echo "Executável não encontrado. Compilando..."
    make
fi

# Verificar se o hostfile existe
if [ ! -f "$HOSTFILE" ]; then
    echo "Hostfile não encontrado: $HOSTFILE"
    echo "Crie um hostfile com os nomes dos seus nós."
    exit 1
fi

echo "=== Configuração do Cluster Híbrido MPI+OpenMP ==="
echo "Hostfile: $HOSTFILE"
echo "Número de processos MPI: $NP"
echo "Cada processo MPI usará todos os cores disponíveis no nó com OpenMP"
echo ""

# Configurar variáveis de ambiente OpenMP
export OMP_PROC_BIND=true
export OMP_PLACES=cores
export OMP_DISPLAY_ENV=true

# Exemplo de entrada para teste
echo "Exemplo de entrada: 1000 1000 100"
echo "Digite as dimensões e iterações ou pressione Ctrl+C para cancelar:"
read -p "max_row max_column max_n: " input

if [ -z "$input" ]; then
    input="1000 1000 100"
fi

echo ""
echo "=== Executando programa ==="
echo "Comando: mpirun -np $NP --hostfile $HOSTFILE --map-by ppr:1:node ./mandelbrot"
echo "Entrada: $input"
echo ""

# Executar o programa
echo $input | mpirun -np $NP --hostfile $HOSTFILE --map-by ppr:1:node ./mandelbrot

echo ""
echo "=== Execução concluída ===" 