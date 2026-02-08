#include "../src/compiler/compiler.h"
#include "../src/vm/chunk.h"
#include "../src/vm/vm.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace cxxx;

#include "../src/vm/table.h"
#include "../src/vm/object.h"

int main() {
    std::string source = "var result = -1.2 + 3.4 * 5;";
    std::cout << "Compiling: " << source << std::endl;

    // We need to pass a table for interning if we want consistency,
    // but for single chunk test, nullptr is okay for compilation.
    // However, VM needs globals table.

    // To make sure VM uses same strings, we should share table?
    // Actually, VM has its own strings table.
    // If we pass nullptr to compile, it allocates new strings.
    // When VM runs, it uses those strings as keys.
    // VM globals table will store them.
    // Lookup by string content requires interning or deep compare.
    // Table uses pointer compare.

    // So we MUST use a string table shared with VM.

    VM vm;
    vm.init();

    std::cout << "vm address: " << &vm << std::endl;
    std::cout << "vm.strings address: " << &vm.strings << std::endl;
    std::cout << "vm.globals address: " << &vm.globals << std::endl;

    ObjFunction* function = compile(source, &vm.strings);
    if (function != nullptr) {
        std::cout << "Compilation successful." << std::endl;
        function->chunk.disassemble("Compiled Chunk");

        vm.interpret(function);

        // Check global 'result'
        ObjString* name = copyString("result", 6, &vm.strings);
        std::cout << "Checking global 'result'. Address of name: " << name << " hash: " << name->hash << std::endl;
        Value val;
        if (vm.globals.get(name, &val)) {
             std::cout << "Result: " << val.asNumber() << std::endl;
             assert(std::abs(val.asNumber() - 15.8) < 0.0001);
        } else {
             std::cerr << "Global 'result' not found." << std::endl;
             // Debug dump globals
             std::cout << "Globals count: " << vm.globals.count << std::endl;
             printTable(&vm.globals);
             return 1;
        }

    } else {
        std::cerr << "Compilation failed." << std::endl;
        return 1;
    }
    return 0;
}
