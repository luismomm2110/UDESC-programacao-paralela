#pragma once

#include <mpi.h>
#include <vector>
#include <filesystem>
#include "task.hpp"
#include <optional>

class Coordinator {
public:
    Coordinator(int nReduce, int worldSize);
    void run();

private:
    void handleTaskRequest(int worker);
    void handleTaskCompleted(int worker);
    std::optional<Task> selectTask(std::vector<Task> &tasks, int worker);
    void sendTaskResponse(const Task& task, int worker);
    void sendNoMoreTasks(int worker);
    std::pair<std::vector<Task>, std::vector<Task> > initializeTasks(int nReduce);

    int nReduce;
    int worldSize;
    std::vector<Task> mapTasks;
    std::vector<Task> reduceTasks;
    int activeWorkers;
    int completedTasks;
}; 