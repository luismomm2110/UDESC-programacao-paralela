#pragma once

#include <iostream>
#include <string>

class Logger {
public:
    static void setLoggingEnabled(bool enabled) {
        loggingEnabled = enabled;
    }
    
    static bool isLoggingEnabled() {
        return loggingEnabled;
    }
    
    template<typename... Args>
    static void log(Args&&... args) {
        if (loggingEnabled) {
            (std::cout << ... << std::forward<Args>(args));
        }
    }
    
    template<typename... Args>
    static void logln(Args&&... args) {
        if (false) {
            (std::cout << ... << std::forward<Args>(args)) << std::endl;
        }
    }

private:
    static bool loggingEnabled;
}; 