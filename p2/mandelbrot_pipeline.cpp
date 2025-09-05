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
	
	// Inicializar MPI com suporte a threads
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// Obter número de cores disponíveis neste nó
	int num_cores = omp_get_max_threads();
	
	// OTIMIZAÇÃO: Usar operações coletivas em vez de Gather+Bcast
	vector<int> cores_per_node(size);
	MPI_Allgather(&num_cores, 1, MPI_INT, cores_per_node.data(), 1, MPI_INT, MPI_COMM_WORLD);
	
	if (rank == 0) {
		cin >> max_row;
		cin >> max_column;
		cin >> max_n;
		msg[0] = max_row;
		msg[1] = max_column;
		msg[2] = max_n;
	}

	MPI_Bcast(&msg, 3, MPI_INT, 0, MPI_COMM_WORLD);
	max_row = msg[0];
	max_column = msg[1];
	max_n = msg[2];
	
	// Calcular distribuição ponderada
	int total_cores = 0;
	for (int i = 0; i < size; i++) {
		total_cores += cores_per_node[i];
	}
	
	vector<int> rows_per_process(size);
	vector<int> displacements(size);
	
	int offset = 0;
	for (int i = 0; i < size; i++) {
		rows_per_process[i] = (max_row * cores_per_node[i]) / total_cores;
		displacements[i] = offset;
		offset += rows_per_process[i];
	}
	
	// Ajustar último processo
	if (rows_per_process[size-1] + displacements[size-1] < max_row) {
		rows_per_process[size-1] = max_row - displacements[size-1];
	}
	
	int my_rows = rows_per_process[rank];
	int my_start_row = displacements[rank];
	
	// OTIMIZAÇÃO: Pipeline - dividir trabalho em chunks para sobrepor comunicação/computação
	const int CHUNK_SIZE = max(1, my_rows / 4); // 4 chunks por processo
	int num_chunks = (my_rows + CHUNK_SIZE - 1) / CHUNK_SIZE;
	
	// Buffers para pipeline (double buffering)
	vector<char*> chunk_buffers(2);
	chunk_buffers[0] = (char*)malloc(CHUNK_SIZE * max_column * sizeof(char));
	chunk_buffers[1] = (char*)malloc(CHUNK_SIZE * max_column * sizeof(char));
	
	vector<MPI_Request> send_requests(num_chunks);
	
	// Definir número de threads OpenMP
	omp_set_num_threads(num_cores);
	
	if (rank == 0) {
		cout << "Usando pipeline com " << num_chunks << " chunks de ~" << CHUNK_SIZE << " linhas" << endl;
	}
	
	// PIPELINE: Sobrepor computação e comunicação
	for (int chunk = 0; chunk < num_chunks; chunk++) {
		int current_buffer = chunk % 2;
		int chunk_start = chunk * CHUNK_SIZE;
		int chunk_rows = min(CHUNK_SIZE, my_rows - chunk_start);
		
		// FASE 1: Computar chunk atual
		#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < chunk_rows; i++) {
			int global_row = my_start_row + chunk_start + i;
			for(int c = 0; c < max_column; ++c){
				complex<float> z;
				int n = 0;
				while(abs(z) < 2 && ++n < max_n) {
					z = pow(z, 2) + decltype(z)(
						(float)c * 2 / max_column - 1.5,
						(float)global_row * 2 / max_row - 1
					);
				}
				chunk_buffers[current_buffer][i * max_column + c] = (n == max_n ? '#' : '.');
			}
		}
		
		// FASE 2: Enviar chunk não-bloqueante para processo 0
		if (rank != 0) {
			MPI_Isend(chunk_buffers[current_buffer], chunk_rows * max_column, MPI_CHAR,
			         0, chunk, MPI_COMM_WORLD, &send_requests[chunk]);
		}
	}
	
	// Processo mestre recebe e organiza dados
	if (rank == 0) {
		char *global_data = (char*)malloc(max_row * max_column * sizeof(char));
		
		// Copiar dados próprios primeiro
		for (int chunk = 0; chunk < num_chunks; chunk++) {
			int chunk_start = chunk * CHUNK_SIZE;
			int chunk_rows = min(CHUNK_SIZE, my_rows - chunk_start);
			int buffer_idx = chunk % 2;
			
			for (int i = 0; i < chunk_rows; i++) {
				int global_row = my_start_row + chunk_start + i;
				for (int c = 0; c < max_column; c++) {
					global_data[global_row * max_column + c] = 
						chunk_buffers[buffer_idx][i * max_column + c];
				}
			}
		}
		
		// OTIMIZAÇÃO: Receber chunks de outros processos usando MPI_Irecv
		vector<MPI_Request> recv_requests;
		vector<char*> recv_buffers;
		
		for (int proc = 1; proc < size; proc++) {
			int proc_chunks = (rows_per_process[proc] + CHUNK_SIZE - 1) / CHUNK_SIZE;
			
			for (int chunk = 0; chunk < proc_chunks; chunk++) {
				int chunk_start = chunk * CHUNK_SIZE;
				int chunk_rows = min(CHUNK_SIZE, rows_per_process[proc] - chunk_start);
				
				char *recv_buffer = (char*)malloc(chunk_rows * max_column * sizeof(char));
				recv_buffers.push_back(recv_buffer);
				
				MPI_Request req;
				MPI_Irecv(recv_buffer, chunk_rows * max_column, MPI_CHAR,
				         proc, chunk, MPI_COMM_WORLD, &req);
				recv_requests.push_back(req);
			}
		}
		
		// Aguardar todas as recepções e organizar dados
		vector<MPI_Status> statuses(recv_requests.size());
		MPI_Waitall(recv_requests.size(), recv_requests.data(), statuses.data());
		
		// Organizar dados recebidos
		int recv_idx = 0;
		for (int proc = 1; proc < size; proc++) {
			int proc_chunks = (rows_per_process[proc] + CHUNK_SIZE - 1) / CHUNK_SIZE;
			
			for (int chunk = 0; chunk < proc_chunks; chunk++) {
				int chunk_start = chunk * CHUNK_SIZE;
				int chunk_rows = min(CHUNK_SIZE, rows_per_process[proc] - chunk_start);
				
				for (int i = 0; i < chunk_rows; i++) {
					int global_row = displacements[proc] + chunk_start + i;
					for (int c = 0; c < max_column; c++) {
						global_data[global_row * max_column + c] = 
							recv_buffers[recv_idx][i * max_column + c];
					}
				}
				recv_idx++;
			}
		}
		
		// Imprimir resultado
		for(int r = 0; r < max_row; ++r){
			for(int c = 0; c < max_column; ++c) {
				cout << global_data[r * max_column + c];
			}
			cout << '\n';
		}
		
		// Limpeza
		free(global_data);
		for (char* buf : recv_buffers) {
			free(buf);
		}
	} else {
		// Aguardar envios completarem
		vector<MPI_Status> statuses(num_chunks);
		MPI_Waitall(num_chunks, send_requests.data(), statuses.data());
	}
	
	// Limpeza final
	free(chunk_buffers[0]);
	free(chunk_buffers[1]);
	
	MPI_Finalize();
} 