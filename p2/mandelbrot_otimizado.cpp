#include <mpi.h>
#include <omp.h>
#include <complex>
#include <iostream>
#include <cstdlib>
#include <vector>

using namespace std;

int main(int argc, char **argv){
	int max_row, max_column, max_n, rank, size;
	int msg[3];
	MPI_Status status;
	
	// Inicializar MPI com suporte a threads
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// Obter número de cores disponíveis neste nó
	int num_cores = omp_get_max_threads();
	
	// Coletar informações de cores de todos os nós usando operação coletiva
	vector<int> cores_per_node(size);
	MPI_Allgather(&num_cores, 1, MPI_INT, cores_per_node.data(), 1, MPI_INT, MPI_COMM_WORLD);
	
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
		cout << "Total de cores: " << total_cores << endl;
	}

	// Broadcast usando operação coletiva
	MPI_Bcast(&msg, 3, MPI_INT, 0, MPI_COMM_WORLD);
	max_row = msg[0];
	max_column = msg[1];
	max_n = msg[2];
	
	// Calcular distribuição ponderada de trabalho baseada na contagem de cores
	int total_cores = 0;
	for (int i = 0; i < size; i++) {
		total_cores += cores_per_node[i];
	}
	
	// Calcular quantas linhas cada processo deve processar e offsets
	vector<int> rows_per_process(size);
	vector<int> displacements(size);
	
	int offset = 0;
	for (int i = 0; i < size; i++) {
		rows_per_process[i] = (max_row * cores_per_node[i]) / total_cores;
		displacements[i] = offset;
		offset += rows_per_process[i];
	}
	
	// Ajustar último processo para garantir que todas as linhas sejam processadas
	if (rows_per_process[size-1] + displacements[size-1] < max_row) {
		rows_per_process[size-1] = max_row - displacements[size-1];
	}
	
	// Quantas linhas este processo vai processar
	int my_rows = rows_per_process[rank];
	int my_start_row = displacements[rank];
	
	// Alocar buffer contíguo para dados locais (melhor para MPI)
	char *local_data = (char*)malloc(my_rows * max_column * sizeof(char));
	
	// Definir número de threads OpenMP para corresponder aos cores disponíveis
	omp_set_num_threads(num_cores);
	
	// Computação paralela com OpenMP - buffer contíguo
	#pragma omp parallel for schedule(dynamic) collapse(1)
	for (int i = 0; i < my_rows; i++) {
		for(int c = 0; c < max_column; ++c){
			complex<float> z;
			int n = 0;
			while(abs(z) < 2 && ++n < max_n) {
				z = pow(z, 2) + decltype(z)(
					(float)c * 2 / max_column - 1.5,
					(float)(my_start_row + i) * 2 / max_row - 1
				);
			}
			// Armazenar em buffer contíguo
			local_data[i * max_column + c] = (n == max_n ? '#' : '.');
		}
	}
	
	// OTIMIZAÇÃO 1: Usar MPI_Gatherv para comunicação coletiva eficiente
	char *global_data = NULL;
	vector<int> recv_counts(size);
	vector<int> recv_displacements(size);
	
	// Preparar parâmetros para Gatherv
	for (int i = 0; i < size; i++) {
		recv_counts[i] = rows_per_process[i] * max_column;
		recv_displacements[i] = displacements[i] * max_column;
	}
	
	if (rank == 0) {
		global_data = (char*)malloc(max_row * max_column * sizeof(char));
	}
	
	// Coletar todos os dados de uma vez usando operação coletiva
	MPI_Gatherv(local_data, my_rows * max_column, MPI_CHAR,
	           global_data, recv_counts.data(), recv_displacements.data(), MPI_CHAR,
	           0, MPI_COMM_WORLD);
	
	// Processo mestre imprime o resultado
	if (rank == 0) {
		for(int r = 0; r < max_row; ++r){
			for(int c = 0; c < max_column; ++c) {
				cout << global_data[r * max_column + c];
			}
			cout << '\n';
		}
		free(global_data);
	}
	
	// Limpeza de memória
	free(local_data);
	
	MPI_Finalize();
} 