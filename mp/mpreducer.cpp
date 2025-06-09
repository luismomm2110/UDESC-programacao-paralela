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

const int COORDINATOR = 0;

int getFileCount(int rank);
std::pair<std::vector<Task>, std::vector<Task>> initializeTasks(int nMap, int nReduce);

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


int main(int argc, char** argv) {
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
            Task task = selectTask(mapTasks, worker);
            MPI_Send(&task, sizeof(Task), MPI_BYTE, worker, 0, MPI_COMM_WORLD);
            nMap--;
        }
   
        while (nReduce > 0) {
            int worker = idleWorkers.front();
            idleWorkers.pop();
            Task task = selectTask(reduceTasks, worker);   
            MPI_Send(&task, sizeof(Task), MPI_BYTE, worker, 0, MPI_COMM_WORLD);
            nReduce--;
        }   
    }
    else {
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
            }
            else {
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
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            files.push_back(entry.path().string());
        }
        
        numFiles = files.size();
        MPI_Bcast(&numFiles, 1, MPI_INT, COORDINATOR, MPI_COMM_WORLD);
    } else {
        MPI_Bcast(&numFiles, 1, MPI_INT, COORDINATOR, MPI_COMM_WORLD);
    }
    
    return numFiles;
}

std::pair<std::vector<Task>, std::vector<Task>> initializeTasks(int nMap, int nReduce) {
    std::vector<Task> mapTasks(nMap);
    std::vector<Task> reduceTasks(nReduce);

    for (int i = 0; i < nMap; i++) {
        mapTasks[i] = Task{
            .status = Task::Status::NOT_ASSIGNED,
            .type = Task::Type::MAP,
            .index = i,
            .file = "",
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

Task selectTask(std::vector<Task>& tasks, int worker) {
    for (auto& task : tasks) {
        if (task.status == Task::Status::NOT_ASSIGNED) {
            task.status = Task::Status::PENDING;
            task.id = worker;
            return task;
        }
    }
    throw std::runtime_error("No task found");
}

void sendTask(Task& task, int worker) {
    int status = task.status;
    MPI_Send(&status, sizeof(int), MPI_INT, worker, 0, MPI_COMM_WORLD);
    //index 
    MPI_Send(&task.index, sizeof(int), MPI_INT, worker, 1, MPI_COMM_WORLD);
    //file
    MPI_Send(&task.file, sizeof(std::string), MPI_CHAR, worker, 2, MPI_COMM_WORLD);
    //id
    MPI_Send(&task.id, sizeof(int), MPI_INT, worker, 3, MPI_COMM_WORLD);

    // send file location 
    std::string fileLocation = "./files/" + task.file;
    MPI_Send(&fileLocation, sizeof(std::string), MPI_CHAR, worker, 4, MPI_COMM_WORLD);
}

Task receiveTask(int worker) {
    int status;
    MPI_Recv(&status, sizeof(int), MPI_INT, worker, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    MPI_Recv(index, sizeof(int), MPI_INT, worker, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(file, sizeof(std::string), MPI_CHAR, worker, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(id, sizeof(int), MPI_INT, worker, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    std::string fileLocation;
    MPI_Recv(&fileLocation, sizeof(std::string), MPI_CHAR, worker, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    file = fileLocation.substr(fileLocation.find_last_of('/') + 1);

    return Task{
        .status = static_cast<Task::Status>(status),
        .index = index,

        .file = file,
        .id = id
    };
}