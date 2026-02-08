#include "object.h"
#include "table.h"
#include <iostream>
#include <cstring>

namespace cxxx {

    static uint32_t hashString(const char* key, int length) {
        uint32_t hash = 2166136261u;
        for (int i = 0; i < length; i++) {
            hash ^= (uint8_t)key[i];
            hash *= 16777619;
        }
        return hash;
    }

    ObjString* allocateString(const std::string& str) {
        ObjString* obj = new ObjString();
        obj->type = OBJ_STRING;
        obj->next = nullptr; // TODO: Hook into GC list
        obj->str = str;
        obj->hash = hashString(str.c_str(), str.length());
        return obj;
    }

    ObjString* copyString(const char* chars, int length, Table* internTable) {
        uint32_t hash = hashString(chars, length);
        if (internTable != nullptr) {
            ObjString* interned = internTable->findString(chars, length, hash);
            if (interned != nullptr) return interned;
        }

        ObjString* obj = allocateString(std::string(chars, length));
        if (internTable != nullptr) {
            internTable->set(obj, NIL_VAL());
        }
        return obj;
    }

    ObjString* takeString(char* chars, int length, Table* internTable) {
        uint32_t hash = hashString(chars, length);
        if (internTable != nullptr) {
            ObjString* interned = internTable->findString(chars, length, hash);
            if (interned != nullptr) return interned;
        }

        ObjString* obj = allocateString(std::string(chars, length));
        if (internTable != nullptr) {
            internTable->set(obj, NIL_VAL());
        }
        return obj;
    }

    ObjNative* allocateNative(NativeFn function) {
        ObjNative* native = new ObjNative();
        native->type = OBJ_NATIVE;
        native->next = nullptr;
        native->function = function;
        return native;
    }

    ObjFunction* allocateFunction() {
        ObjFunction* function = new ObjFunction();
        function->type = OBJ_FUNCTION;
        function->next = nullptr;
        function->arity = 0;
        function->name = nullptr;
        return function;
    }

    ObjClass* allocateClass(ObjString* name) {
        ObjClass* klass = new ObjClass();
        klass->type = OBJ_CLASS;
        klass->name = name;
        klass->methods = new Table();
        klass->superclass = nullptr;
        klass->next = nullptr;
        return klass;
    }

    ObjInstance* allocateInstance(ObjClass* klass) {
        ObjInstance* instance = new ObjInstance();
        instance->type = OBJ_INSTANCE;
        instance->klass = klass;
        instance->fields = new Table();
        instance->next = nullptr;
        return instance;
    }

    ObjBoundMethod* allocateBoundMethod(Value receiver, ObjFunction* method) {
        ObjBoundMethod* bound = new ObjBoundMethod();
        bound->type = OBJ_BOUND_METHOD;
        bound->receiver = receiver;
        bound->method = method;
        bound->next = nullptr;
        return bound;
    }

    void printObject(Value value) {
        switch (value.as.obj->type) {
            case OBJ_STRING:
                std::cout << ((ObjString*)value.as.obj)->str;
                break;
            case OBJ_NATIVE:
                std::cout << "<native fn>";
                break;
            case OBJ_FUNCTION:
                if (((ObjFunction*)value.as.obj)->name == nullptr) {
                    std::cout << "<script>";
                } else {
                    std::cout << "<fn " << ((ObjFunction*)value.as.obj)->name->str << ">";
                }
                break;
            case OBJ_CLASS:
                std::cout << ((ObjClass*)value.as.obj)->name->str;
                break;
            case OBJ_INSTANCE:
                std::cout << ((ObjInstance*)value.as.obj)->klass->name->str << " instance";
                break;
            case OBJ_BOUND_METHOD:
                std::cout << "<fn " << ((ObjBoundMethod*)value.as.obj)->method->name->str << ">";
                break;
        }
    }
}
