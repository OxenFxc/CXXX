#ifndef cxxx_value_h
#define cxxx_value_h

#include "common.h"
#include <iostream>

namespace cxxx {

    // Helper functions for constructing Values (internal usage)
    inline Value BOOL_VAL(bool value) {
        return Value::boolean(value);
    }

    inline Value NIL_VAL() {
        return Value::nil();
    }

    inline Value NUMBER_VAL(double value) {
        return Value::number(value);
    }

    inline Value OBJ_VAL(Obj* object) {
        return Value::object(object);
    }

    // Helper methods
    bool valuesEqual(Value a, Value b);
    void printValue(Value value);
}

#endif
