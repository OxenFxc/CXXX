#include "../include/cxxx.h"
#include "vm.h"
#include "value.h"
#include "../compiler/compiler.h"
#include "chunk.h"
#include <iostream>

namespace cxxx {

    CXXX::CXXX() {
        vm = new VM();
        ((VM*)vm)->init();
        lastResult = 0.0;
    }

    CXXX::~CXXX() {
        // cleanup
        ((VM*)vm)->free();
        delete (VM*)vm;
    }

    InterpretResult CXXX::interpret(const std::string& source) {
        Chunk chunk;
        VM* v = (VM*)vm;

        if (!compile(source, &chunk)) {
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
             } else {
                 lastResult = 0.0;
             }
        }

        return result;
    }

    double CXXX::getResult() {
        return lastResult;
    }
}
