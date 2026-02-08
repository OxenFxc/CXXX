#include "../src/vm/vm.h"
#include "../src/vm/chunk.h"
#include "../src/vm/value.h"
#include <iostream>
#include <cassert>

using namespace cxxx;

int main() {
    std::cout << "Testing VM Stack..." << std::endl;
    VM vm;
    vm.init();

    vm.push(NUMBER_VAL(1.23));
    vm.push(BOOL_VAL(true));
    vm.push(NIL_VAL());

    assert(vm.peek(0).isNil());
    assert(vm.peek(1).isBool());
    assert(vm.peek(2).isNumber());

    Value v1 = vm.pop();
    assert(v1.isNil());
    Value v2 = vm.pop();
    assert(v2.asBool() == true);
    Value v3 = vm.pop();
    assert(v3.asNumber() == 1.23);

    std::cout << "Stack tests passed." << std::endl;

    // Minimal Run Test
    ObjFunction* function = allocateFunction(&vm);
    Chunk* chunk = &function->chunk;

    int constant = chunk->addConstant(NUMBER_VAL(42));
    chunk->write(OP_CONSTANT, 123);
    chunk->write(constant, 123);
    chunk->write(OP_RETURN, 123);

    // interpret calls run()
    InterpretResult result = vm.interpret(function);
    if (result == InterpretResult::OK) {
        std::cout << "VM Run Test Passed." << std::endl;
    } else {
        std::cout << "VM Run Test Failed." << std::endl;
        return 1;
    }

    return 0;
}
