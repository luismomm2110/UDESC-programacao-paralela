#include <mpi.h>
#include <omp.h>
#include <complex>
#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char **argv){
	int max_row, max_column, max_n, rank, size;
	int msg[3];
	MPI_Status status;
	
	// inicia OPENMP com thread
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// obtem número de cores por nó
	int numero_de_cores = omp_get_max_threads();
	
	// junta informação sobre a quantidade de cores por nó
	int *cores_por_no = NULL;
	if (rank == 0) {
		cores_por_no = (int*)malloc(sizeof(int) * size);
	}
	MPI_Gather(&numero_de_cores, 1, MPI_INT, cores_por_no, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	if (rank == 0) {
		cin >> max_row;
		cin >> max_column;
		cin >> max_n;
		msg[0] = max_row;
		msg[1] = max_column;
		msg[2] = max_n;
		
		int total_cores = 0;
		for (int i = 0; i < size; i++) {
			total_cores += cores_por_no[i];
		}
	}

	MPI_Bcast(&msg, 3, MPI_INT, 0, MPI_COMM_WORLD);
	max_row = msg[0];
	max_column = msg[1];
	max_n = msg[2];
	
	// Transmitir informações de cores para todos os processos
	if (cores_por_no == NULL) {
		cores_por_no = (int*)malloc(sizeof(int) * size);
	}
	MPI_Bcast(cores_por_no, size, MPI_INT, 0, MPI_COMM_WORLD);
	
	char **matriz_completa = NULL;

	if (rank == 0) {
		matriz_completa = (char**)malloc(sizeof(char*)*max_row);
		for (int i=0; i<max_row;i++)
			matriz_completa[i]=(char*)malloc(sizeof(char)*max_column);
	}

    // Calcular distribuição ponderada de trabalho baseada na contagem de cores
	int total_de_cores = 0;
	for (int i = 0; i < size; i++) {
		total_de_cores += cores_por_no[i];
	}
	
	// Calcular quantas linhas este nó deve processar baseado na sua contagem de cores
	int meus_cores = cores_por_no[rank];
	int linhas_para_este_no = (max_row * meus_cores) / total_de_cores;
	
	// Calcular linha inicial para este nó
	int linha_inicial = 0;
	for (int i = 0; i < rank; i++) {
		linha_inicial += (max_row * cores_por_no[i]) / total_de_cores;
	}
	
	// Garantir que não excedemos max_row devido ao arredondamento
	if (rank == size - 1) {
		linhas_para_este_no = max_row - linha_inicial;
	}

	// Calcular a porção da matriz deste nó usando OpenMP
	char **matriz_parcial = (char**)malloc(sizeof(char*)*linhas_para_este_no);
	
	// Definir número de threads OpenMP para corresponder aos cores disponíveis
	omp_set_num_threads(meus_cores);
	
	// Computação paralela com OpenMP
	#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < linhas_para_este_no; i++) {
		char *row = (char*)malloc(sizeof(char)*max_column);
		
		for(int c = 0; c < max_column; ++c){
			complex<float> z;
			int n = 0;
			while(abs(z) < 2 && ++n < max_n) {
				z = pow(z, 2) + decltype(z)(
					(float)c * 2 / max_column - 1.5,
					(float)(linha_inicial + i) * 2 / max_row - 1
				);
			}
			row[c]=(n == max_n ? '#' : '.');
		}
		matriz_parcial[i] = row;
	}

	// Coletar resultados de todos os nós
	if (rank != 0) {
		// Enviar número de linhas primeiro
		MPI_Send(&linhas_para_este_no, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(&linha_inicial, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		
		// Enviar cada linha
		for (int i = 0; i < linhas_para_este_no; i++) {
			MPI_Send(matriz_parcial[i], max_column, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
		}
	} else {
		// Processo mestre: copiar seus próprios dados primeiro
		for (int i = 0; i < linhas_para_este_no; i++) {
			for(int c = 0; c < max_column; ++c){
				matriz_completa[linha_inicial + i][c] = matriz_parcial[i][c];
			}
		}
		
		// Receber dados de outros processos
		for (int par = 1; par < size; par++) {
			int linhas_recebidas, linha_inicial_recebida;
			// recebe o número de linhas e a linha inicial
			MPI_Recv(&linhas_recebidas, 1, MPI_INT, par, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(&linha_inicial_recebida, 1, MPI_INT, par, 1, MPI_COMM_WORLD, &status);
			// recebe as linhas
			for (int i = 0; i < linhas_recebidas; i++) {
				MPI_Recv(matriz_completa[linha_inicial_recebida + i], max_column, MPI_CHAR, par, 2, MPI_COMM_WORLD, &status);
			}
		}

		// Imprimir a matriz completa
		// for(int r = 0; r < max_row; ++r){
		// 	for(int c = 0; c < max_column; ++c)
		// 		std::cout << matriz_completa[r][c];
		// 	cout << '\n';
		// }
	}
	
	// Limpeza de memória
	free(cores_por_no);
	for (int i = 0; i < linhas_para_este_no; i++) {
		free(matriz_parcial[i]);
	}
	free(matriz_parcial);
	
	if (rank == 0) {
		for (int i = 0; i < max_row; i++) {
			free(matriz_completa[i]);
		}
		free(matriz_completa);
	}
	
	MPI_Finalize();
}


