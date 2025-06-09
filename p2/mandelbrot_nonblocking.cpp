#include <mpi.h>
#include <complex>
#include <iostream>

using namespace std;

int main(int argc, char **argv){
	int max_row, max_column, max_n, rank, size;
	int msg[3];
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 	MPI_Comm_size(MPI_COMM_WORLD, &size);
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
	char **matriz_completa = NULL;

	if (rank == 0) {
		matriz_completa = (char**)malloc(sizeof(char*)*max_row);
		for (int i=0; i<max_row;i++)
			matriz_completa[i]=(char*)malloc(sizeof(char)*max_column);
	}

    // quantidade de linhas que cada processo vai processar
	int fatia = max_row / size;

	// calcular sua parte da matriz (mantém estrutura original)
	char **matriz_parcial = (char**)malloc(sizeof(char*)*fatia);
	for (int i = 0; i < fatia; i++) {
		char *row = (char*)malloc(sizeof(char)*max_column);
		for(int c = 0; c < max_column; ++c){
			complex<float> z;
			int n = 0;
			while(abs(z) < 2 && ++n < max_n) {
				z = pow(z, 2) + decltype(z)(
					(float)c * 2 / max_column - 1.5,
					(float)(i+fatia*rank) * 2 / max_row - 1
				);
			}
			row[c]=(n == max_n ? '#' : '.');
		}
		matriz_parcial[i] = row;
	}

	// usar comunicação não-bloqueante para enviar todas as linhas simultaneamente
	if (rank != 0) {
		MPI_Request *requests = (MPI_Request*)malloc(fatia * sizeof(MPI_Request));
		// inicia o envio de todas as linhas ao mesmo tempo
		for (int i = 0; i < fatia; i++) {
			MPI_Isend(matriz_parcial[i], max_column, MPI_CHAR, 0, i, MPI_COMM_WORLD, &requests[i]);
		}
		// espera todos os envios terminarem
		MPI_Waitall(fatia, requests, MPI_STATUSES_IGNORE);
		free(requests);
	} else {
		// mestre recebe de todos os processos simultaneamente
		for (int par = 1; par < size; par++) {
			MPI_Request *requests = (MPI_Request*)malloc(fatia * sizeof(MPI_Request));
			int linha_inicial = par * fatia;
			
			// inicia o recebimento de todas as linhas do processo par
			for (int i = 0; i < fatia; i++) {
				MPI_Irecv(matriz_completa[linha_inicial + i], max_column, MPI_CHAR, par, i, MPI_COMM_WORLD, &requests[i]);
			}
			// espera todos os recebimentos terminarem
			MPI_Waitall(fatia, requests, MPI_STATUSES_IGNORE);
			free(requests);
		}
		
		// copia a matriz parcial do mestre para a matriz completa
		for (int i = 0; i < fatia; i++) {
			for(int c = 0; c < max_column; ++c){
				matriz_completa[i][c] = matriz_parcial[i][c];
			}
		}

		// imprime a matriz completa
		for(int r = 0; r < max_row; ++r){
			for(int c = 0; c < max_column; ++c)
				std::cout << matriz_completa[r][c];
			cout << '\n';
		}
		
		// libera memória da matriz completa
		for (int i = 0; i < max_row; i++) {
			free(matriz_completa[i]);
		}
		free(matriz_completa);
	}

	// libera memória da matriz parcial
	for (int i = 0; i < fatia; i++) {
		free(matriz_parcial[i]);
	}
	free(matriz_parcial);

	MPI_Finalize();
} 