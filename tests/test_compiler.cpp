#include "../src/compiler/compiler.h"
#include "../src/vm/chunk.h"
#include "../src/vm/vm.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace cxxx;

int main() {
    std::string source = "-1.2 + 3.4 * 5";
    Chunk chunk;
    std::cout << "Compiling: " << source << std::endl;

    if (compile(source, &chunk)) {
        std::cout << "Compilation successful." << std::endl;
        chunk.disassemble("Compiled Chunk");

        VM vm;
        vm.init();
        vm.interpret(&chunk);

        Value result = vm.pop();
        std::cout << "Result: " << result.asNumber() << std::endl;
        // -1.2 + (3.4 * 5) = -1.2 + 17 = 15.8
        assert(std::abs(result.asNumber() - 15.8) < 0.0001);
    } else {
        std::cerr << "Compilation failed." << std::endl;
        return 1;
    }
    return 0;
}
