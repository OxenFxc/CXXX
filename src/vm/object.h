#ifndef cxxx_object_h
#define cxxx_object_h

#include "common.h"
#include "value.h"
#include <string>

namespace cxxx {

    enum ObjType {
        OBJ_STRING,
        OBJ_NATIVE
    };

    struct Obj {
        ObjType type;
        Obj* next; // For GC
    };

    struct ObjString : public Obj {
        std::string str;
        uint32_t hash;
    };

    struct ObjNative : public Obj {
        NativeFn function;
    };

    inline bool isObjType(Value value, ObjType type) {
        return value.isObj() && value.as.obj->type == type;
    }

    // Forward declare Table
    class Table;

    // Helper functions
    ObjString* allocateString(const std::string& str);
    // If internTable is not null, it returns the interned string if found.
    ObjString* copyString(const char* chars, int length, Table* internTable = nullptr);
    ObjString* takeString(char* chars, int length, Table* internTable = nullptr);

    ObjNative* allocateNative(NativeFn function);

    void printObject(Value value);

}

#endif
