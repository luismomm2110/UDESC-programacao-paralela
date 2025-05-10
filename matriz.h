#ifndef __MATRIZ_H
#define __MATRIZ_H

#define random() ((rand() ^ rand()) / (RAND_MAX + 1.0))
typedef struct {
      double **dados;
      int linhas;
      int colunas;
} matriz_t;

typedef struct {
	int tid;
	int num_threads; //numero de threads em execução
	matriz_t *A;
	matriz_t *B;
	matriz_t *C;
	matriz_t *D;
} thread_params;


matriz_t *matriz_criar(int linhas, int colunas);

void matriz_destruir(matriz_t *m);

void matriz_preencher_rand(matriz_t *m);

void matriz_preencher(matriz_t *m, double valor);

matriz_t *matriz_multiplicar(matriz_t *A, matriz_t *B);

void *matriz_somar_paralelo(void *args);

void *matriz_multiplicar_paralelo(void *args);

void *matriz_multiplicar_paralelo_transposta(void *args);

matriz_t *matriz_somar(matriz_t *A, matriz_t *B);

void matriz_imprimir(matriz_t *m);


int matrizes_iguais(matriz_t *a, matriz_t *b);

matriz_t *matriz_transpor(matriz_t *A);

matriz_t *matriz_multiplicar_openmp(matriz_t *A, matriz_t *B);

#endif 
