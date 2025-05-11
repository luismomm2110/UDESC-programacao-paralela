#include <stdio.h>
#include <stdlib.h>
#include "matriz.h"
#include <pthread.h>
#include <assert.h>
#include <omp.h>

int main(int argc, char **argv) {
    if ((argc != 4)) {
        printf("Uso: %s <N> <NUM_THREADS> <PARALLEL_TYPE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int linhas = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int colunas = linhas;

    matriz_t *A = matriz_criar(linhas, colunas);
    matriz_t *B = matriz_criar(linhas, colunas);
    matriz_t *D = matriz_criar(linhas, colunas);
    // matriz_t *D_sequencial = matriz_criar(linhas, colunas);

    matriz_preencher_rand(B);
    matriz_preencher_rand(A);

    // Properly initialize thread_params
    thread_params parametros;
    parametros.A = A;
    parametros.B = B;
    parametros.D = D;
    parametros.num_threads = num_threads;

    // Compute OpenMP version
    matriz_multiplicar_openmp(&parametros);

    // Compute sequential version to compare
    // D_sequencial = matriz_multiplicar(A, B);
    //
    // // Verify the correctness
    // if (!matrizes_iguais(D, D_sequencial)) {
    //     fprintf(stderr, "Error: Matrices are not equal!\n");
    //     exit(EXIT_FAILURE);
    // }

    matriz_destruir(A);
    matriz_destruir(B);
    matriz_destruir(D);
    // matriz_destruir(D_sequencial);

    return EXIT_SUCCESS;
}
