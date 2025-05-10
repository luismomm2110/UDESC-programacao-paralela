#include <stdio.h>
#include <stdlib.h>
#include "matriz.h"
#include <pthread.h>
#include <assert.h>

int main(int argc, char **argv)
{
    int linhas = 0;
    int colunas = 0;
    int num_threads = 0;
    matriz_t *A = NULL;
    matriz_t *B = NULL;
    matriz_t *C = NULL;
    matriz_t *C_sequencial = NULL;
    matriz_t *D = NULL;
    matriz_t *B_transposta = NULL;
    matriz_t *D_sequencial = NULL;

    if ((argc != 4)) {
        printf("Uso: %s <N> <NUM_THREADS> <PARALLEL_TYPE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    linhas = atoi(argv[1]);
    num_threads = atoi(argv[2]);
    colunas = linhas; 

    A = matriz_criar(linhas, colunas); 
    B = matriz_criar(linhas, colunas); 
    matriz_preencher_rand(B);
    matriz_preencher_rand(A);
    B_transposta = matriz_criar(colunas, linhas);
    B_transposta = matriz_transpor(B);

	thread_params *parametros = NULL;
	pthread_t *threads = NULL;
	parametros = (thread_params*) malloc(sizeof(thread_params) * num_threads);
	threads = (pthread_t*) malloc(sizeof(pthread_t) * num_threads);
	
    C_sequencial  = matriz_somar(A, B);
    C = matriz_criar(linhas, colunas); 
    D = matriz_criar(linhas, colunas);
    D_sequencial = matriz_criar(linhas, colunas);
    D_sequencial = matriz_multiplicar(A, B);
    D = matriz_multiplicar_openmp(A, B);

    if (!matrizes_iguais(D, D_sequencial)) { fprintf(stderr, "Error: Matrices are not equal!\n");
         exit(EXIT_FAILURE);
    }

    matriz_destruir(A);
    matriz_destruir(B);
    matriz_destruir(C);
    matriz_destruir(C_sequencial);
    matriz_destruir(D);
    matriz_destruir(D_sequencial);

    return EXIT_SUCCESS;
}