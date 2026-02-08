#ifndef cxxx_object_h
#define cxxx_object_h

#include "common.h"
#include "value.h"
#include "chunk.h"
#include <string>

namespace cxxx {

    enum ObjType {
        OBJ_STRING,
        OBJ_NATIVE,
        OBJ_FUNCTION
    };

    struct Obj; // Forward declare

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

    struct ObjFunction : public Obj {
        int arity;
        Chunk chunk;
        ObjString* name;
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
    ObjFunction* allocateFunction();

    void printObject(Value value);

}

#endif
