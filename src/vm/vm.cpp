#include "vm.h"
#include <iostream>

namespace cxxx {

    VM::VM() : globals(), strings() {
        resetStack();
        frameCount = 0;
    }

    VM::~VM() {
        free();
    }

    void VM::init() {
        resetStack();
        frameCount = 0;
    }

    void VM::free() {
        // Nothing to free yet
    }

    void VM::resetStack() {
        stackTop = stack;
        frameCount = 0;
    }

    void VM::push(Value value) {
        if (stackTop - stack >= STACK_MAX) {
            std::cerr << "Stack overflow!" << std::endl;
            // In robust impl, we should signal error.
            return;
        }
        *stackTop = value;
        stackTop++;
    }

    Value VM::pop() {
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
        push(OBJ_VAL((Obj*)function));
        CallFrame* frame = &frames[frameCount++];
        frame->function = function;
        frame->ip = function->chunk.code.data();
        frame->slots = stack;

        return run();
    }

    bool isFalsey(Value value) {
        return value.isNil() || (value.isBool() && !value.asBool());
    }

    InterpretResult VM::run() {
        CallFrame* frame = &frames[frameCount - 1];

        #define READ_BYTE() (*frame->ip++)
        #define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
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
                frame->function->chunk.disassembleInstruction((int)(frame->ip - frame->function->chunk.code.data()));
            #endif

            uint8_t instruction;
            switch (instruction = READ_BYTE()) {
                case OP_CONSTANT: {
                    Value constant = READ_CONSTANT();
                    push(constant);
                    break;
                }
                case OP_ADD: {
                    double b = pop().asNumber();
                    double a = pop().asNumber();
                    push(NUMBER_VAL(a + b));
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
                case OP_DEFINE_GLOBAL: {
                    ObjString* name = READ_STRING();
                    // std::cout << "VM: Defining global " << name->str << std::endl;
                    globals.set(name, peek(0));
                    pop();
                    break;
                }
                case OP_GET_GLOBAL: {
                    ObjString* name = READ_STRING();
                    Value value;
                    if (!globals.get(name, &value)) {
                        // runtime error
                        std::cerr << "Undefined variable '" << name->str << "'." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    push(value);
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
                case OP_CALL: {
                    int argCount = READ_BYTE();
                    Value callee = peek(argCount);
                    if (isObjType(callee, OBJ_FUNCTION)) {
                        ObjFunction* function = (ObjFunction*)callee.as.obj;
                        if (argCount != function->arity) {
                            std::cerr << "Expected " << function->arity << " arguments but got " << argCount << "." << std::endl;
                            return InterpretResult::RUNTIME_ERROR;
                        }
                        if (frameCount == FRAMES_MAX) {
                            std::cerr << "Stack overflow." << std::endl;
                            return InterpretResult::RUNTIME_ERROR;
                        }
                        CallFrame* newFrame = &frames[frameCount++];
                        newFrame->function = function;
                        newFrame->ip = function->chunk.code.data();
                        newFrame->slots = stackTop - argCount - 1;
                        frame = newFrame; // Switch to new frame
                        break;
                    }
                    if (isObjType(callee, OBJ_NATIVE)) {
                        NativeFn native = ((ObjNative*)callee.as.obj)->function;
                        Value result = native(argCount, stackTop - argCount);
                        stackTop -= argCount + 1; // Pop args and callee
                        push(result);
                        break;
                    }
                    std::cerr << "Can only call functions and classes." << std::endl;
                    return InterpretResult::RUNTIME_ERROR;
                }
                case OP_PRINT: {
                    printValue(pop());
                    std::cout << std::endl;
                    break;
                }
                case OP_NEGATE: {
                    // optimization: modify in place
                    // if (!peek(0).isNumber()) return RUNTIME_ERROR;
                    push(NUMBER_VAL(-pop().asNumber()));
                    break;
                }
                case OP_RETURN: {
                    Value result = pop();
                    frameCount--;
                    if (frameCount == 0) {
                        pop(); // Pop main script function
                        // Keep result on stack for API/Test access
                        push(result);
                        return InterpretResult::OK;
                    }

                    stackTop = frame->slots; // Discard locals
                    push(result);
                    frame = &frames[frameCount - 1]; // Restore caller frame
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

}
