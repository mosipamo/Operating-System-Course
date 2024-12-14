#include "logger.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include "colorprint.hpp"

void SetColor(int textColor) {cout << "\033[" << textColor << "m";}

void ResetColor() { cout << "\033[0m"; }

Logger::Logger(std::string program) : program_(std::move(program)) {}

void Logger::error(const std::string& msg) {
    std::cerr << Color::RED << "[ERROR:" << program_ << "]: "
              << Color::RST << msg << '\n';
}

void Logger::warning(const std::string& msg) {
    std::cout << Color::YEL << "[WRNING:" << program_ << "]: "
              << Color::RST << msg << '\n';
}

void Logger::info(const std::string& msg) {
    std::cout << Color::BLU << "[INFO:" << program_ << "]: "
              << Color::RST << msg << '\n';
}

void Logger::perrno() {
    error(strerror(errno));
}
