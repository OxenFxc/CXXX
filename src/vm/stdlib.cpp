#include "../include/cxxx.h"
#include "../vm/value.h"
#include <iostream>
#include <ctime>

namespace cxxx {

    Value clockNative(int argCount, Value* args) {
        return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
    }

    // Init stdlib
    void initStdLib(CXXX& vm) {
        vm.registerFunction("clock", clockNative);
    }
}

// Implement loadStdLib method on CXXX
void cxxx::CXXX::loadStdLib() {
    cxxx::initStdLib(*this);
}
