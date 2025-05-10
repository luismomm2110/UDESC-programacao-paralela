#include <stdio.h>
#include <stdlib.h>
#include "matriz.h"
#include <pthread.h>

int main(int argc, char **argv)
{
    int linhas = 0;
    int colunas = 0;
    int num_threads = 0;
    matriz_t *A = NULL;
    matriz_t *B = NULL;
    matriz_t *C = NULL;
    matriz_t *D = NULL;

    if ((argc != 4)) {
        printf("Uso: %s <N> <NUM_THREADS> <PARALLEL_TYPE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    linhas = atoi(argv[1]);
    num_threads = atoi(argv[2]);
    colunas = linhas; 

    A = matriz_criar(linhas, colunas); 
    B = matriz_criar(linhas, colunas); 
    D = matriz_criar(linhas, colunas);

    matriz_preencher_rand(A);
    matriz_preencher_rand(B);

    //printf("Matriz A\n");
    //matriz_imprimir(A);

    //printf("Matriz B\n");
    //matriz_imprimir(B);

	// aqui criamos tudo
	thread_params *parametros = NULL;
	pthread_t *threads = NULL;
	parametros = (thread_params*) malloc(sizeof(thread_params) * num_threads);
	threads = (pthread_t*) malloc(sizeof(pthread_t) * num_threads);
	
    	C = matriz_criar(linhas, colunas); 
	for (int i = 0; i < num_threads; i++) {
		parametros[i].tid = i;
		parametros[i].A = A;
		parametros[i].B = B;
		parametros[i].C = C;
        parametros[i].D = D;
		parametros[i].num_threads = num_threads;
		pthread_create(&threads[i], NULL, matriz_multiplicar_paralelo, &parametros[i]);
	}

    //C = matriz_somar(A, B);
    for (int i = 0; i < num_threads; i++) {
	    pthread_join(threads[i], NULL);
    }


//    printf("Matriz C\n");
//    matriz_imprimir(C);

    D = matriz_multiplicar(A, B);

/*    printf("Matriz D\n");
    matriz_imprimir(D);
*/
    matriz_destruir(A);
    matriz_destruir(B);
    matriz_destruir(C);
    matriz_destruir(D);

    return EXIT_SUCCESS;
}