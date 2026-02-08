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
        OP_LOOP,
        OP_POP,
        OP_DEFINE_GLOBAL,
        OP_GET_GLOBAL,
        OP_SET_GLOBAL,
        OP_GET_LOCAL,
        OP_SET_LOCAL,
        OP_CALL,
        OP_PRINT,
        OP_CLASS,
        OP_METHOD,
        OP_GET_PROPERTY,
        OP_SET_PROPERTY,
        OP_INVOKE,
        OP_INHERIT,
        OP_GET_SUPER,
        OP_SUPER_INVOKE,
        OP_CLOSURE,
        OP_GET_UPVALUE,
        OP_SET_UPVALUE,
        OP_CLOSE_UPVALUE,
        OP_INSTANCEOF
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
