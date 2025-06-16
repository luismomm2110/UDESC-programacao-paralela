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
        std::cout << "Received task " << task.type << " " << " " << task.file << " id: " << task.workerId << " index: " << task.index << std::endl; 


        if (task.type == Task::Type::MAP) {
            std::ifstream file(task.file);
            processMapTask(file, task);
            file.close();
        } else if (task.type == Task::Type::REDUCE) {
            processReduceTask(task);
        }

        if (task.type == Task::Type::NO_TASKS) {
            // sleep 200 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }
}

void Worker::processMapTask(std::ifstream &file, Task task) {
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
                intermediate[word].push_back(std::to_string(task.index));
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
        std::string bufferFile = "./temp/intermediate-" + std::to_string(task.index) + "-" + std::to_string(i) + ".txt";
        std::ofstream bufferOut(bufferFile);

        for (int j = 0; j < bufferIndices[i]; j++) {
            bufferOut << buffers[i][j] << "\n";
        }

        bufferOut.close();
    }

    notifyTaskCompleted(task);
}

void Worker::processReduceTask(Task task) {
    // cria chave e valor para cada par de palavra
    std::map<std::string, std::vector<std::string>> kv_store;

    auto files = std::vector<std::string>();
    // read all files with intermediate-taskIndex-anything.txt
    for (const auto &entry: std::filesystem::directory_iterator("./temp")) {
        auto filename = entry.path().filename().string();
        if (filename.find("intermediate-") != std::string::npos && 
            filename.find("-" + std::to_string(task.workerId) + ".txt") != std::string::npos) {
            files.push_back(entry.path().string());
        }
    }
    std::cout << "Files: " << files.size() << std::endl;

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

    createReduceOutput(task, kv_store);
}

void Worker::createReduceOutput(Task task, std::map<std::string, std::vector<std::string>> &kv_store) {
    std::filesystem::create_directory("./output");
    std::ofstream outputFile("./output/reduce-" + std::to_string(task.index) + ".txt");
    for (const auto &pair: kv_store) {
        outputFile << pair.first << " " << pair.second.size() << std::endl;
    }
    outputFile.close();

    notifyTaskCompleted(task);
}

void Worker::notifyTaskCompleted(Task task) {
    MessageType msgType = MessageType::TASK_COMPLETED;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    MPI_Send(&task.type, sizeof(Task::Type), MPI_BYTE, COORDINATOR, 1, MPI_COMM_WORLD);
    MPI_Send(&task.index, sizeof(int), MPI_INT, COORDINATOR, 2, MPI_COMM_WORLD);
    MPI_Send(&task.workerId, sizeof(int), MPI_INT, COORDINATOR, 3, MPI_COMM_WORLD);
    std::cout << "Notified task completed:  type: " << task.type << "  task: " << task.index << " worker: " << task.workerId << std::endl;
}

Task Worker::requestTask() {
    // Send task request to coordinator
    MessageType msgType = MessageType::TASK_REQUEST;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    
    // Receive response type
    MessageType responseType;
    MPI_Recv(&responseType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    std::cout << "Received task request" << std::endl;
    std::cout << "Response type: " << responseType << std::endl;
    
    if (responseType == MessageType::NO_MORE_TASKS) {
        return Task{
            .status = Task::Status::COMPLETED,
            .type = Task::Type::NO_TASKS,
            .index = -1,
            .file = "",
            .workerId = -1
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
    int taskType;
    MPI_Recv(&taskType, sizeof(int), MPI_INT, COORDINATOR, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "Received task type: " << taskType << std::endl;
    return Task{
        .status = static_cast<Task::Status>(status),
        .type = static_cast<Task::Type>(taskType),
        .index = index,
        .file = "./files/" + file,
        .workerId = id
    };
} 