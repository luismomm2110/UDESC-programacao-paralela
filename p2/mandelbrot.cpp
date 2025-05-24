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
	// todo calcular a sobra
	int fatia = max_row / size;

	// calcular sua parte da matriz
	char **matriz_parcial = (char**)malloc(sizeof(char*)*fatia);
	for (int i = 0; i < fatia; i++) {
		// será responsável por alocar a memória para cada linha
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
			// armazena a linha na matriz parcial
			matriz_parcial[i] = row;
		}
	}
	// se não for o processo 0, envie a matriz parcial
	if (rank != 0) {
		for (int i = 0; i < fatia; i++) {
			MPI_Send(matriz_parcial[i], fatia*max_column, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
		}
	} else {
		// mestre
		//  não precisa receber a sua propria parte
		for (int par = 1; par < size; par++) {
			// recebe a matriz parcial de cada processo
			int linha_inicial = par * fatia;
			for (int linha_do_par = 0; linha_do_par < fatia; linha_do_par++) {
				MPI_Recv(matriz_completa[linha_inicial+linha_do_par], fatia*max_column, MPI_CHAR, par, 0, MPI_COMM_WORLD, &status);
			}
		}
		// copia a matriz parcial do mestre para a matriz completa

		for (int i = 0; i < fatia; i++) {
			for(int c = 0; c < max_column; ++c){
				matriz_completa[i][c] = matriz_parcial[i][c];
			}
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0) {
		// mestre imprime
		cout << "max row: " << max_row << "\n";
		for(int r = 0; r < max_row; ++r){
			cout << "Linha " << r << ": ";
			for(int c = 0; c < max_column; ++c)
				std::cout << matriz_completa[r][c];
			cout << '\n';
		}
	}
	MPI_Finalize();
}


