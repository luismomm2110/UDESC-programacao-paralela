#!/bin/bash

MODE=$1
MATRIX_SIZE=1000

make 

if [ -z "$MODE" ]; then
  echo "Usage: $0 <mode>"
  exit 1
fi

{
if [ "$MODE" -eq 1 ]; then
  time ./main $MATRIX_SIZE 1 $MODE # warmup
  for i in {1..10}; do 
    echo "=== Running test with $i threads ==="
    export OMP_NUM_THREADS=$i
    time ./main $MATRIX_SIZE $i $MODE
  done
else
  echo "=== Running test sequentially ==="
  time ./main $MATRIX_SIZE 1 $MODE
fi
} | tee log
