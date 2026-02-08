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
        VM* v = (VM*)vm;

        ObjFunction* function = compile(v, source);
        if (function == nullptr) {
            return InterpretResult::COMPILE_ERROR;
        }

        v->push(OBJ_VAL((Obj*)function));
        InterpretResult result = v->interpret(function);

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

    double CXXX::getGlobalNumber(const std::string& name) {
        VM* v = (VM*)vm;
        ObjString* str = copyString(v, name.c_str(), name.length());
        Value val;
        if (v->globals.get(str, &val)) {
            if (val.isNumber()) return val.asNumber();
        }
        return 0.0;
    }

    bool CXXX::getGlobalBool(const std::string& name) {
        VM* v = (VM*)vm;
        ObjString* str = copyString(v, name.c_str(), name.length());
        Value val;
        if (v->globals.get(str, &val)) {
            if (val.isBool()) return val.asBool();
        }
        return false;
    }

    void CXXX::setGlobal(const std::string& name, Value val) {
        VM* v = (VM*)vm;
        ObjString* str = copyString(v, name.c_str(), name.length());
        v->globals.set(str, val);
    }

    Value CXXX::createString(const std::string& s) {
        VM* v = (VM*)vm;
        ObjString* str = copyString(v, s.c_str(), s.length());
        return Value::object((Obj*)str);
    }

    void CXXX::registerFunction(const char* name, NativeFn fn) {
        VM* v = (VM*)vm;
        ObjString* fnName = copyString(v, name, strlen(name));
        v->globals.set(fnName, Value::object((Obj*)allocateNative(v, fn)));
    }
}
