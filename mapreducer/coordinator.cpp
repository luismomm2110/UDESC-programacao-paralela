#include "coordinator.hpp"
#include "task.hpp"
#include <iostream>

const int COORDINATOR = 0;

Coordinator::Coordinator(int nReduce, int worldSize) 
    : nReduce(nReduce), worldSize(worldSize) {
    // Inicializa as tarefas
    auto [mapTasks, reduceTasks] = initializeTasks(nReduce);
    this->mapTasks = std::move(mapTasks);
    this->reduceTasks = std::move(reduceTasks);
    
    // Limpa e cria o diretório temporário
    activeWorkers = worldSize - 1;  // Todos os workers exceto o coordenador
    completedTasks = 0;
}

void Coordinator::run() {
    while (activeWorkers > 0) {
        // Aguarda mensagem de qualquer worker
        MessageType msgType;
        MPI_Status status;
        MPI_Recv(&msgType, sizeof(MessageType), MPI_BYTE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        int worker = status.MPI_SOURCE;
        
        if (msgType == MessageType::TASK_REQUEST) {
            handleTaskRequest(worker);
        } else if (msgType == MessageType::TASK_COMPLETED) {
            handleTaskCompleted(worker);
        }
    }
    
    std::cout << "Todas as tarefas completadas. Total: " << completedTasks << std::endl;
}

void Coordinator::handleTaskRequest(int worker) {
    auto task = selectTask(mapTasks, worker);
    if (task.has_value()) {
        sendTaskResponse(task.value(), worker);
    } else {
        sendNoMoreTasks(worker);
        activeWorkers--;
    }
}

void Coordinator::handleTaskCompleted(int worker) {
    int taskId;
    MPI_Recv(&taskId, sizeof(int), MPI_INT, worker, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    completedTasks++;
    
    // Marca a tarefa como completada
    for (auto& task : mapTasks) {
        if (task.id == taskId) {
            task.status = Task::Status::COMPLETED;
            break;
        }
    }
}

std::optional<Task> Coordinator::selectTask(std::vector<Task> &tasks, int worker) {
    for (auto &task: tasks) {
        if (task.status == Task::Status::NOT_ASSIGNED) {
            task.status = Task::Status::PENDING;
            task.id = worker;
            return std::make_optional(task);
        }
    }
    return std::nullopt;
}

void Coordinator::sendTaskResponse(const Task& task, int worker) {
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

void Coordinator::sendNoMoreTasks(int worker) {
    MessageType msgType = MessageType::NO_MORE_TASKS;
    MPI_Send(&msgType, sizeof(MessageType), MPI_BYTE, worker, 0, MPI_COMM_WORLD);
}

std::pair<std::vector<Task>, std::vector<Task>> Coordinator::initializeTasks(int nReduce) {
    std::vector<std::string> files;
    std::string directory = "./files";

    // List files in directory
    for (const auto &entry: std::filesystem::directory_iterator(directory)) {
        files.push_back(entry.path().filename().string());
    }

    int nMap = files.size();  // nMap is now determined by number of input files
    std::vector<Task> mapTasks(nMap);
    std::vector<Task> reduceTasks(nReduce);

    for (int i = 0; i < nMap; i++) {
        mapTasks[i] = Task{
            .status = Task::Status::NOT_ASSIGNED,
            .type = Task::Type::MAP,
            .index = i,
            .file = files[i],  // Use the actual filename
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