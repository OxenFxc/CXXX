#include "../include/cxxx.h"
#include "../vm/value.h"
#include "../vm/object.h"
#include <iostream>
#include <ctime>

namespace cxxx {

    Value clockNative(int argCount, Value* args) {
        return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    }

    Value strLenNative(int argCount, Value* args) {
        if (argCount != 1 || !isObjType(args[0], OBJ_STRING)) {
            return NIL_VAL();
        }
        ObjString* strObj = (ObjString*)args[0].as.obj;
        return NUMBER_VAL((double)strObj->str.length());
    }

    Value strAtNative(int argCount, Value* args) {
        if (argCount != 2 || !isObjType(args[0], OBJ_STRING) || !args[1].isNumber()) {
            return NIL_VAL();
        }
        ObjString* strObj = (ObjString*)args[0].as.obj;
        int index = (int)args[1].asNumber();
        if (index < 0 || index >= strObj->str.length()) return NIL_VAL();

        std::string sub = strObj->str.substr(index, 1);
        return OBJ_VAL((Obj*)copyString(sub.c_str(), 1));
    }

    // Init stdlib
    void initStdLib(CXXX& vm) {
        vm.registerFunction("clock", clockNative);
        vm.registerFunction("len", strLenNative);
        vm.registerFunction("strAt", strAtNative);
    }
}

// Implement loadStdLib method on CXXX
void cxxx::CXXX::loadStdLib() {
    cxxx::initStdLib(*this);
}
