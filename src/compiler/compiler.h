#ifndef cxxx_compiler_h
#define cxxx_compiler_h

#include "../vm/chunk.h"
#include "../vm/table.h"
#include <string>

// #error "INCLUDED CORRECTLY"

namespace cxxx {

    bool compile(const std::string& source, Chunk* chunk, Table* internTable);

}

#endif
