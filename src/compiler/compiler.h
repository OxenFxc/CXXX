#ifndef cxxx_compiler_h
#define cxxx_compiler_h

#include "../vm/chunk.h"
#include "../vm/table.h"
#include "../vm/object.h"
#include <string>

namespace cxxx {

    ObjFunction* compile(const std::string& source, Table* internTable);

}

#endif
