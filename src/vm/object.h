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
    class VM;   // Forward declare

    struct Obj {
        ObjType type;
        bool isMarked;
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
        ObjUpvalue* nextUpvalue; // For the open upvalue list
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
    ObjString* allocateString(VM* vm, const std::string& str);
    // Uses vm->strings for interning if vm is provided (though vm is required now)
    ObjString* copyString(VM* vm, const char* chars, int length);
    ObjString* takeString(VM* vm, char* chars, int length);

    ObjNative* allocateNative(VM* vm, NativeFn function);
    ObjFunction* allocateFunction(VM* vm);
    ObjUpvalue* allocateUpvalue(VM* vm, Value* slot);
    ObjClosure* allocateClosure(VM* vm, ObjFunction* function);
    ObjClass* allocateClass(VM* vm, ObjString* name);
    ObjInstance* allocateInstance(VM* vm, ObjClass* klass);
    ObjBoundMethod* allocateBoundMethod(VM* vm, Value receiver, ObjClosure* method);

    void freeObject(Obj* obj);
    void printObject(Value value);

}

#endif
