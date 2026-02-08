#include "../src/include/cxxx.h"
#include <iostream>

int main() {
    cxxx::CXXX vm;
    std::cout << "Running: 1 + 2 * 3" << std::endl;
    cxxx::InterpretResult result = vm.interpret("1 + 2 * 3");
    if (result == cxxx::InterpretResult::OK) {
        std::cout << "Success!" << std::endl;
    } else {
        std::cerr << "Failed!" << std::endl;
        return 1;
    }
    return 0;
}
