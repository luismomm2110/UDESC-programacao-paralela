#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <iomanip>

// Estrutura para transmitir pares palavra-contagem via MPI
struct KeyValue {
    char key[128];
    int value;
};

void preprocess_word(std::string& word) {
    // Remove pontuação e converte para minúsculas
    word.erase(std::remove_if(word.begin(), word.end(), 
        [](char c) { return !std::isalnum(c); }), word.end());
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
}

// Função de mapeamento - conta palavras em dados locais
std::map<std::string, int> map_phase(const std::vector<std::string>& lines) {
    std::map<std::string, int> word_count;
    
    for (const auto& line : lines) {
        std::istringstream iss(line);
        std::string word;
        
        while (iss >> word) {
            preprocess_word(word);
            if (!word.empty()) {
                word_count[word]++;
            }
        }
    }
    
    return word_count;
}

// Função de redução usando MPI
std::map<std::string, int> reduce_phase(const std::map<std::string, int>& local_map, 
                                        int rank, int size) {
    // Converter map local para array de KeyValue
    std::vector<KeyValue> local_data;
    for (const auto& pair : local_map) {
        if (pair.first.length() < 128) {  // Verifica tamanho da palavra
            KeyValue kv;
            strcpy(kv.key, pair.first.c_str());
            kv.value = pair.second;
            local_data.push_back(kv);
        }
    }
    
    // Obter tamanhos de todos os processos
    int local_size = local_data.size();
    std::vector<int> all_sizes(size);
    MPI_Allgather(&local_size, 1, MPI_INT, all_sizes.data(), 1, MPI_INT, MPI_COMM_WORLD);
    
    // Calcular deslocamentos
    std::vector<int> displs(size, 0);
    int total_size = all_sizes[0];
    for (int i = 1; i < size; i++) {
        displs[i] = displs[i-1] + all_sizes[i-1];
        total_size += all_sizes[i];
    }
    
    // Coletar todos os dados
    std::vector<KeyValue> all_data(total_size);
    
    // Criar tipo MPI personalizado para KeyValue
    MPI_Datatype mpi_keyvalue;
    int lengths[2] = {128, 1};
    MPI_Aint offsets[2] = {0, 128 * sizeof(char)};
    MPI_Datatype types[2] = {MPI_CHAR, MPI_INT};
    
    MPI_Type_create_struct(2, lengths, offsets, types, &mpi_keyvalue);
    MPI_Type_commit(&mpi_keyvalue);
    
    MPI_Allgatherv(local_data.data(), local_size, mpi_keyvalue,
                   all_data.data(), all_sizes.data(), displs.data(), 
                   mpi_keyvalue, MPI_COMM_WORLD);
    
    MPI_Type_free(&mpi_keyvalue);
    
    // Combinar resultados
    std::map<std::string, int> global_count;
    for (const auto& kv : all_data) {
        global_count[std::string(kv.key)] += kv.value;
    }
    
    return global_count;
}

// Gerar dados de teste baseados no rank do processo
std::vector<std::string> generate_test_data(int rank) {
    std::vector<std::string> data;
    
    switch (rank % 4) {
        case 0:
            data = {
                "O processamento paralelo acelera computações complexas",
                "MPI Message Passing Interface facilita comunicação",
                "Map reduce é um paradigma poderoso para big data"
            };
            break;
        case 1:
            data = {
                "Computação distribuída resolve problemas grandes",
                "Algoritmos paralelos dividem tarefas eficientemente",
                "MPI permite escalabilidade em clusters"
            };
            break;
        case 2:
            data = {
                "Big data requer processamento distribuído",
                "Map reduce divide dados em chunks menores",
                "Paradigma funcional facilita paralelização"
            };
            break;
        case 3:
            data = {
                "Clusters de computadores oferecem alto desempenho",
                "Programação paralela otimiza uso de recursos",
                "MPI é padrão para computação científica"
            };
            break;
    }
    
    return data;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        std::cout << "=== MAP-REDUCE COM MPI ===" << std::endl;
        std::cout << "Processos MPI: " << size << std::endl;
        std::cout << "Iniciando processamento..." << std::endl;
    }
    
    // Sincronizar todos os processos
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Fase MAP: cada processo conta palavras em seus dados
    std::vector<std::string> local_data = generate_test_data(rank);
    std::map<std::string, int> local_counts = map_phase(local_data);
    
    if (rank == 0) {
        std::cout << "\nFase MAP concluída em todos os processos." << std::endl;
    }
    
    // Fase REDUCE: combinar resultados de todos os processos
    MPI_Barrier(MPI_COMM_WORLD);
    std::map<std::string, int> global_counts = reduce_phase(local_counts, rank, size);
    
    // Apenas o processo master imprime os resultados
    if (rank == 0) {
        std::cout << "\nFase REDUCE concluída." << std::endl;
        std::cout << "\n=== RESULTADOS FINAIS ===" << std::endl;
        std::cout << "Palavra                    | Contagem" << std::endl;
        std::cout << "---------------------------|----------" << std::endl;
        
        // Ordenar por contagem (decrescente)
        std::vector<std::pair<std::string, int>> sorted_results(
            global_counts.begin(), global_counts.end());
        std::sort(sorted_results.begin(), sorted_results.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (const auto& pair : sorted_results) {
            std::cout << std::left << std::setw(26) << pair.first 
                      << " | " << pair.second << std::endl;
        }
        
        std::cout << "\nEstatísticas:" << std::endl;
        std::cout << "- Total de palavras únicas: " << global_counts.size() << std::endl;
        
        int total_words = 0;
        for (const auto& pair : global_counts) {
            total_words += pair.second;
        }
        std::cout << "- Total de palavras processadas: " << total_words << std::endl;
    }
    
    MPI_Finalize();
    return 0;
}
