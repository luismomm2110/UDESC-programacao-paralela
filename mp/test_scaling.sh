#!/bin/bash

echo "================================================"
echo "           TESTE DE ESCALABILIDADE"
echo "             MAP-REDUCE COM MPI"
echo "================================================"

# Verificar se o executável existe
if [ ! -f "./mpreducer" ]; then
    echo "Erro: mpreducer não encontrado. Execute 'make all' primeiro."
    exit 1
fi

# Array com diferentes números de processos para testar
PROCESS_COUNTS=(1 2 3 4 6 8)

for np in "${PROCESS_COUNTS[@]}"; do
    echo ""
    echo "================================================"
    echo "         EXECUTANDO COM $np PROCESSO(S)"
    echo "================================================"
    
    # Medir tempo de execução
    start_time=$(date +%s.%N)
    
    # Executar o programa
    mpirun -np $np ./mpreducer
    
    end_time=$(date +%s.%N)
    
    # Calcular tempo decorrido
    execution_time=$(echo "scale=3; $end_time - $start_time" | bc -l)
    echo ""
    echo "Tempo de execução: ${execution_time}s"
    echo "================================================"
    
    # Pequena pausa entre execuções
    sleep 1
done

echo ""
echo "================================================"
echo "              TESTE CONCLUÍDO"
echo "================================================" 