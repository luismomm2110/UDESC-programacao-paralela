#include "worker.hpp"
#include "task.hpp"

const int COORDINATOR = 0;
const int BUFFER_SIZE = 1024;  // Tamanho do buffer para cada reducer

Worker::Worker(int id, int nReducers)
    : id(id), nReducers(nReducers) {}

int Worker::getId() const { return id; }
int Worker::getNumReducers() const { return nReducers; }

void Worker::run() {
    while (true) {
        Task task = requestTask();

        // recebeu sinal de fim de tarefas
        if (task.type == Task::Type::EXIT) {
            return;
        }

        if (task.type == Task::Type::MAP) {
            std::ifstream file(task.file);
            processMapTask(file, task.index);
            file.close();
        } else if (task.type == Task::Type::REDUCE) {
            processReduceTask(task.index);
        }

        if (task.type == Task::Type::NO_TASKS) {
            // sleep 200 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}

void Worker::processMapTask(std::ifstream &file, int taskIndex) {
    std::string line;
    std::map<std::string, std::vector<std::string>> intermediate;
    std::vector<std::vector<std::string>> buffers(nReducers, std::vector<std::string>(BUFFER_SIZE, ""));
    std::vector<int> bufferIndices(nReducers, 0);

    // Lê o arquivo linha por linha
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string word;

        // Processa cada palavra da linha
        while (iss >> word) {
            // Converte para minúsculas
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);

            // Remove pontuação
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());

            if (!word.empty()) {
                // Adiciona ao mapa intermediário
                intermediate[word].push_back(std::to_string(taskIndex));
            }
        }
    }

    // Distribui as palavras pelos reducers
    for (const auto &pair : intermediate) {
        const std::string &word = pair.first;
        const std::vector<std::string> &values = pair.second;

        size_t hash = std::hash<std::string>{}(word) % nReducers;

        for (const auto &value : values) {
            if (bufferIndices[hash] < BUFFER_SIZE) {
                buffers[hash][bufferIndices[hash]] = word + " " + value;
                bufferIndices[hash]++;
            }
        }
    }

    // Garante existência da pasta temporária
    std::filesystem::create_directory("./temp");

    // Escreve buffers para arquivos específicos de cada reducer
    for (int i = 0; i < nReducers; i++) {
        std::cout << "creating file " << "./temp/intermediate-" + std::to_string(taskIndex) + "-" + std::to_string(i) + ".txt" << std::endl;
        std::string bufferFile = "./temp/intermediate-" + std::to_string(taskIndex) + "-" + std::to_string(i) + ".txt";
        std::ofstream bufferOut(bufferFile);

        for (int j = 0; j < bufferIndices[i]; j++) {
            bufferOut << buffers[i][j] << "\n";
        }

        bufferOut.close();
    }
}

void Worker::processReduceTask(int taskIndex) {
    // cria chave e valor para cada par de palavra
    std::map<std::string, std::vector<std::string>> kv_store;

    auto files = std::vector<std::string>();
    // read all files with intermediate-taskIndex-anything.txt
    for (const auto &entry: std::filesystem::directory_iterator("./temp")) {
        auto filename = entry.path().filename().string();
        if (filename.find("intermediate-" + std::to_string(taskIndex) + "-") != std::string::npos) {
            files.push_back(entry.path().string());
        }
    }

    for (const auto &file: files) {
        std::ifstream inFile(file);
        std::string line;
        while (std::getline(inFile, line)) {
            // split line by space
            std::istringstream iss(line);
            std::string word;
            std::string value;
            iss >> word >> value;
            // adiciona para o kv store, aumentando o valor de cada chave se já existir
            if (kv_store.find(word) != kv_store.end()) {
                kv_store[word].push_back(value);
            } else {
                kv_store[word] = {value};
            }
        }
    }

    createReduceOutput(taskIndex, kv_store);
}

void Worker::createReduceOutput(int taskIndex, std::map<std::string, std::vector<std::string>> &kv_store) {
    std::filesystem::create_directory("./output");
    std::ofstream outputFile("./output/reduce-" + std::to_string(taskIndex) + ".txt");
    for (const auto &pair: kv_store) {
        outputFile << pair.first << " " << pair.second.size() << std::endl;
    }
    outputFile.close();
}

Task Worker::requestTask() {
    // Send task request to coordinator
    MessageType msgType = MessageType::TASK_REQUEST;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    
    // Receive response type
    MessageType responseType;
    MPI_Recv(&responseType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    if (responseType == MessageType::NO_MORE_TASKS) {
        return Task{
            .status = Task::Status::COMPLETED,
            .type = Task::Type::EXIT,
            .index = -1,
            .file = "",
            .id = -1
        };
    }
    
    // Receive task details
    int status, index, id;
    MPI_Recv(&status, sizeof(int), MPI_INT, COORDINATOR, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&index, sizeof(int), MPI_INT, COORDINATOR, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // Receive file name
    int fileNameLength;
    MPI_Recv(&fileNameLength, 1, MPI_INT, COORDINATOR, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    char* fileCharArray = new char[fileNameLength];
    MPI_Recv(fileCharArray, fileNameLength, MPI_CHAR, COORDINATOR, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::string file(fileCharArray);
    delete[] fileCharArray;
    
    MPI_Recv(&id, sizeof(int), MPI_INT, COORDINATOR, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    return Task{
        .status = static_cast<Task::Status>(status),
        .type = static_cast<Task::Type>(index >= 0 ? Task::Type::MAP : Task::Type::NO_TASKS),
        .index = index,
        .file = "./files/" + file,
        .id = id
    };
} 