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
        OBJ_FUNCTION,
        OBJ_CLOSURE,
        OBJ_UPVALUE,
        OBJ_CLASS,
        OBJ_INSTANCE,
        OBJ_BOUND_METHOD
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
        int upvalueCount;
        Chunk chunk;
        ObjString* name;
    };

    struct ObjUpvalue : public Obj {
        Value* location;
        Value closed;
        ObjUpvalue* next; // For the open upvalue list
    };

    struct ObjClosure : public Obj {
        ObjFunction* function;
        ObjUpvalue** upvalues;
        int upvalueCount;
    };

    // Forward declare Table
    class Table;

    struct ObjClass : public Obj {
        ObjString* name;
        Table* methods;
        // Optional superclass, can be null
        struct ObjClass* superclass;
    };

    struct ObjInstance : public Obj {
        ObjClass* klass;
        Table* fields;
    };

    struct ObjBoundMethod : public Obj {
        Value receiver;
        ObjClosure* method;
    };

    inline bool isObjType(Value value, ObjType type) {
        return value.isObj() && value.as.obj->type == type;
    }

    // Helper functions
    ObjString* allocateString(const std::string& str);
    // If internTable is not null, it returns the interned string if found.
    ObjString* copyString(const char* chars, int length, Table* internTable = nullptr);
    ObjString* takeString(char* chars, int length, Table* internTable = nullptr);

    ObjNative* allocateNative(NativeFn function);
    ObjFunction* allocateFunction();
    ObjUpvalue* allocateUpvalue(Value* slot);
    ObjClosure* allocateClosure(ObjFunction* function);
    ObjClass* allocateClass(ObjString* name);
    ObjInstance* allocateInstance(ObjClass* klass);
    ObjBoundMethod* allocateBoundMethod(Value receiver, ObjClosure* method);

    void printObject(Value value);

}

#endif
