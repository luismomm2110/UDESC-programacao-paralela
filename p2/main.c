//
// Created by luis.aduarte@jv01.local on 20/05/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "matriz.h"

int main(int argc, char **argv)
{
    int linhas = 0;
    int colunas = 0;
    matriz_t *A = NULL;
    matriz_t *B = NULL;
    matriz_t *C = NULL;
    matriz_t *D = NULL;

    MPI_Init(&argc, &argv);
    if ((argc != 2)) {
        printf("Uso: %s <N>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        linhas = atoi(argv[1]);
    }
    MPI_Bcast(&linhas, 1, MPI_INT, 0, MPI_COMM_WORLD);

    colunas = linhas;

    A = matriz_criar(linhas, colunas);
    B = matriz_criar(linhas, colunas);


    if (rank == 0) {
        matriz_preencher_rand(A);
        matriz_preencher_rand(B);
    }

    // aqui somente o mestre tem as informacoes
    MPI_Bcast(&A->dados[0][0], linhas * colunas, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&B->dados[0][0], linhas * colunas, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // aqui todos tem a mesma memoria
    C = matriz_somar(A, B);

    printf("HEHEHEHEHEHEHEHE\n");
    // juntar tudo
    int fatia = linhas / size;
    // ponto inicial
    MPI_Gather(&C->dados[0][0], fatia, MPI_DOUBLE, &C->dados[0][0], fatia, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    printf("Matriz C\n");
    matriz_imprimir(C);

    /*    D = matriz_multiplicar(A, B);

        printf("Matriz D\n");
        matriz_imprimir(D);

        matriz_destruir(A);
        matriz_destruir(B);
        matriz_destruir(C);
        matriz_destruir(D);
    */
    MPI_Finalize();
    return EXIT_SUCCESS;
}