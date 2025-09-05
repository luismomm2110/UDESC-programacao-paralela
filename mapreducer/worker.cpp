#include "worker.hpp"
#include "task.hpp"
#include "logger.hpp"

const int COORDINATOR = 0;

Worker::Worker(int id, int nReducers)
    : id(id), nReducers(nReducers) {}

int Worker::getId() const { return id; }
int Worker::getNumReducers() const { return nReducers; }

void Worker::run() {
    while (true) {
        Task task = requestTask();
        Logger::logln("Received task ", task.type, " ", " ", task.file, " id: ", task.workerId, " index: ", task.index);

        if (task.type == Task::Type::MAP) {
            std::ifstream file(task.file);
            processMapTask(file, task);
            file.close();
        } else if (task.type == Task::Type::REDUCE) {
            processReduceTask(task);
        }

        if (task.type == Task::Type::NO_TASKS) {
            // dorme 50 ms para evitar busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (task.type == Task::Type::EXIT) {
            Logger::logln("Worker ", id, " received exit message");
            break;
        }
    }
}

void Worker::processMapTask(std::ifstream &file, Task task) {
    std::string line;
    std::unordered_map<std::string, std::vector<std::string>> intermediate;
    std::vector<std::vector<std::string>> reducerData(nReducers);

    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    const int numThreads = omp_get_max_threads();
    std::vector<std::unordered_map<std::string, std::vector<std::string>>> threadIntermediates(numThreads);
    
    #pragma omp parallel for
    for (size_t i = 0; i < lines.size(); ++i) {
        int threadId = omp_get_thread_num();
        std::istringstream iss(lines[i]);
        std::string word;

        while (iss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);

            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());

            if (!word.empty()) {
                threadIntermediates[threadId][word].push_back("1");
            }
        }
    }

    for (const auto& threadMap : threadIntermediates) {
        for (const auto& pair : threadMap) {
            const std::string& word = pair.first;
            const std::vector<std::string>& values = pair.second;
            
            intermediate[word].insert(intermediate[word].end(), values.begin(), values.end());
        }
    }

    std::vector<std::pair<std::string, std::vector<std::string>>> pairs;
    for (const auto& pair : intermediate) {
        pairs.push_back(pair);
    }
    
    std::vector<std::vector<std::vector<std::string>>> threadReducerData(numThreads, std::vector<std::vector<std::string>>(nReducers));
    
    #pragma omp parallel for
    for (size_t i = 0; i < pairs.size(); ++i) {
        int threadId = omp_get_thread_num();
        const std::string& word = pairs[i].first;
        const std::vector<std::string>& values = pairs[i].second;
        size_t hash = std::hash<std::string>{}(word) % nReducers;
        for (const auto& value : values) {
            threadReducerData[threadId][hash].push_back(word + " " + value);
        }
    }

    for (int t = 0; t < numThreads; ++t) {
        for (int r = 0; r < nReducers; ++r) {
            reducerData[r].insert(reducerData[r].end(), threadReducerData[t][r].begin(), threadReducerData[t][r].end());
        }
    }


    #pragma omp parallel for
    for (int i = 0; i < nReducers; i++) {
        std::string bufferFile = "./temp/intermediate-" + std::to_string(task.index) + "-" + std::to_string(i) + ".txt";
        std::ofstream bufferOut(bufferFile);

        for (const auto& data : reducerData[i]) {
            bufferOut << data << "\n";
        }

        bufferOut.close();
    }

    notifyTaskCompleted(task);
}

void Worker::processReduceTask(Task task) {
    std::vector<std::pair<std::string, std::string>> intermediate_data;

    auto files = std::vector<std::string>();
    for (const auto &entry: std::filesystem::directory_iterator("./temp")) {
        auto filename = entry.path().filename().string();
        if (filename.find("intermediate-") != std::string::npos && 
            filename.find("-" + std::to_string(task.index) + ".txt") != std::string::npos) {
            files.push_back(entry.path().string());
        }
    }

    #pragma omp parallel for
    for (size_t i = 0; i < files.size(); ++i) {
        std::ifstream inFile(files[i]);
        std::string line;
        
        #pragma omp critical
        {
            while (std::getline(inFile, line)) {
                std::istringstream iss(line);
                std::string word;
                std::string value;
                iss >> word >> value;
                intermediate_data.push_back({word, value});
            }
        }
    }

    std::sort(intermediate_data.begin(), intermediate_data.end());

    std::unordered_map<std::string, std::vector<std::string>> kv_store;
    
    for (const auto& pair : intermediate_data) {
        const std::string& word = pair.first;
        const std::string& value = pair.second;
        kv_store[word].push_back(value);
    }

    createReduceOutput(task, kv_store);
}

void Worker::createReduceOutput(Task task, std::unordered_map<std::string, std::vector<std::string>> &kv_store) {
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
    int taskTypeValue = static_cast<int>(task.type);
    MPI_Send(&taskTypeValue, 1, MPI_INT, COORDINATOR, 1, MPI_COMM_WORLD);
    MPI_Send(&task.index, 1, MPI_INT, COORDINATOR, 2, MPI_COMM_WORLD);
    MPI_Send(&task.workerId, 1, MPI_INT, COORDINATOR, 3, MPI_COMM_WORLD);
    Logger::logln("Notified task completed:  type value: ", taskTypeValue, "  task: ", task.index, " worker: ", task.workerId);
}

Task Worker::requestTask() {
    MessageType msgType = MessageType::TASK_REQUEST;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    
    MessageType responseType;
    MPI_Recv(&responseType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    if (responseType == MessageType::NO_MORE_TASKS) {
        return Task{
            .status = Task::Status::COMPLETED,
            .type = Task::Type::NO_TASKS,
            .index = -1,
            .file = "",
            .workerId = -1
        };
    }
    
    int status, index, id;
    MPI_Recv(&status, sizeof(int), MPI_INT, COORDINATOR, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&index, sizeof(int), MPI_INT, COORDINATOR, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    int fileNameLength;
    MPI_Recv(&fileNameLength, 1, MPI_INT, COORDINATOR, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    char* fileCharArray = new char[fileNameLength];
    MPI_Recv(fileCharArray, fileNameLength, MPI_CHAR, COORDINATOR, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::string file(fileCharArray);
    delete[] fileCharArray;
    
    MPI_Recv(&id, sizeof(int), MPI_INT, COORDINATOR, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    int taskType;
    MPI_Recv(&taskType, sizeof(int), MPI_INT, COORDINATOR, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return Task{
        .status = static_cast<Task::Status>(status),
        .type = static_cast<Task::Type>(taskType),
        .index = index,
        .file = "./files/" + file,
        .workerId = id
    };
} 