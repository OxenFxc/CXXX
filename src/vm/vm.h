#ifndef cxxx_vm_h
#define cxxx_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"
#include "../include/cxxx.h" // For InterpretResult

namespace cxxx {

    #define STACK_MAX 256

    class VM {
    public:
        VM();
        ~VM();

        void init();
        void free();

        InterpretResult interpret(Chunk* chunk);

        // Stack operations
        void push(Value value);
        Value pop();
        Value peek(int distance);
        bool stackEmpty();

        Table globals;
        Table strings;

    private:
        Chunk* chunk;
        uint8_t* ip;
        Value stack[STACK_MAX];
        Value* stackTop;

        InterpretResult run();

        void resetStack();
    };

}

#endif
