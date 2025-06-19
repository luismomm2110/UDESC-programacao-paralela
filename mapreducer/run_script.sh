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

# Run the MPI program with 4 processes
mpirun --machinefile hosts.txt  --mca btl_tcp_if_include 10.20.221.0/24 mpreducer


exit $EXIT_CODE                # propaga código de saída do MPI
