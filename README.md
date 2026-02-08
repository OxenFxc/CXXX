# CXXX

A high-performance, modular, embeddable scripting language written in C++17.
Inspired by Lua's architecture but using modern C++.

## Features

- **Modular Architecture**: Decoupled VM, Compiler, and Public API.
- **High Performance**:
  - Efficient bytecode VM.
  - Compact `Value` representation (Tagged Union).
  - Single-pass compiler.
- **Embeddable**: Simple C++ API (`cxxx::CXXX`).
- **Cross-Platform**: CMake build system.

## Project Structure

- `src/include/`: Public API headers.
- `src/vm/`: The virtual machine core (stack, opcodes, value).
- `src/compiler/`: The compiler (lexer, parser, code generator).
- `src/cli/`: The command-line interface (REPL and file runner).
- `tests/`: Unit tests.

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

Interactive REPL:
```bash
./cxxx
```

Run a script:
```bash
./cxxx script.cxxx
```

## Embedding

```cpp
#include <cxxx.h>

cxxx::CXXX vm;
cxxx::InterpretResult result = vm.interpret("print(1 + 2);");
```
