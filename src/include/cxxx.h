#ifndef cxxx_h
#define cxxx_h

#include <string>
#include <cstdint>

namespace cxxx {

    enum class InterpretResult {
        OK,
        COMPILE_ERROR,
        RUNTIME_ERROR
    };

    enum ValueType {
        VAL_BOOL,
        VAL_NIL,
        VAL_NUMBER,
        VAL_OBJ
    };

    // Forward declaration for Object (opaque to user)
    struct Obj;

    struct Value {
        ValueType type;
        union {
            bool boolean;
            double number;
            Obj* obj;
        } as;

        bool isBool() const { return type == VAL_BOOL; }
        bool isNil() const { return type == VAL_NIL; }
        bool isNumber() const { return type == VAL_NUMBER; }
        bool isObj() const { return type == VAL_OBJ; }

        double asNumber() const { return as.number; }
        bool asBool() const { return as.boolean; }
        Obj* asObj() const { return as.obj; }

        static Value boolean(bool value) {
            Value v;
            v.type = VAL_BOOL;
            v.as.boolean = value;
            return v;
        }

        static Value nil() {
            Value v;
            v.type = VAL_NIL;
            v.as.number = 0;
            return v;
        }

        static Value number(double value) {
            Value v;
            v.type = VAL_NUMBER;
            v.as.number = value;
            return v;
        }

        static Value object(Obj* object) {
            Value v;
            v.type = VAL_OBJ;
            v.as.obj = object;
            return v;
        }
    };

    // Native function pointer type
    // We pass void* vm to allow native functions to allocate objects.
    typedef Value (*NativeFn)(void* vm, int argCount, Value* args);

    class CXXX {
    public:
        CXXX();
        ~CXXX();

        InterpretResult interpret(const std::string& source);

        // For testing/debugging, return the last computation result as double.
        // Returns 0.0 if not a number or stack empty.
        double getResult();

        // Get global variable value
        double getGlobalNumber(const std::string& name);
        bool getGlobalBool(const std::string& name);

        // Set global variable value
        void setGlobal(const std::string& name, Value val);

        // Helper to create a string value (interned)
        Value createString(const std::string& s);

        void registerFunction(const char* name, NativeFn fn);

        // Internal: load stdlib
        void loadStdLib();

    private:
        void* vm; // Opaque pointer to internal VM
        double lastResult;
    };

}

#endif
