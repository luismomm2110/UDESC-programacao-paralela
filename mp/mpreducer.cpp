#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <queue>
#include <filesystem>
#include <optional>

struct Task;
const int COORDINATOR = 0;
const int BUFFER_SIZE = 1024;  // Tamanho do buffer para cada reducer

int getFileCount(int rank);

std::pair<std::vector<Task>, std::vector<Task> > initializeTasks(int nMap, int nReduce);

Task receiveTask(int worker);

std::optional<Task> selectTask(std::vector<Task> &tasks, int worker);

void sendTaskToWorker(std::vector<Task> &tasks, int worker);

// processMapTask passa a ser método da classe Mapper

struct Task {
    enum class Status {
        NOT_ASSIGNED,
        PENDING,
        IN_PROGRESS,
        COMPLETED,
    };

    enum class Type {
        MAP,
        REDUCE
    };

    Status status;
    Type type;
    int index;
    std::string file;
    int id;
};

std::ostream &operator<<(std::ostream &os, const Task::Status &status) {
    switch (status) {
        case Task::Status::NOT_ASSIGNED:
            os << "NOT_ASSIGNED";
            break;
        case Task::Status::PENDING:
            os << "PENDING";
            break;
        case Task::Status::IN_PROGRESS:
            os << "IN_PROGRESS";
            break;
        case Task::Status::COMPLETED:
            os << "COMPLETED";
            break;
    }
    return os;
}

// Classe Mapper que encapsula informações do mapper
class Mapper {
public:
    Mapper(int id, int nReducers)
        : id(id), nReducers(nReducers) {}

    int getId() const { return id; }
    int getNumReducers() const { return nReducers; }

    void processMapTask(std::ifstream &file, int taskIndex) {
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

private:
    int id;          // Identificador único do mapper
    int nReducers;   // Número de reducers no sistema
};

// Add new message type for task requests
enum class MessageType {
    TASK_REQUEST,
    TASK_RESPONSE,
    TASK_COMPLETED,
    NO_MORE_TASKS
};

void sendTaskResponse(const Task& task, int worker) {
    std::cout << "Sending task response to worker " << worker << std::endl;
    
    // Send message type first
    MessageType msgType = MessageType::TASK_RESPONSE;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, worker, 0, MPI_COMM_WORLD);
    
    // Send task status
    MPI_Send(&task.status, sizeof(int), MPI_INT, worker, 1, MPI_COMM_WORLD);
    MPI_Send(&task.index, sizeof(int), MPI_INT, worker, 2, MPI_COMM_WORLD);
    
    // Send file name
    int fileNameLength = task.file.size() + 1;
    MPI_Send(&fileNameLength, 1, MPI_INT, worker, 3, MPI_COMM_WORLD);
    MPI_Send(task.file.c_str(), fileNameLength, MPI_CHAR, worker, 4, MPI_COMM_WORLD);
    
    MPI_Send(&task.id, sizeof(int), MPI_INT, worker, 5, MPI_COMM_WORLD);
}

void sendNoMoreTasks(int worker) {
    MessageType msgType = MessageType::NO_MORE_TASKS;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, worker, 0, MPI_COMM_WORLD);
}

Task requestTask(int worker) {
    // Send task request to coordinator
    MessageType msgType = MessageType::TASK_REQUEST;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    
    // Receive response type
    MessageType responseType;
    MPI_Recv(&responseType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    if (responseType == MessageType::NO_MORE_TASKS) {
        // Return a dummy task to indicate no more tasks
        return Task{
            .status = Task::Status::COMPLETED,
            .type = Task::Type::MAP,
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
        .type = Task::Type::MAP,
        .index = index,
        .file = "./files/" + file,
        .id = id
    };
}

void notifyTaskCompleted(int worker, int taskId) {
    MessageType msgType = MessageType::TASK_COMPLETED;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    MPI_Send(&taskId, sizeof(int), MPI_INT, COORDINATOR, 1, MPI_COMM_WORLD);
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size, nMap, nReduce;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    nMap = 1;
    nReduce = 2;

    auto [mapTasks, reduceTasks] = initializeTasks(nMap, nReduce);

    if (rank == COORDINATOR) {
        // Clean temp directory before starting
        if (std::filesystem::exists("./temp")) {
            std::filesystem::remove_all("./temp");
        }
        std::filesystem::create_directory("./temp");

        int activeWorkers = size - 1;  // All workers except coordinator
        int completedTasks = 0;
        
        while (activeWorkers > 0) {
            // Wait for any message from workers
            MessageType msgType;
            MPI_Status status;
            MPI_Recv(&msgType, sizeof(MessageType), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            int worker = status.MPI_SOURCE;
            
            if (msgType == MessageType::TASK_REQUEST) {
                auto task = selectTask(mapTasks, worker);
                if (task.has_value()) {
                    sendTaskResponse(task.value(), worker);
                } else {
                    sendNoMoreTasks(worker);
                    activeWorkers--;
                }
            } else if (msgType == MessageType::TASK_COMPLETED) {
                int taskId;
                MPI_Recv(&taskId, sizeof(int), MPI_INT, worker, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                completedTasks++;
                
                // Mark task as completed
                for (auto& task : mapTasks) {
                    if (task.id == taskId) {
                        task.status = Task::Status::COMPLETED;
                        break;
                    }
                }
            }
        }
        
        std::cout << "All tasks completed. Total: " << completedTasks << std::endl;
    } else {
        // Worker process
        Mapper mapper(rank, nReduce);
        
        while (true) {
            Task task = requestTask(rank);
            
            // Check if no more tasks
            if (task.index == -1) {
                std::cout << "Worker " << rank << " received no more tasks signal" << std::endl;
                break;
            }
            
            std::cout << "Worker " << rank << " processing map task " << task.index << std::endl;
            std::ifstream file(task.file);
            mapper.processMapTask(file, task.index);
            file.close();
            
            // Notify coordinator that task is completed
            notifyTaskCompleted(rank, task.id);
        }
    }

    MPI_Finalize();
}

int getFileCount(int rank) {
    int numFiles;

    if (rank == COORDINATOR) {
        std::vector<std::string> files;
        std::string directory = "./files";

        // List files in directory
        for (const auto &entry: std::filesystem::directory_iterator(directory)) {
            files.push_back(entry.path().string());
        }

        numFiles = files.size();
        MPI_Bcast(&numFiles, 1, MPI_INT, COORDINATOR, MPI_COMM_WORLD);
    } else {
        MPI_Bcast(&numFiles, 1, MPI_INT, COORDINATOR, MPI_COMM_WORLD);
    }

    return numFiles;
}

std::pair<std::vector<Task>, std::vector<Task> > initializeTasks(int nMap, int nReduce) {
    std::vector<Task> mapTasks(nMap);
    std::vector<Task> reduceTasks(nReduce);

    std::vector<std::string> files;
    std::string directory = "./files";

    // List files in directory
    for (const auto &entry: std::filesystem::directory_iterator(directory)) {
        files.push_back(entry.path().filename().string());
    }

    for (int i = 0; i < nMap; i++) {
        mapTasks[i] = Task{
            .status = Task::Status::NOT_ASSIGNED,
            .type = Task::Type::MAP,
            .index = i,
            .file = files[i % files.size()],
            .id = i
        };
    }

    for (int i = 0; i < nReduce; i++) {
        reduceTasks[i] = Task{
            .status = Task::Status::NOT_ASSIGNED,
            .type = Task::Type::REDUCE,
            .index = i,
            .file = "",
            .id = i
        };
    }

    return {mapTasks, reduceTasks};
}

std::optional<Task> selectTask(std::vector<Task> &tasks, int worker) {
    for (auto &task: tasks) {
        if (task.status == Task::Status::NOT_ASSIGNED) {
            task.status = Task::Status::PENDING;
            task.id = worker;
            return std::make_optional(task);
        }
    }
    return std::nullopt;
}

void sendTaskToWorker(std::vector<Task> &tasks, int worker) {
    std::cout << "Sending map task to worker " << worker << std::endl;
    auto task = selectTask(tasks, worker);
    if (!task.has_value()) {
        return;
    }

    std::cout << "sent Status: " << task.value().status << std::endl;
    MPI_Send(&task.value().status, sizeof(int), MPI_INT, worker, 0, MPI_COMM_WORLD);
    std::cout << "Index: " << task.value().index << std::endl;
    MPI_Send(&task.value().index, sizeof(int), MPI_INT, worker, 1, MPI_COMM_WORLD);
    std::cout << "File: " << task.value().file << std::endl;
    
    // Send string length first
    int fileNameLength = task.value().file.size() + 1;
    MPI_Send(&fileNameLength, 1, MPI_INT, worker, 2, MPI_COMM_WORLD);
    
    // Then send the string
    const char *fileCharArray = task.value().file.c_str();
    MPI_Send(fileCharArray, fileNameLength, MPI_CHAR, worker, 3, MPI_COMM_WORLD);
    
    std::cout << "Id: " << task.value().id << std::endl;
    MPI_Send(&task.value().id, sizeof(int), MPI_INT, worker, 4, MPI_COMM_WORLD);

    std::string fileLocation = "./files/" + task.value().file;
    std::cout << "File location: " << fileLocation << std::endl;
    
    // Send file location length first
    int fileLocationLength = fileLocation.size() + 1;
    MPI_Send(&fileLocationLength, 1, MPI_INT, worker, 5, MPI_COMM_WORLD);
    
    // Then send the file location
    const char *fileLocationCharArray = fileLocation.c_str();
    MPI_Send(fileLocationCharArray, fileLocationLength, MPI_CHAR, worker, 6, MPI_COMM_WORLD);
    std::cout << "File location sent" << std::endl;
}

Task receiveTask(int worker) {
    std::cout << "Receiving task from coordinator" << std::endl;
    int status;
    MPI_Recv(&status, sizeof(int), MPI_INT, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "received Status: " << status << std::endl;

    int index;
    std::string file;
    std::string fileLocation;
    int id;
    MPI_Recv(&index, sizeof(int), MPI_INT, COORDINATOR, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "received Index: " << index << std::endl;
    
    // Receive string length first
    int fileNameLength;
    MPI_Recv(&fileNameLength, 1, MPI_INT, COORDINATOR, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // Then receive the string based on the length
    char* fileCharArray = new char[fileNameLength];
    MPI_Recv(fileCharArray, fileNameLength, MPI_CHAR, COORDINATOR, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    file = std::string(fileCharArray);
    delete[] fileCharArray;
    std::cout << "received File: " << file << std::endl;
    
    MPI_Recv(&id, sizeof(int), MPI_INT, COORDINATOR, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "received Id: " << id << std::endl;

    // Receive file location length first
    int fileLocationLength;
    MPI_Recv(&fileLocationLength, 1, MPI_INT, COORDINATOR, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    // Then receive the file location based on the length
    char* fileLocationCharArray = new char[fileLocationLength];
    MPI_Recv(fileLocationCharArray, fileLocationLength, MPI_CHAR, COORDINATOR, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fileLocation = std::string(fileLocationCharArray);
    std::cout << "received File location: " << fileLocation << std::endl;
    delete[] fileLocationCharArray;

    return Task{
        .status = static_cast<Task::Status>(status),
        .type = Task::Type::MAP,
        .index = index,
        .file = fileLocation,  // Use the full path instead of just filename
        .id = id
    };
}
