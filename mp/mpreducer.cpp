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

int getFileCount(int rank);

std::pair<std::vector<Task>, std::vector<Task> > initializeTasks(int nMap, int nReduce);

Task receiveTask(int worker);

std::optional<Task> selectTask(std::vector<Task> &tasks, int worker);

void sendTaskToWorker(std::vector<Task> &tasks, int worker);

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

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size, nMap, nReduce;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::queue<int> idleWorkers;
    for (int i = 1; i < size; i++) {
        idleWorkers.push(i);
    }


    // todo mudar o numero de reducers e mappers para pegar do argumento
    nMap = getFileCount(rank);
    nReduce = 2;

    auto [mapTasks, reduceTasks] = initializeTasks(nMap, nReduce);

    if (rank == COORDINATOR) {
        while (nMap > 0) {
            int worker = idleWorkers.front();
            idleWorkers.pop();
            if (idleWorkers.empty()) {
                break;
            }
            sendTaskToWorker(mapTasks, worker);
            nMap--;
        }

        // while (nReduce > 0) {
        //     int worker = idleWorkers.front();
        //     idleWorkers.pop();
        //     if (idleWorkers.empty()) {
        //         break;
        //     }
        //     sendTaskToWorker(reduceTasks, worker);
        //     nReduce--;
        // }
    } else {
        while (true) {
            Task task = receiveTask(rank);
            if (task.type == Task::Type::MAP) {
                // map task
                std::ifstream file(task.file);
                std::string line;
                std::cout << "Map task " << task.index << " received" << std::endl;
                while (std::getline(file, line)) {
                    std::cout << line << std::endl;
                }
                file.close();
                idleWorkers.push(rank);
            } else {
                // reduce task 
            }
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
