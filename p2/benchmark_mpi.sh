#!/bin/bash

# Script para comparar performance das diferentes versões de comunicação MPI

HOSTFILE="hostfile.txt"
NP=5
TEST_SIZE="1000 1000 100"

echo "=== BENCHMARK DE COMUNICAÇÃO MPI ==="
echo "Teste: $TEST_SIZE"
echo "Processos: $NP"
echo "Hostfile: $HOSTFILE"
echo ""

# Verificar se os executáveis existem
if [ ! -f "mandelbrot" ] || [ ! -f "mandelbrot_otimizado" ] || [ ! -f "mandelbrot_pipeline" ]; then
    echo "Compilando todas as versões..."
    make clean && make
    echo ""
fi

# Configurar ambiente OpenMP para benchmark
export OMP_PROC_BIND=true
export OMP_PLACES=cores
export OMP_DISPLAY_ENV=false

echo "=== VERSÃO ORIGINAL (muitas mensagens pequenas) ==="
echo "Medindo tempo..."
start_time=$(date +%s.%N)
echo $TEST_SIZE | mpirun -np $NP --hostfile $HOSTFILE --map-by ppr:1:node ./mandelbrot > /dev/null
end_time=$(date +%s.%N)
original_time=$(echo "$end_time - $start_time" | bc)
echo "Tempo original: ${original_time}s"
echo ""

echo "=== VERSÃO OTIMIZADA (MPI_Gatherv) ==="
echo "Medindo tempo..."
start_time=$(date +%s.%N)
echo $TEST_SIZE | mpirun -np $NP --hostfile $HOSTFILE --map-by ppr:1:node ./mandelbrot_otimizado > /dev/null
end_time=$(date +%s.%N)
otimized_time=$(echo "$end_time - $start_time" | bc)
echo "Tempo otimizado: ${otimized_time}s"
echo ""

echo "=== VERSÃO PIPELINE (comunicação não-bloqueante) ==="
echo "Medindo tempo..."
start_time=$(date +%s.%N)
echo $TEST_SIZE | mpirun -np $NP --hostfile $HOSTFILE --map-by ppr:1:node ./mandelbrot_pipeline > /dev/null
end_time=$(date +%s.%N)
pipeline_time=$(echo "$end_time - $start_time" | bc)
echo "Tempo pipeline: ${pipeline_time}s"
echo ""

echo "=== RESULTADOS COMPARATIVOS ==="
echo "Original:   ${original_time}s"
echo "Otimizado:  ${otimized_time}s"
echo "Pipeline:   ${pipeline_time}s"
echo ""

# Calcular speedup
if [ $(echo "$original_time > 0" | bc) -eq 1 ]; then
    speedup_opt=$(echo "scale=2; $original_time / $otimized_time" | bc)
    speedup_pipe=$(echo "scale=2; $original_time / $pipeline_time" | bc)
    
    echo "Speedup da versão otimizada: ${speedup_opt}x"
    echo "Speedup da versão pipeline: ${speedup_pipe}x"
    
    improvement_opt=$(echo "scale=1; ($original_time - $otimized_time) / $original_time * 100" | bc)
    improvement_pipe=$(echo "scale=1; ($original_time - $pipeline_time) / $original_time * 100" | bc)
    
    echo "Melhoria otimizada: ${improvement_opt}%"
    echo "Melhoria pipeline: ${improvement_pipe}%"
fi

echo ""
echo "=== ANÁLISE DE COMUNICAÇÃO ==="
total_data=$(echo "$TEST_SIZE" | awk '{print $1 * $2}')
echo "Volume total de dados: $total_data bytes"

# Estimar número de mensagens
echo "Mensagens estimadas:"
echo "- Original: ~$(echo "$TEST_SIZE" | awk '{print $1}') mensagens pequenas"
echo "- Otimizado: 1 operação coletiva (MPI_Gatherv)"
echo "- Pipeline: ~$((NP * 4)) mensagens em chunks sobrepostos"

echo ""
echo "=== RECOMENDAÇÕES ==="
echo "✓ Use versão OTIMIZADA para máximo throughput"
echo "✓ Use versão PIPELINE para latência mínima e overlap"
echo "✓ Evite versão ORIGINAL em redes de alta latência" 