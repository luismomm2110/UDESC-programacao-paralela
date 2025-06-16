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

inline std::ostream &operator<<(std::ostream &os, const Task::Type &type) {
    switch (type) {
        case Task::Type::MAP:
            os << "MAP";
            break;
        case Task::Type::REDUCE:
            os << "REDUCE";
            break;
        case Task::Type::EXIT:
            os << "EXIT";
            break;
        case Task::Type::NO_TASKS:
            os << "NO_TASKS";
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const Task::Status &status);

inline std::ostream &operator<<(std::ostream &os, const MessageType &type) {
    switch (type) {
        case MessageType::TASK_REQUEST:
            os << "TASK_REQUEST";
            break;
        case MessageType::TASK_RESPONSE:
            os << "TASK_RESPONSE";
            break;
        case MessageType::TASK_COMPLETED:
            os << "TASK_COMPLETED";
            break;
        case MessageType::NO_MORE_TASKS:
            os << "NO_MORE_TASKS";
            break;
    }
    return os;
} 