#include "vm.h"
#include <iostream>

namespace cxxx {

    VM::VM() : globals(), strings() {
        resetStack();
        chunk = nullptr;
        ip = nullptr;
    }

    VM::~VM() {
        free();
    }

    void VM::init() {
        resetStack();
        chunk = nullptr;
        ip = nullptr;
    }

    void VM::free() {
        // Nothing to free yet
    }

    void VM::resetStack() {
        stackTop = stack;
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

    InterpretResult VM::interpret(Chunk* chunk) {
        this->chunk = chunk;
        this->ip = chunk->code.data();
        return run();
    }

    InterpretResult VM::run() {
        // Simple placeholder for step 3
        // In next step, I will add the loop.
        // For now, I can assume it returns OK.
        // But to test stack, I might need it to do something.
        // Let's implement OP_RETURN at least.

        #define READ_BYTE() (*ip++)
        #define READ_CONSTANT() (chunk->constants[READ_BYTE()])
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
                chunk->disassembleInstruction((int)(ip - chunk->code.data()));
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
                case OP_POP: {
                    pop();
                    break;
                }
                case OP_DEFINE_GLOBAL: {
                    ObjString* name = READ_STRING();
                    std::cout << "VM: Defining global " << name->str << std::endl;
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
                    if (!isObjType(callee, OBJ_NATIVE)) {
                        std::cerr << "Can only call native functions." << std::endl;
                        return InterpretResult::RUNTIME_ERROR;
                    }
                    NativeFn native = ((ObjNative*)callee.as.obj)->function;
                    Value result = native(argCount, stackTop - argCount);
                    stackTop -= argCount + 1; // Pop args and callee
                    push(result);
                    break;
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
                    // For now, pop result.
                    // printValue(pop());
                    // std::cout << std::endl;
                    return InterpretResult::OK;
                }
                default:
                    return InterpretResult::RUNTIME_ERROR;
            }
        }

        #undef READ_BYTE
        #undef READ_CONSTANT
    }

}
