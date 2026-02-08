#ifndef cxxx_vm_h
#define cxxx_vm_h

#include "common.h"
#include "chunk.h"
#include "value.h"
#include "table.h"
#include "object.h"
#include "../include/cxxx.h" // For InterpretResult
#include <vector>

namespace cxxx {

    #define FRAMES_MAX 256
    #define STACK_MAX (FRAMES_MAX * 256)

    struct CallFrame {
        ObjClosure* closure;
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

        // GC
        Obj* objects; // Linked list of all objects
        std::vector<Obj*> grayStack; // For GC marking

        void collectGarbage();
        void markObject(Obj* obj);
        void markValue(Value value);
        void markRoots();
        void traceReferences();
        void blackenObject(Obj* obj);
        void sweep();
        void freeObjects();
        void markTable(Table* table);

    private:
        CallFrame frames[FRAMES_MAX];
        int frameCount;

        Value stack[STACK_MAX];
        Value* stackTop;
        ObjUpvalue* openUpvalues;

        InterpretResult run();

        void resetStack();
        ObjUpvalue* captureUpvalue(Value* local);
        void closeUpvalues(Value* last);
        void defineMethod(ObjString* name);
        bool bindMethod(ObjClass* klass, ObjString* name);
        bool callValue(Value callee, int argCount);
        bool invoke(ObjString* name, int argCount);
        bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount);
    };

}

#endif
