#include "value.h"
#include "object.h"
#include <iostream>

namespace cxxx {

    bool valuesEqual(Value a, Value b) {
        if (a.type != b.type) return false;
        switch (a.type) {
            case VAL_BOOL: return a.as.boolean == b.as.boolean;
            case VAL_NIL: return true;
            case VAL_NUMBER: return a.as.number == b.as.number;
            case VAL_OBJ: {
                if (a.as.obj == b.as.obj) return true;
                if (a.as.obj->type == OBJ_STRING && b.as.obj->type == OBJ_STRING) {
                    ObjString* sa = (ObjString*)a.as.obj;
                    ObjString* sb = (ObjString*)b.as.obj;
                    return sa->str == sb->str;
                }
                return false;
            }
            default: return false; // Should not happen
        }
    }

    void printValue(Value value) {
        switch (value.type) {
            case VAL_BOOL:
                std::cout << (value.as.boolean ? "true" : "false");
                break;
            case VAL_NIL:
                std::cout << "nil";
                break;
            case VAL_NUMBER:
                std::cout << value.as.number;
                break;
            case VAL_OBJ:
                printObject(value);
                break;
        }
    }
}
