#!/bin/bash

# Remove temp directory if it exists
rm -rf ./temp

# Compile the MPI program
mpic++ -std=c++17 -o mpreducer mpreducer.cpp

# Run the MPI program with 4 processes
mpirun -np 4 ./mpreducer 