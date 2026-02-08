#include "../include/cxxx.h"
#include "vm.h"
#include "value.h"
#include "object.h"
#include "../compiler/compiler.h"
#include "chunk.h"
#include <iostream>
#include <cstring>

namespace cxxx {

    CXXX::CXXX() {
        vm = new VM();
        ((VM*)vm)->init();
        lastResult = 0.0;
        loadStdLib();
    }

    CXXX::~CXXX() {
        // cleanup
        ((VM*)vm)->free();
        delete (VM*)vm;
    }

    InterpretResult CXXX::interpret(const std::string& source) {
        Chunk chunk;
        VM* v = (VM*)vm;

        if (!compile(source, &chunk, &v->strings)) {
            return InterpretResult::COMPILE_ERROR;
        }

        InterpretResult result = v->interpret(&chunk);

        if (result == InterpretResult::OK) {
             if (!v->stackEmpty()) {
                 Value val = v->pop();
                 if (val.isNumber()) {
                     lastResult = val.asNumber();
                 } else {
                     lastResult = 0.0;
                 }
                 // Keep stack clean? No, we just popped.
             } else {
                 // std::cout << "Stack empty after interpret." << std::endl;
                 // lastResult = 0.0;
                 // If stack is empty, we keep previous lastResult? Or reset?
                 // For now, reset.
                 // lastResult = 0.0; // Don't reset, so we can see side-effect results?
                 // But wait, "var a = ..." leaves stack empty.
                 // "a;" leaves value on stack.
                 // So if "a;" is last, we get it.
                 // If "print a;" is last, stack is empty.
                 // So getResult() returns 0 or prev value?
                 // Let's reset to avoid confusion.
                 lastResult = 0.0;
             }
        }

        return result;
    }

    double CXXX::getResult() {
        return lastResult;
    }

    void CXXX::registerFunction(const char* name, NativeFn fn) {
        VM* v = (VM*)vm;
        ObjString* fnName = copyString(name, strlen(name), &v->strings);
        v->globals.set(fnName, OBJ_VAL((Obj*)allocateNative((cxxx::NativeFn)fn)));
    }
}
