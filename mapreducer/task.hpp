#pragma once

#include <string>

struct Task {
    enum class Status {
        NOT_ASSIGNED,
        PENDING,
        IN_PROGRESS,
        COMPLETED,
    };

    enum class Type {
        MAP,
        REDUCE,
        EXIT,
        NO_TASKS
    };

    Status status;
    Type type;
    int index;
    std::string file;
    int id;
};

enum class MessageType {
    TASK_REQUEST,
    TASK_RESPONSE,
    TASK_COMPLETED,
    NO_MORE_TASKS
};

std::ostream &operator<<(std::ostream &os, const Task::Status &status); 