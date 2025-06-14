#pragma once

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>
#include "task.hpp"

class Worker {
public:
    Worker(int id, int nReducers);
    int getId() const;
    int getNumReducers() const;
    void run();
    void processMapTask(std::ifstream &file, int taskIndex);
    void processReduceTask(int taskIndex);
    Task requestTask();

private:
    void createReduceOutput(int taskIndex, std::map<std::string, std::vector<std::string>> &kv_store);
    int id;          // Identificador único do worker
    int nReducers;   // Número de reducers no sistema
}; 