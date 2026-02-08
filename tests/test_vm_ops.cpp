#include "../src/vm/vm.h"
#include "../src/vm/chunk.h"
#include "../src/vm/value.h"
#include <iostream>
#include <cassert>

using namespace cxxx;

void test_arithmetic() {
    std::cout << "Testing Arithmetic..." << std::endl;
    VM vm;
    vm.init();

    ObjFunction* function = allocateFunction(&vm);
    Chunk* chunk = &function->chunk;

    int c1 = chunk->addConstant(NUMBER_VAL(1.2));
    int c2 = chunk->addConstant(NUMBER_VAL(3.4));

    // 1.2 + 3.4
    chunk->write(OP_CONSTANT, 1);
    chunk->write(c1, 1);
    chunk->write(OP_CONSTANT, 1);
    chunk->write(c2, 1);
    chunk->write(OP_ADD, 1);
    chunk->write(OP_RETURN, 1);

    vm.interpret(function);
    Value result = vm.pop();
    std::cout << "1.2 + 3.4 = " << result.asNumber() << std::endl;
    assert(result.asNumber() == 4.6);
}

void test_negate() {
    std::cout << "Testing Negate..." << std::endl;
    VM vm;
    vm.init();

    ObjFunction* function = allocateFunction(&vm);
    Chunk* chunk = &function->chunk;
    int c1 = chunk->addConstant(NUMBER_VAL(5));

    chunk->write(OP_CONSTANT, 1);
    chunk->write(c1, 1);
    chunk->write(OP_NEGATE, 1);
    chunk->write(OP_RETURN, 1);

    vm.interpret(function);
    Value result = vm.pop();
    assert(result.asNumber() == -5);
}

int main() {
    test_arithmetic();
    test_negate();
    std::cout << "All Opcode Tests Passed." << std::endl;
    return 0;
}
