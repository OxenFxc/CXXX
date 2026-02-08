#ifndef cxxx_h
#define cxxx_h

#include <string>

namespace cxxx {

    enum class InterpretResult {
        OK,
        COMPILE_ERROR,
        RUNTIME_ERROR
    };

    struct Value;

    class CXXX {
    public:
        typedef struct Value (*NativeFn)(int argCount, struct Value* args);

        CXXX();
        ~CXXX();

        InterpretResult interpret(const std::string& source);

        // For testing/debugging, return the last computation result as double.
        // Returns 0.0 if not a number or stack empty.
        double getResult();

        void registerFunction(const char* name, NativeFn fn);

        // Internal: load stdlib
        void loadStdLib();

    private:
        void* vm; // Opaque pointer to internal VM
        double lastResult;
    };

}

#endif
