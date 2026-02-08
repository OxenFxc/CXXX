#ifndef cxxx_chunk_h
#define cxxx_chunk_h

#include "common.h"
#include "value.h"
#include <vector>
#include <cstdint>

namespace cxxx {

    enum OpCode {
        OP_CONSTANT,
        OP_RETURN,
        OP_NEGATE,
        OP_ADD,
        OP_SUBTRACT,
        OP_MULTIPLY,
        OP_DIVIDE,
        OP_NOT,
        OP_EQUAL,
        OP_GREATER,
        OP_LESS,
        OP_JUMP,
        OP_JUMP_IF_FALSE,
        OP_POP,
        OP_DEFINE_GLOBAL,
        OP_GET_GLOBAL,
        OP_SET_GLOBAL,
        OP_CALL,
        OP_PRINT
    };

    class Chunk {
    public:
        Chunk();
        ~Chunk();

        void write(uint8_t byte, int line);
        int addConstant(Value value);

        std::vector<uint8_t> code;
        std::vector<int> lines;
        std::vector<Value> constants;

        // Debugging / Disassembly
        void disassemble(const char* name);
        int disassembleInstruction(int offset);
    };

}

#endif
