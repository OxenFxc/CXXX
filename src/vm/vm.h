#ifndef cxxx_vm_h
#define cxxx_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"
#include "../include/cxxx.h" // For InterpretResult

namespace cxxx {

    #define FRAMES_MAX 64
    #define STACK_MAX (FRAMES_MAX * 256)

    struct CallFrame {
        ObjFunction* function;
        uint8_t* ip;
        Value* slots;
    };

    class VM {
    public:
        VM();
        ~VM();

        void init();
        void free();

        InterpretResult interpret(ObjFunction* function);

        // Stack operations
        void push(Value value);
        Value pop();
        Value peek(int distance);
        bool stackEmpty();

        Table globals;
        Table strings;

    private:
        CallFrame frames[FRAMES_MAX];
        int frameCount;

        Value stack[STACK_MAX];
        Value* stackTop;

        InterpretResult run();

        void resetStack();
    };

}

#endif
