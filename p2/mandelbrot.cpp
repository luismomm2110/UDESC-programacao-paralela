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
	
	// Initialize MPI with thread support
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// Get number of available cores on this node
	int num_cores = omp_get_max_threads();
	
	// Gather core information from all nodes
	int *cores_per_node = NULL;
	if (rank == 0) {
		cores_per_node = (int*)malloc(sizeof(int) * size);
	}
	MPI_Gather(&num_cores, 1, MPI_INT, cores_per_node, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	if (rank == 0) {
		cin >> max_row;
		cin >> max_column;
		cin >> max_n;
		msg[0] = max_row;
		msg[1] = max_column;
		msg[2] = max_n;
		
		cout << "Nós e cores detectados:" << endl;
		int total_cores = 0;
		for (int i = 0; i < size; i++) {
			cout << "Nó " << i << ": " << cores_per_node[i] << " cores" << endl;
			total_cores += cores_per_node[i];
		}
		cout << "Total cores: " << total_cores << endl;
	}

	MPI_Bcast(&msg, 3, MPI_INT, 0, MPI_COMM_WORLD);
	max_row = msg[0];
	max_column = msg[1];
	max_n = msg[2];
	
	// Transmitir informações de cores para todos os processos
	if (cores_per_node == NULL) {
		cores_per_node = (int*)malloc(sizeof(int) * size);
	}
	MPI_Bcast(cores_per_node, size, MPI_INT, 0, MPI_COMM_WORLD);
	
	char **matriz_completa = NULL;

	if (rank == 0) {
		matriz_completa = (char**)malloc(sizeof(char*)*max_row);
		for (int i=0; i<max_row;i++)
			matriz_completa[i]=(char*)malloc(sizeof(char)*max_column);
	}

    // Calcular distribuição ponderada de trabalho baseada na contagem de cores
	int total_cores = 0;
	for (int i = 0; i < size; i++) {
		total_cores += cores_per_node[i];
	}
	
	// Calcular quantas linhas este nó deve processar baseado na sua contagem de cores
	int my_core_weight = cores_per_node[rank];
	int rows_for_this_node = (max_row * my_core_weight) / total_cores;
	
	// Calcular linha inicial para este nó
	int start_row = 0;
	for (int i = 0; i < rank; i++) {
		start_row += (max_row * cores_per_node[i]) / total_cores;
	}
	
	// Garantir que não excedemos max_row devido ao arredondamento
	if (rank == size - 1) {
		rows_for_this_node = max_row - start_row;
	}

	// Calcular a porção da matriz deste nó usando OpenMP
	char **matriz_parcial = (char**)malloc(sizeof(char*)*rows_for_this_node);
	
	// Definir número de threads OpenMP para corresponder aos cores disponíveis
	omp_set_num_threads(num_cores);
	
	// Computação paralela com OpenMP
	#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < rows_for_this_node; i++) {
		char *row = (char*)malloc(sizeof(char)*max_column);
		
		// Loop interno também pode ser paralelizado, mas cuidado com alocação de memória
		for(int c = 0; c < max_column; ++c){
			complex<float> z;
			int n = 0;
			while(abs(z) < 2 && ++n < max_n) {
				z = pow(z, 2) + decltype(z)(
					(float)c * 2 / max_column - 1.5,
					(float)(start_row + i) * 2 / max_row - 1
				);
			}
			row[c]=(n == max_n ? '#' : '.');
		}
		matriz_parcial[i] = row;
	}

	// Coletar resultados de todos os nós
	if (rank != 0) {
		// Enviar número de linhas primeiro
		MPI_Send(&rows_for_this_node, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(&start_row, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
		
		// Enviar cada linha
		for (int i = 0; i < rows_for_this_node; i++) {
			MPI_Send(matriz_parcial[i], max_column, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
		}
	} else {
		// Processo mestre: copiar seus próprios dados primeiro
		for (int i = 0; i < rows_for_this_node; i++) {
			for(int c = 0; c < max_column; ++c){
				matriz_completa[start_row + i][c] = matriz_parcial[i][c];
			}
		}
		
		// Receber dados de outros processos
		for (int par = 1; par < size; par++) {
			int received_rows, received_start_row;
			MPI_Recv(&received_rows, 1, MPI_INT, par, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(&received_start_row, 1, MPI_INT, par, 1, MPI_COMM_WORLD, &status);
			
			for (int i = 0; i < received_rows; i++) {
				MPI_Recv(matriz_completa[received_start_row + i], max_column, MPI_CHAR, par, 2, MPI_COMM_WORLD, &status);
			}
		}

		// Imprimir a matriz completa
		for(int r = 0; r < max_row; ++r){
			for(int c = 0; c < max_column; ++c)
				std::cout << matriz_completa[r][c];
			cout << '\n';
		}
	}
	
	// Limpeza de memória
	free(cores_per_node);
	for (int i = 0; i < rows_for_this_node; i++) {
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


