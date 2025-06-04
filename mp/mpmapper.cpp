#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <fstream>

struct WordCount {
    char word[64];
    int count;
};

class MapReducer {
private:
    int rank;
    int size;
    
public:
    MapReducer() {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    
    // Função de mapeamento - processa dados locais
    std::map<std::string, int> map_function(const std::vector<std::string>& input_data) {
        std::map<std::string, int> local_counts;
        
        for (const auto& line : input_data) {
            std::istringstream iss(line);
            std::string word;
            
            while (iss >> word) {
                // Remove pontuação e converte para minúsculas
                word.erase(std::remove_if(word.begin(), word.end(), 
                    [](char c) { return !std::isalnum(c); }), word.end());
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                
                if (!word.empty()) {
                    local_counts[word]++;
                }
            }
        }
        
        return local_counts;
    }
    
    // Função de redução - combina resultados de todos os processos
    std::map<std::string, int> reduce_function(const std::map<std::string, int>& local_counts) {
        std::vector<WordCount> local_data;
        
        // Converte map local para formato serializável
        for (const auto& pair : local_counts) {
            WordCount wc;
            strncpy(wc.word, pair.first.c_str(), sizeof(wc.word) - 1);
            wc.word[sizeof(wc.word) - 1] = '\0';
            wc.count = pair.second;
            local_data.push_back(wc);
        }
        
        // Coleta tamanhos de cada processo
        int local_size = local_data.size();
        std::vector<int> sizes(size);
        MPI_Allgather(&local_size, 1, MPI_INT, sizes.data(), 1, MPI_INT, MPI_COMM_WORLD);
        
        // Calcula deslocamentos para Allgatherv
        std::vector<int> displs(size);
        int total_size = 0;
        for (int i = 0; i < size; i++) {
            displs[i] = total_size;
            total_size += sizes[i];
        }
        
        // Coleta todos os dados
        std::vector<WordCount> all_data(total_size);
        MPI_Allgatherv(local_data.data(), local_size, 
                      MPI_2INT, // Assumindo que WordCount é equivalente a 2 ints
                      all_data.data(), sizes.data(), displs.data(), 
                      MPI_2INT, MPI_COMM_WORLD);
        
        // Combina os resultados
        std::map<std::string, int> global_counts;
        for (const auto& wc : all_data) {
            global_counts[std::string(wc.word)] += wc.count;
        }
        
        return global_counts;
    }
    
    // Simula dados de entrada distribuídos
    std::vector<std::string> generate_input_data() {
        std::vector<std::string> data;
        
        // Dados de exemplo diferentes para cada processo
        if (rank == 0) {
            data = {
                "Hello world this is a test",
                "Map reduce with MPI is powerful",
                "Parallel processing makes computation faster"
            };
        } else if (rank == 1) {
            data = {
                "This is another test for map reduce",
                "MPI enables distributed computing",
                "Hello from process one"
            };
        } else if (rank == 2) {
            data = {
                "Map reduce paradigm is useful",
                "Distributed systems are complex",
                "Hello world from process two"
            };
        } else {
            data = {
                "Additional data from other processes",
                "Scaling with more MPI processes",
                "Hello from process " + std::to_string(rank)
            };
        }
        
        return data;
    }
    
    void run() {
        if (rank == 0) {
            std::cout << "Iniciando Map-Reduce com " << size << " processos MPI\n";
        }
        
        // Fase de Map
        auto input_data = generate_input_data();
        auto local_counts = map_function(input_data);
        
        if (rank == 0) {
            std::cout << "\nFase de Mapeamento concluída. Iniciando redução...\n";
        }
        
        // Fase de Reduce
        auto global_counts = reduce_function(local_counts);
        
        // Apenas o processo 0 imprime os resultados finais
        if (rank == 0) {
            std::cout << "\nResultados finais do Map-Reduce:\n";
            std::cout << "Palavra\t\tContagem\n";
            std::cout << "========================\n";
            
            for (const auto& pair : global_counts) {
                std::cout << pair.first << "\t\t" << pair.second << std::endl;
            }
            
            std::cout << "\nTotal de palavras únicas: " << global_counts.size() << std::endl;
        }
    }
};

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    // Registra tipo customizado para WordCount
    MPI_Datatype MPI_WORDCOUNT;
    int blocklengths[2] = {64, 1};
    MPI_Aint displacements[2] = {0, sizeof(char) * 64};
    MPI_Datatype types[2] = {MPI_CHAR, MPI_INT};
    
    MPI_Type_create_struct(2, blocklengths, displacements, types, &MPI_WORDCOUNT);
    MPI_Type_commit(&MPI_WORDCOUNT);
    
    MapReducer mr;
    mr.run();
    
    MPI_Type_free(&MPI_WORDCOUNT);
    MPI_Finalize();
    
    return 0;
}
