#include <stdio.h>
#include <mpi.h>

// hello world program in MPI


// passo 1 - distriburi executável
// encontra os hosts.txt

// compila mpicc mpi.c -o mpi
    // pesquisar como funciona esse  hosts para multi-nodes e multi-processos
    // o que é tag
// rodando o mpirun --machinefile hosts.txt -np 4 ./mpi
//passo 2 - despachar

int main(int argc, char** argv) {
    int id, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size); // quantidade de processos
    MPI_Comm_rank(MPI_COMM_WORLD, &id); // quem sou eu

    // MPI_COMM_WORLD é o comunicador padrão. é um domínio de comunicação


    printf("Hello World from process\n");

    MPI_Finalize();
}