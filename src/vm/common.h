#ifndef cxxx_common_h
#define cxxx_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace cxxx {
    // Forward declare Value
    struct Value;

    // Native function pointer type
    // Takes arg count and arg array. returns Value.
    typedef Value (*NativeFn)(int argCount, Value* args);
}

#endif
