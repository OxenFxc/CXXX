#include "../src/vm/vm.h"
#include "../src/vm/chunk.h"
#include "../src/vm/value.h"
#include "../src/vm/object.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace cxxx;

void testStackOverflow() {
    std::cout << "Testing Stack Overflow..." << std::endl;
    VM vm;
    vm.init();

    // Fill stack to max
    int i = 0;
    bool overflowDetected = false;

    // We assume VM::push returns bool. If not updated yet, this won't compile.
    for (; i < STACK_MAX + 10; i++) {
        if (!vm.push(NUMBER_VAL(i))) {
            overflowDetected = true;
            break;
        }
    }

    if (overflowDetected) {
        std::cout << "Stack overflow correctly detected at " << i << "." << std::endl;
    } else {
        std::cout << "Stack overflow NOT detected!" << std::endl;
        exit(1);
    }
}

void testDivisionByZero() {
    std::cout << "Testing Division by Zero..." << std::endl;
    VM vm;
    vm.init();

    ObjFunction* function = allocateFunction(&vm);
    Chunk* chunk = &function->chunk;

    // 1 / 0
    int constant1 = chunk->addConstant(NUMBER_VAL(1));
    int constant0 = chunk->addConstant(NUMBER_VAL(0));

    chunk->write(OP_CONSTANT, 1);
    chunk->write(constant1, 1);
    chunk->write(OP_CONSTANT, 1);
    chunk->write(constant0, 1);
    chunk->write(OP_DIVIDE, 1);
    chunk->write(OP_RETURN, 1);

    InterpretResult result = vm.interpret(function);
    if (result == InterpretResult::RUNTIME_ERROR) {
        std::cout << "Division by zero correctly handled." << std::endl;
    } else {
        std::cout << "Division by zero NOT handled! Result: " << (int)result << std::endl;
        exit(1);
    }
}

int main() {
    testStackOverflow();
    testDivisionByZero();
    std::cout << "All stability tests passed." << std::endl;
    return 0;
}
