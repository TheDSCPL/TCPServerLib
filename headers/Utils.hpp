#pragma once

#include <string>
#include "Thread.hpp"

class Timer {
    struct timespec startTime={0},endTime={0};
public:
    void start();
    long int getTime();
    void end();
    bool isRunning();
};

extern Mutex coutMutex;

namespace Utils {

    bool s2b(std::string const&);

    double tenPower(const long);

    // Converts a string to double.
    // Throws logic_error if the string isn't a double
    // Only use dots (".") for separating integer part from decimal part.
    double stod(const std::string &);

    int stoi(const std::string &s);

    // If the string is empty, returns true. Can consider negative numbers
    bool isInt(const std::string&);

    // If the string is empty, returns true. Can consider negative numbers.
    // Is true even if there is no decimal part.
    // Only use dots (".") for separating integer part from decimal part.
    bool isDouble(const std::string&);
}

std::string operator+(const std::string &s, int i);

std::string operator+(int i, const std::string &s);