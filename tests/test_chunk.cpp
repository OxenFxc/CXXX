#include "../src/vm/chunk.h"
#include "../src/vm/value.h"
#include <iostream>

using namespace cxxx;

int main() {
    Chunk chunk;
    int constant = chunk.addConstant(NUMBER_VAL(1.2));
    chunk.write(OP_CONSTANT, 123);
    chunk.write(constant, 123);
    chunk.write(OP_RETURN, 123);

    chunk.disassemble("Test Chunk");

    return 0;
}
