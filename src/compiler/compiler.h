#ifndef cxxx_compiler_h
#define cxxx_compiler_h

#include "../vm/chunk.h"
#include <string>

namespace cxxx {

    bool compile(const std::string& source, Chunk* chunk);

}

#endif
