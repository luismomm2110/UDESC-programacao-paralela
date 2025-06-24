#!/bin/bash

if [[ "$1" == "--prepare-only" ]]; then
    # Remove temp directory if it exists
    rm -rf ./temp
    rm -rf ./output
    
    mkdir ./temp
    mkdir ./output
    
    # remove compiled files
    rm -f *.o
    rm -f mpreducer
    
    mpic++ -std=c++17 -O3 -fopenmp -o mpreducer mpreducer.cpp coordinator.cpp worker.cpp
    exit 0
fi

# Remove temp directory if it exists
rm -rf ./temp
rm -rf ./output

mkdir ./temp
mkdir ./output

# remove compiled files
rm -f *.o
rm -f mpreducer

# Remove temp directory if it exists

mpic++ -std=c++17 -O3 -fopenmp -o mpreducer mpreducer.cpp coordinator.cpp worker.cpp
export OMP_NUM_THREADS=4


# Run the MPI program with 4 processes
mpirun -np 4 --machinefile hosts.txt --mca btl_tcp_if_include 10.20.221.0/24 ./mpreducer 
EXIT_CODE=$?

exit $EXIT_CODE
