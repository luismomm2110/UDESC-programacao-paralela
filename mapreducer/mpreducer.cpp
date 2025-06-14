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
#include "worker.hpp"
#include "coordinator.hpp"
#include "task.hpp"

const int COORDINATOR = 0;
const int BUFFER_SIZE = 1024;  // Tamanho do buffer para cada reducer


void sendTaskResponse(const Task& task, int worker);
void sendNoMoreTasks(int worker);
void notifyTaskCompleted(int worker, int taskId);

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


void notifyTaskCompleted(int worker, int taskId) {
    MessageType msgType = MessageType::TASK_COMPLETED;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
    MPI_Send(&taskId, sizeof(int), MPI_INT, COORDINATOR, 1, MPI_COMM_WORLD);
}

int main(int argc, char **argv) {
    if (std::filesystem::exists("./temp")) {
        std::filesystem::remove_all("./temp");
    }
    std::filesystem::create_directory("./temp");
    
    MPI_Init(&argc, &argv);

    int rank, size, nReduce;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    nReduce = 2;

    if (rank == COORDINATOR) {
        auto coordinator = Coordinator(nReduce, size);
        coordinator.run();
    } else {
        // Worker process
        Worker worker(rank, nReduce);
        worker.run();
    }

    MPI_Finalize();
    return 0;
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
