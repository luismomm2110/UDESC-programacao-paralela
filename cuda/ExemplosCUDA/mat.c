#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void matrixMulOnHost(float* M, float* N, float* P, int width);
void dados(float *M, int size);

int main(int argc, char *argv[]) {
	float *M;
	float *N;
	float *P;
	int size;
	double start_time, end_time;

	if (argc != 2) {
		printf("Informe size\n");
		exit(1);
	}

	size = atoi(argv[1]);

	M = (float *) malloc(sizeof(float * size));
	N = (float *) malloc(sizeof(float * size));
	P = (float *) malloc(sizeof(float * size));

	dados(M, size);
	dados(N, size);

	start_time = wtime();
	matrixMulOnHost(M, N, P, size);
	end_time = wtime();

	free(M);
	free(N);
	free(P);
	
	printf("%d %f\n", size, end_time - start_time);

	return 0;
}

double wtime()
{
   struct timeval t;
   gettimeofday(&t, NULL);
   return t.tv_sec + t.tv_usec / 1000000.0;
}

void dados(float *M, int size)
{
	int i, j;
	srand(time(NULL));
	for (int i = 0; i < size; ++i) {
        	for (int j = 0; j < size; ++j) {
			M[i][j] = rand();
		}
	}
}

void matrixMulOnHost(float* M, float* N, float* P, int width) {
        for (int i = 0; i < width; ++i) {
                for (int j = 0; j < width; ++j) {
                        double sum = 0;
                        for (int k = 0; k < width; ++k) {
                                double a = M[i * width + k];
                                double b = N[k * width + j];
                                sum += a * b;
                        }
                        P[i * width + j] = sum;
                }
        }       
}

