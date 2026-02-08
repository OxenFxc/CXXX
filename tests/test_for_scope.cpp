#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;
    std::cout << "Running testForLoopLeak..." << std::endl;
    // Inside a block, redeclaration is an error.
    // If 'i' leaks from 'for', it's in the block scope.
    // So 'var i = 1' is a redeclaration in same scope -> Error.
    vm.interpret("{ for (var i = 0; i < 1; i = i + 1) {} var i = 1; }");
    return 0;
}
