#!/bin/bash

echo "=== Benchmark de Performance com Diferentes Combinações MPI/OpenMP ==="
echo "Executando 10 vezes para cada configuração..."
echo ""

# Obter número máximo de cores disponíveis
max_cores=$(nproc)
echo "Número máximo de cores disponíveis: $max_cores"
echo ""

# Criar arquivo CSV com cabeçalho
csv_file="benchmark_results.csv"
echo "mpi_processos,openmp_threads,execucao,tempo_segundos" > "$csv_file"

# Array com diferentes números de processos MPI
mpi_processes=(2 3 4 5)

# Array com diferentes números de threads OpenMP (1 até max_cores)
openmp_threads=()
for ((i=1; i<=max_cores; i++)); do
    openmp_threads+=($i)
done

# Preparar ambiente uma vez
echo "Preparando ambiente..."
./run_script.sh --prepare-only > /dev/null 2>&1

for np in "${mpi_processes[@]}"; do
    for nt in "${openmp_threads[@]}"; do
        echo "Executando com $np processos MPI e $nt threads OpenMP (10 execuções)..."
        
        for execution in {1..10}; do
            echo "  Execução $execution/10..."
            
            # Medir o tempo de execução apenas do mpreducer
            start_time=$(date +%s.%N)
            OMP_NUM_THREADS=$nt mpirun -np $np --machinefile hosts.txt --mca btl_tcp_if_include 10.20.221.0/24 ./mpreducer > /dev/null 2>&1
            end_time=$(date +%s.%N)
            
            # Calcular a diferença de tempo
            execution_time=$(echo "$end_time - $start_time" | bc -l)
            
            # Salvar no CSV
            echo "$np,$nt,$execution,$execution_time" >> "$csv_file"
            
            echo "    Tempo: ${execution_time} segundos"
        done
        
        echo "----------------------------------------"
    done
done

echo "Benchmark concluído!"
echo "Resultados salvos em: $csv_file"

# Mostrar estatísticas resumidas
echo ""
echo "=== Estatísticas Resumidas ==="
for np in "${mpi_processes[@]}"; do
    for nt in "${openmp_threads[@]}"; do
        echo "MPI: $np, OpenMP: $nt"
        avg_time=$(awk -F',' -v np="$np" -v nt="$nt" '$1 == np && $2 == nt {sum+=$4; count++} END {print sum/count}' "$csv_file")
        min_time=$(awk -F',' -v np="$np" -v nt="$nt" '$1 == np && $2 == nt {if(min=="") min=$4; if($4<min) min=$4} END {print min}' "$csv_file")
        max_time=$(awk -F',' -v np="$np" -v nt="$nt" '$1 == np && $2 == nt {if($4>max) max=$4} END {print max}' "$csv_file")
        echo "  Média: ${avg_time} segundos"
        echo "  Mínimo: ${min_time} segundos"
        echo "  Máximo: ${max_time} segundos"
        echo ""
    done
done
