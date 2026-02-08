#include "../include/cxxx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

void repl(cxxx::CXXX& vm) {
    std::string line;
    std::cout << "> ";
    while (std::getline(std::cin, line)) {
        cxxx::InterpretResult result = vm.interpret(line);
        if (result == cxxx::InterpretResult::OK) {
            std::cout << vm.getResult() << std::endl;
        }
        std::cout << "> ";
    }
}

std::string readFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file \"" << path << "\"." << std::endl;
        exit(74);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void runFile(cxxx::CXXX& vm, const char* path) {
    std::string source = readFile(path);
    cxxx::InterpretResult result = vm.interpret(source);
    if (result == cxxx::InterpretResult::COMPILE_ERROR) exit(65);
    if (result == cxxx::InterpretResult::RUNTIME_ERROR) exit(70);
}

int main(int argc, char* argv[]) {
    cxxx::CXXX vm;

    if (argc == 1) {
        repl(vm);
    } else if (argc == 2) {
        runFile(vm, argv[1]);
    } else {
        std::cerr << "Usage: cxxx [path]" << std::endl;
        exit(64);
    }

    return 0;
}
