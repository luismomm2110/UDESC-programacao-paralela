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

    int rank, size, nReduce;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Calculate nReduce based on number of workers to ensure good load balancing
    // Use number of workers (size - 1) as the number of reduce tasks
    nReduce = size - 1;

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
