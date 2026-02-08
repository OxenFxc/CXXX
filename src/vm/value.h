#ifndef cxxx_value_h
#define cxxx_value_h

#include "common.h"
#include <iostream>

namespace cxxx {

    enum ValueType {
        VAL_BOOL,
        VAL_NIL,
        VAL_NUMBER,
        VAL_OBJ
    };

    // Forward declaration for Object
    struct Obj;

    struct Value {
        ValueType type;
        union {
            bool boolean;
            double number;
            Obj* obj;
        } as;

        bool isBool() const { return type == VAL_BOOL; }
        bool isNil() const { return type == VAL_NIL; }
        bool isNumber() const { return type == VAL_NUMBER; }
        bool isObj() const { return type == VAL_OBJ; }

        double asNumber() const { return as.number; }
        bool asBool() const { return as.boolean; }
    };

    // Helper functions for constructing Values
    inline Value BOOL_VAL(bool value) {
        Value v;
        v.type = VAL_BOOL;
        v.as.boolean = value;
        return v;
    }

    inline Value NIL_VAL() {
        Value v;
        v.type = VAL_NIL;
        v.as.number = 0;
        return v;
    }

    inline Value NUMBER_VAL(double value) {
        Value v;
        v.type = VAL_NUMBER;
        v.as.number = value;
        return v;
    }

    inline Value OBJ_VAL(Obj* object) {
        Value v;
        v.type = VAL_OBJ;
        v.as.obj = object;
        return v;
    }

    // Helper methods
    bool valuesEqual(Value a, Value b);
    void printValue(Value value);
}

#endif
