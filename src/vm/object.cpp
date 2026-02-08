#include "object.h"
#include "table.h"
#include "vm.h"
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

    ObjString* allocateString(VM* vm, const std::string& str) {
        ObjString* obj = new ObjString();
        obj->type = OBJ_STRING;
        obj->isMarked = false;
        obj->next = vm->objects;
        vm->objects = obj;
        obj->str = str;
        obj->hash = hashString(str.c_str(), str.length());
        return obj;
    }

    ObjString* copyString(VM* vm, const char* chars, int length) {
        uint32_t hash = hashString(chars, length);
        ObjString* interned = vm->strings.findString(chars, length, hash);
        if (interned != nullptr) return interned;

        ObjString* obj = allocateString(vm, std::string(chars, length));
        vm->strings.set(obj, NIL_VAL());
        return obj;
    }

    ObjString* takeString(VM* vm, char* chars, int length) {
        uint32_t hash = hashString(chars, length);
        ObjString* interned = vm->strings.findString(chars, length, hash);
        if (interned != nullptr) return interned;

        ObjString* obj = allocateString(vm, std::string(chars, length));
        vm->strings.set(obj, NIL_VAL());
        return obj;
    }

    ObjNative* allocateNative(VM* vm, NativeFn function) {
        ObjNative* native = new ObjNative();
        native->type = OBJ_NATIVE;
        native->isMarked = false;
        native->next = vm->objects;
        vm->objects = native;
        native->function = function;
        return native;
    }

    ObjFunction* allocateFunction(VM* vm) {
        ObjFunction* function = new ObjFunction();
        function->type = OBJ_FUNCTION;
        function->isMarked = false;
        function->next = vm->objects;
        vm->objects = function;
        function->arity = 0;
        function->upvalueCount = 0;
        function->name = nullptr;
        return function;
    }

    ObjUpvalue* allocateUpvalue(VM* vm, Value* slot) {
        ObjUpvalue* upvalue = new ObjUpvalue();
        upvalue->type = OBJ_UPVALUE;
        upvalue->isMarked = false;
        upvalue->next = vm->objects;
        vm->objects = upvalue;
        upvalue->location = slot;
        upvalue->closed = NIL_VAL();
        upvalue->nextUpvalue = nullptr;
        return upvalue;
    }

    ObjClosure* allocateClosure(VM* vm, ObjFunction* function) {
        ObjClosure* closure = new ObjClosure();
        closure->type = OBJ_CLOSURE;
        closure->isMarked = false;
        closure->next = vm->objects;
        vm->objects = closure;
        closure->function = function;
        closure->upvalues = new ObjUpvalue*[function->upvalueCount];
        closure->upvalueCount = function->upvalueCount;
        for (int i = 0; i < function->upvalueCount; i++) {
            closure->upvalues[i] = nullptr;
        }
        return closure;
    }

    ObjClass* allocateClass(VM* vm, ObjString* name) {
        ObjClass* klass = new ObjClass();
        klass->type = OBJ_CLASS;
        klass->isMarked = false;
        klass->next = vm->objects;
        vm->objects = klass;
        klass->name = name;
        klass->methods = new Table();
        klass->superclass = nullptr;
        return klass;
    }

    ObjInstance* allocateInstance(VM* vm, ObjClass* klass) {
        ObjInstance* instance = new ObjInstance();
        instance->type = OBJ_INSTANCE;
        instance->isMarked = false;
        instance->next = vm->objects;
        vm->objects = instance;
        instance->klass = klass;
        instance->fields = new Table();
        return instance;
    }

    ObjBoundMethod* allocateBoundMethod(VM* vm, Value receiver, ObjClosure* method) {
        ObjBoundMethod* bound = new ObjBoundMethod();
        bound->type = OBJ_BOUND_METHOD;
        bound->isMarked = false;
        bound->next = vm->objects;
        vm->objects = bound;
        bound->receiver = receiver;
        bound->method = method;
        return bound;
    }

    void freeObject(Obj* obj) {
        switch (obj->type) {
            case OBJ_STRING: {
                ObjString* string = (ObjString*)obj;
                delete string;
                break;
            }
            case OBJ_NATIVE: {
                delete (ObjNative*)obj;
                break;
            }
            case OBJ_FUNCTION: {
                ObjFunction* function = (ObjFunction*)obj;
                delete function;
                break;
            }
            case OBJ_CLOSURE: {
                ObjClosure* closure = (ObjClosure*)obj;
                delete[] closure->upvalues;
                delete closure;
                break;
            }
            case OBJ_UPVALUE: {
                delete (ObjUpvalue*)obj;
                break;
            }
            case OBJ_CLASS: {
                ObjClass* klass = (ObjClass*)obj;
                delete klass->methods;
                delete klass;
                break;
            }
            case OBJ_INSTANCE: {
                ObjInstance* instance = (ObjInstance*)obj;
                delete instance->fields;
                delete instance;
                break;
            }
            case OBJ_BOUND_METHOD: {
                delete (ObjBoundMethod*)obj;
                break;
            }
        }
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
            case OBJ_CLOSURE:
                if (((ObjClosure*)value.as.obj)->function->name == nullptr) {
                    std::cout << "<script>";
                } else {
                    std::cout << "<fn " << ((ObjClosure*)value.as.obj)->function->name->str << ">";
                }
                break;
            case OBJ_UPVALUE:
                std::cout << "upvalue";
                break;
            case OBJ_CLASS:
                std::cout << ((ObjClass*)value.as.obj)->name->str;
                break;
            case OBJ_INSTANCE:
                std::cout << ((ObjInstance*)value.as.obj)->klass->name->str << " instance";
                break;
            case OBJ_BOUND_METHOD:
                if (((ObjBoundMethod*)value.as.obj)->method->function->name == nullptr) {
                    std::cout << "<script>";
                } else {
                    std::cout << "<fn " << ((ObjBoundMethod*)value.as.obj)->method->function->name->str << ">";
                }
                break;
        }
    }
}
