#include "vm.h"
#include <iostream>

namespace cxxx {

    VM::VM() : globals(), strings() {
        resetStack();
        openUpvalues = nullptr;
        objects = nullptr;
        frameCount = 0;
    }

    VM::~VM() {
        free();
    }

    void VM::init() {
        resetStack();
        openUpvalues = nullptr;
        objects = nullptr;
        frameCount = 0;
    }

    void VM::free() {
        freeObjects();
    }

    void VM::resetStack() {
        stackTop = stack;
        frameCount = 0;
        openUpvalues = nullptr;
    }

    void VM::push(Value value) {
        if (stackTop - stack >= STACK_MAX) {
            std::cerr << "Stack overflow!" << std::endl;
            // In a real robust system, we would signal error, but here we just return to avoid crash.
            // Or better, set an error flag.
            return;
        }
        *stackTop = value;
        stackTop++;
    }

    Value VM::pop() {
        if (stackTop == stack) {
            std::cerr << "Stack underflow!" << std::endl;
            return NIL_VAL();
        }
        stackTop--;
        return *stackTop;
    }

    Value VM::peek(int distance) {
        return stackTop[-1 - distance];
    }

    bool VM::stackEmpty() {
        return stackTop == stack;
    }

    InterpretResult VM::interpret(ObjFunction* function) {
        ObjClosure* closure = allocateClosure(this, function);
        push(OBJ_VAL((Obj*)closure));
        callValue(OBJ_VAL((Obj*)closure), 0);

        return run();
    }

    bool isFalsey(Value value) {
        return value.isNil() || (value.isBool() && !value.asBool());
    }

    InterpretResult VM::run() {
        CallFrame* frame = &frames[frameCount - 1];

        #define READ_BYTE() (*frame->ip++)
        #define READ_CONSTANT() (frame->closure->function->chunk.constants[READ_BYTE()])
        #define READ_STRING() ((ObjString*)READ_CONSTANT().as.obj)

        for (;;) {
            #ifdef DEBUG_TRACE_EXECUTION
                std::cout << "          ";
                for (Value* slot = stack; slot < stackTop; slot++) {
                    std::cout << "[ ";
                    printValue(*slot);
                    std::cout << " ]";
                }
                std::cout << std::endl;
                frame->closure->function->chunk.disassembleInstruction((int)(frame->ip - frame->closure->function->chunk.code.data()));
            #endif

            uint8_t instruction;
            switch (instruction = READ_BYTE()) {
                case OP_CONSTANT: {
                    Value constant = READ_CONSTANT();
                    push(constant);
                    break;
                }
                case OP_ADD: {
                    if (isObjType(peek(0), OBJ_STRING) && isObjType(peek(1), OBJ_STRING)) {
                        ObjString* b = (ObjString*)peek(0).as.obj;
                        ObjString* a = (ObjString*)peek(1).as.obj;
                        std::string s = a->str + b->str;
                        pop();
                        pop();
                        push(OBJ_VAL((Obj*)copyString(this, s.c_str(), (int)s.length())));
                    } else if (peek(0).isNumber() && peek(1).isNumber()) {
                        double b = pop().asNumber();
                        double a = pop().asNumber();
                        push(NUMBER_VAL(a + b));
                    } else if (isObjType(peek(0), OBJ_STRING) && peek(1).isNumber()) {
                        // Number + String -> String
                        ObjString* b = (ObjString*)peek(0).as.obj;
                        double aVal = peek(1).asNumber();
                        std::string aStr = std::to_string(aVal);
                        if (aStr.find('.') != std::string::npos) {
                            while (aStr.back() == '0') aStr.pop_back();
                            if (aStr.back() == '.') aStr.pop_back();
                        }
                        std::string s = aStr + b->str;
                        pop();
                        pop();
                        push(OBJ_VAL((Obj*)copyString(this, s.c_str(), (int)s.length())));
                    } else if (peek(0).isNumber() && isObjType(peek(1), OBJ_STRING)) {
                        // String + Number -> String
                        double bVal = peek(0).asNumber();
                        ObjString* a = (ObjString*)peek(1).as.obj;
                        std::string bStr = std::to_string(bVal);
                        if (bStr.find('.') != std::string::npos) {
                            while (bStr.back() == '0') bStr.pop_back();
                            if (bStr.back() == '.') bStr.pop_back();
                        }
                        std::string s = a->str + bStr;
                        pop();
                        pop();
                        push(OBJ_VAL((Obj*)copyString(this, s.c_str(), (int)s.length())));
                    } else {
                        std::cerr << "Operands must be numbers or strings." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    break;
                }
                case OP_SUBTRACT: {
                    double b = pop().asNumber();
                    double a = pop().asNumber();
                    push(NUMBER_VAL(a - b));
                    break;
                }
                case OP_MULTIPLY: {
                    double b = pop().asNumber();
                    double a = pop().asNumber();
                    push(NUMBER_VAL(a * b));
                    break;
                }
                case OP_DIVIDE: {
                    double b = pop().asNumber();
                    double a = pop().asNumber();
                    push(NUMBER_VAL(a / b));
                    break;
                }
                case OP_NOT: {
                    push(BOOL_VAL(isFalsey(pop())));
                    break;
                }
                case OP_EQUAL: {
                    Value b = pop();
                    Value a = pop();
                    push(BOOL_VAL(valuesEqual(a, b)));
                    break;
                }
                case OP_GREATER: {
                    double b = pop().asNumber();
                    double a = pop().asNumber();
                    push(BOOL_VAL(a > b));
                    break;
                }
                case OP_LESS: {
                    double b = pop().asNumber();
                    double a = pop().asNumber();
                    push(BOOL_VAL(a < b));
                    break;
                }
                case OP_JUMP: {
                    uint16_t offset = (uint16_t)(READ_BYTE() << 8);
                    offset |= READ_BYTE();
                    frame->ip += offset;
                    break;
                }
                case OP_JUMP_IF_FALSE: {
                    uint16_t offset = (uint16_t)(READ_BYTE() << 8);
                    offset |= READ_BYTE();
                    if (isFalsey(peek(0))) {
                        frame->ip += offset;
                    }
                    break;
                }
                case OP_LOOP: {
                    uint16_t offset = (uint16_t)(READ_BYTE() << 8);
                    offset |= READ_BYTE();
                    frame->ip -= offset;
                    break;
                }
                case OP_POP: {
                    pop();
                    break;
                }
                case OP_GET_LOCAL: {
                    uint8_t slot = READ_BYTE();
                    push(frame->slots[slot]);
                    break;
                }
                case OP_SET_LOCAL: {
                    uint8_t slot = READ_BYTE();
                    frame->slots[slot] = peek(0);
                    break;
                }
                case OP_GET_GLOBAL: {
                    ObjString* name = READ_STRING();
                    Value value;
                    if (!globals.get(name, &value)) {
                        std::cerr << "Undefined variable '" << name->str << "'." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    push(value);
                    break;
                }
                case OP_DEFINE_GLOBAL: {
                    ObjString* name = READ_STRING();
                    globals.set(name, peek(0));
                    pop();
                    break;
                }
                case OP_SET_GLOBAL: {
                    ObjString* name = READ_STRING();
                    if (globals.set(name, peek(0))) {
                        globals.deleteEntry(name);
                        std::cerr << "Undefined variable '" << name->str << "'." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    break;
                }
                case OP_CLASS: {
                    push(OBJ_VAL((Obj*)allocateClass(this, READ_STRING())));
                    break;
                }
                case OP_METHOD: {
                    defineMethod(READ_STRING());
                    break;
                }
                case OP_GET_PROPERTY: {
                    if (!isObjType(peek(0), OBJ_INSTANCE)) {
                        std::cerr << "Only instances have properties." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    ObjInstance* instance = (ObjInstance*)peek(0).as.obj;
                    ObjString* name = READ_STRING();

                    Value value;
                    if (instance->fields->get(name, &value)) {
                        pop(); // Instance.
                        push(value);
                        break;
                    }

                    if (!bindMethod(instance->klass, name)) {
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    break;
                }
                case OP_SET_PROPERTY: {
                    if (!isObjType(peek(1), OBJ_INSTANCE)) {
                        std::cerr << "Only instances have fields." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    ObjInstance* instance = (ObjInstance*)peek(1).as.obj;
                    instance->fields->set(READ_STRING(), peek(0));
                    Value value = pop();
                    pop();
                    push(value);
                    break;
                }
                case OP_INVOKE: {
                    ObjString* method = READ_STRING();
                    int argCount = READ_BYTE();
                    if (!invoke(method, argCount)) {
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    frame = &frames[frameCount - 1];
                    break;
                }
                case OP_INHERIT: {
                    Value superclass = peek(1);
                    if (!isObjType(superclass, OBJ_CLASS)) {
                        std::cerr << "Superclass must be a class." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    ObjClass* subclass = (ObjClass*)peek(0).as.obj;
                    subclass->superclass = (ObjClass*)superclass.as.obj; // Set superclass
                    pop(); // Subclass.
                    break;
                }
                case OP_GET_SUPER: {
                    ObjString* name = READ_STRING();
                    ObjClass* superclass = (ObjClass*)pop().as.obj;
                    if (!bindMethod(superclass, name)) {
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    break;
                }
                case OP_SUPER_INVOKE: {
                    ObjString* method = READ_STRING();
                    int argCount = READ_BYTE();
                    ObjClass* superclass = (ObjClass*)pop().as.obj;
                    if (!invokeFromClass(superclass, method, argCount)) {
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    frame = &frames[frameCount - 1];
                    break;
                }
                case OP_CALL: {
                    int argCount = READ_BYTE();
                    if (!callValue(peek(argCount), argCount)) {
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    frame = &frames[frameCount - 1];
                    break;
                }
                case OP_PRINT: {
                    printValue(pop());
                    std::cout << std::endl;
                    break;
                }
                case OP_CLOSURE: {
                    ObjFunction* function = (ObjFunction*)READ_CONSTANT().as.obj;
                    ObjClosure* closure = allocateClosure(this, function);
                    push(OBJ_VAL((Obj*)closure));
                    for (int i = 0; i < closure->upvalueCount; i++) {
                        uint8_t isLocal = READ_BYTE();
                        uint8_t index = READ_BYTE();
                        if (isLocal) {
                            closure->upvalues[i] = captureUpvalue(frame->slots + index);
                        } else {
                            closure->upvalues[i] = frame->closure->upvalues[index];
                        }
                    }
                    break;
                }
                case OP_GET_UPVALUE: {
                    uint8_t slot = READ_BYTE();
                    push(*frame->closure->upvalues[slot]->location);
                    break;
                }
                case OP_SET_UPVALUE: {
                    uint8_t slot = READ_BYTE();
                    *frame->closure->upvalues[slot]->location = peek(0);
                    break;
                }
                case OP_CLOSE_UPVALUE: {
                    closeUpvalues(stackTop - 1);
                    pop();
                    break;
                }
                case OP_INSTANCEOF: {
                    Value superclass = peek(0);
                    if (!isObjType(superclass, OBJ_CLASS)) {
                        std::cerr << "Right operand must be a class." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    Value instance = peek(1);
                    if (!isObjType(instance, OBJ_INSTANCE)) {
                        pop(); // superclass
                        pop(); // instance
                        push(BOOL_VAL(false));
                        break;
                    }

                    ObjClass* targetClass = (ObjClass*)superclass.as.obj;
                    ObjInstance* obj = (ObjInstance*)instance.as.obj;
                    ObjClass* currentClass = obj->klass;

                    bool found = false;
                    while (currentClass != nullptr) {
                        if (currentClass == targetClass) {
                            found = true;
                            break;
                        }
                        currentClass = currentClass->superclass;
                    }

                    pop(); // superclass
                    pop(); // instance
                    push(BOOL_VAL(found));
                    break;
                }
                case OP_NEGATE: {
                    push(NUMBER_VAL(-pop().asNumber()));
                    break;
                }
                case OP_RETURN: {
                    Value result = pop();
                    closeUpvalues(frame->slots);
                    frameCount--;
                    if (frameCount == 0) {
                        pop();
                        push(result);
                        return InterpretResult::OK;
                    }

                    stackTop = frame->slots;
                    push(result);
                    frame = &frames[frameCount - 1];
                    break;
                }
                default:
                    return InterpretResult::RUNTIME_ERROR;
            }
        }

        #undef READ_BYTE
        #undef READ_CONSTANT
        #undef READ_STRING
    }

    ObjUpvalue* VM::captureUpvalue(Value* local) {
        ObjUpvalue* prevUpvalue = nullptr;
        ObjUpvalue* upvalue = openUpvalues;

        while (upvalue != nullptr && upvalue->location > local) {
            prevUpvalue = upvalue;
            upvalue = upvalue->nextUpvalue;
        }

        if (upvalue != nullptr && upvalue->location == local) {
            return upvalue;
        }

        ObjUpvalue* createdUpvalue = allocateUpvalue(this, local);
        createdUpvalue->nextUpvalue = upvalue;

        if (prevUpvalue == nullptr) {
            openUpvalues = createdUpvalue;
        } else {
            prevUpvalue->nextUpvalue = createdUpvalue;
        }

        return createdUpvalue;
    }

    void VM::closeUpvalues(Value* last) {
        while (openUpvalues != nullptr && openUpvalues->location >= last) {
            ObjUpvalue* upvalue = openUpvalues;
            upvalue->closed = *upvalue->location;
            upvalue->location = &upvalue->closed;
            openUpvalues = upvalue->nextUpvalue;
        }
    }

    void VM::defineMethod(ObjString* name) {
        Value method = peek(0);
        ObjClass* klass = (ObjClass*)peek(1).as.obj;
        klass->methods->set(name, method);
        pop();
    }

    bool VM::bindMethod(ObjClass* klass, ObjString* name) {
        Value method;
        ObjClass* current = klass;
        while (current != nullptr) {
            if (current->methods->get(name, &method)) {
                ObjBoundMethod* bound = allocateBoundMethod(this, peek(0), (ObjClosure*)method.as.obj);
                pop();
                push(OBJ_VAL((Obj*)bound));
                return true;
            }
            current = current->superclass;
        }

        std::cerr << "Undefined property '" << name->str << "'." << std::endl;
        return false;
    }

    bool VM::callValue(Value callee, int argCount) {
        if (isObjType(callee, OBJ_BOUND_METHOD)) {
            ObjBoundMethod* bound = (ObjBoundMethod*)callee.as.obj;
            stackTop[-argCount - 1] = bound->receiver;
            return callValue(OBJ_VAL((Obj*)bound->method), argCount);
        }
        else if (isObjType(callee, OBJ_CLASS)) {
            ObjClass* klass = (ObjClass*)callee.as.obj;
            stackTop[-argCount - 1] = OBJ_VAL(allocateInstance(this, klass));
            Value initializer;
            if (klass->methods->get(copyString(this, "init", 4), &initializer)) {
                return callValue(initializer, argCount);
            } else if (argCount != 0) {
                std::cerr << "Expected 0 arguments but got " << argCount << "." << std::endl;
                return false;
            }
            return true;
        }
        else if (isObjType(callee, OBJ_CLOSURE)) {
            ObjClosure* closure = (ObjClosure*)callee.as.obj;
            if (argCount != closure->function->arity) {
                std::cerr << "Expected " << closure->function->arity << " arguments but got " << argCount << "." << std::endl;
                return false;
            }
            if (frameCount == FRAMES_MAX) {
                std::cerr << "Stack overflow." << std::endl;
                return false;
            }
            CallFrame* newFrame = &frames[frameCount++];
            newFrame->closure = closure;
            newFrame->ip = closure->function->chunk.code.data();
            newFrame->slots = stackTop - argCount - 1;
            return true;
        }
        else if (isObjType(callee, OBJ_NATIVE)) {
            NativeFn native = ((ObjNative*)callee.as.obj)->function;
            Value result = native(this, argCount, stackTop - argCount);
            stackTop -= argCount + 1;
            push(result);
            return true;
        }
        std::cerr << "Can only call functions and classes." << std::endl;
        return false;
    }

    bool VM::invoke(ObjString* name, int argCount) {
        Value receiver = peek(argCount);
        if (!isObjType(receiver, OBJ_INSTANCE)) {
             std::cerr << "Only instances have methods." << std::endl;
             return false;
        }
        ObjInstance* instance = (ObjInstance*)receiver.as.obj;

        Value value;
        if (instance->fields->get(name, &value)) {
            stackTop[-argCount - 1] = value;
            return callValue(value, argCount);
        }

        return invokeFromClass(instance->klass, name, argCount);
    }

    bool VM::invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
        Value method;
        ObjClass* current = klass;
        while (current != nullptr) {
            if (current->methods->get(name, &method)) {
                return callValue(method, argCount);
            }
            current = current->superclass;
        }

        std::cerr << "Undefined property '" << name->str << "'." << std::endl;
        return false;
    }

    // GC

    void VM::freeObjects() {
        Obj* object = objects;
        while (object != nullptr) {
            Obj* next = object->next;
            freeObject(object);
            object = next;
        }
        objects = nullptr;
    }

    void VM::collectGarbage() {
        markRoots();
        traceReferences();
        sweep();
    }

    void VM::markRoots() {
        for (Value* slot = stack; slot < stackTop; slot++) {
            markValue(*slot);
        }

        markTable(&globals);

        // Closures on call frames are usually on stack, but marking them explicitly is safe
        for (int i = 0; i < frameCount; i++) {
            markObject((Obj*)frames[i].closure);
        }

        for (ObjUpvalue* upvalue = openUpvalues; upvalue != nullptr; upvalue = upvalue->nextUpvalue) {
            markObject((Obj*)upvalue);
        }
    }

    void VM::markTable(Table* table) {
        for (int i = 0; i < table->capacity; i++) {
            Entry* entry = &table->entries[i];
            if (entry->key != nullptr) {
                markObject((Obj*)entry->key);
                markValue(entry->value);
            }
        }
    }

    void VM::markValue(Value value) {
        if (value.isObj()) markObject(value.as.obj);
    }

    void VM::markObject(Obj* obj) {
        if (obj == nullptr) return;
        if (obj->isMarked) return;

        obj->isMarked = true;
        grayStack.push_back(obj);
    }

    void VM::traceReferences() {
        while (!grayStack.empty()) {
            Obj* obj = grayStack.back();
            grayStack.pop_back();
            blackenObject(obj);
        }
    }

    void VM::blackenObject(Obj* obj) {
        switch (obj->type) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = (ObjBoundMethod*)obj;
                markValue(bound->receiver);
                markObject((Obj*)bound->method);
                break;
            }
            case OBJ_CLASS: {
                ObjClass* klass = (ObjClass*)obj;
                markObject((Obj*)klass->name);
                markTable(klass->methods); // Keep methods alive
                if (klass->superclass) markObject((Obj*)klass->superclass);
                break;
            }
            case OBJ_CLOSURE: {
                ObjClosure* closure = (ObjClosure*)obj;
                markObject((Obj*)closure->function);
                for (int i = 0; i < closure->upvalueCount; i++) {
                    markObject((Obj*)closure->upvalues[i]);
                }
                break;
            }
            case OBJ_FUNCTION: {
                ObjFunction* function = (ObjFunction*)obj;
                markObject((Obj*)function->name);
                for (size_t i = 0; i < function->chunk.constants.size(); i++) {
                    markValue(function->chunk.constants[i]);
                }
                break;
            }
            case OBJ_INSTANCE: {
                ObjInstance* instance = (ObjInstance*)obj;
                markObject((Obj*)instance->klass);
                markTable(instance->fields);
                break;
            }
            case OBJ_UPVALUE:
                markValue(((ObjUpvalue*)obj)->closed);
                break;
            case OBJ_NATIVE:
            case OBJ_STRING:
                break;
        }
    }

    void VM::sweep() {
        // Remove weak references from string table first
        for (int i = 0; i < strings.capacity; i++) {
            Entry* entry = &strings.entries[i];
            if (entry->key != nullptr && !entry->key->isMarked) {
                strings.deleteEntry(entry->key);
            }
        }

        Obj* previous = nullptr;
        Obj* object = objects;
        while (object != nullptr) {
            if (object->isMarked) {
                object->isMarked = false;
                previous = object;
                object = object->next;
            } else {
                Obj* unreached = object;
                object = object->next;
                if (previous != nullptr) {
                    previous->next = object;
                } else {
                    objects = object;
                }
                freeObject(unreached);
            }
        }
    }
}
