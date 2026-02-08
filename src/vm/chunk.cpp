#include "chunk.h"
#include <iostream>
#include <iomanip>

namespace cxxx {

    Chunk::Chunk() {}
    Chunk::~Chunk() {}

    void Chunk::write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    int Chunk::addConstant(Value value) {
        constants.push_back(value);
        return constants.size() - 1;
    }

    void Chunk::disassemble(const char* name) {
        std::cout << "== " << name << " ==" << std::endl;
        for (int offset = 0; offset < code.size();) {
            offset = disassembleInstruction(offset);
        }
    }

    int Chunk::disassembleInstruction(int offset) {
        std::cout << std::setw(4) << std::setfill('0') << offset << " ";

        if (offset > 0 && lines[offset] == lines[offset - 1]) {
            std::cout << "   | ";
        } else {
            std::cout << std::setw(4) << lines[offset] << " ";
        }

        uint8_t instruction = code[offset];
        switch (instruction) {
            case OP_CONSTANT:
                {
                    uint8_t constant = code[offset + 1];
                    std::cout << std::left << std::setw(16) << "OP_CONSTANT" << (int)constant << " '";
                    printValue(constants[constant]);
                    std::cout << "'" << std::endl;
                    return offset + 2;
                }
            case OP_RETURN:
                std::cout << "OP_RETURN" << std::endl;
                return offset + 1;
            case OP_NEGATE:
                std::cout << "OP_NEGATE" << std::endl;
                return offset + 1;
            case OP_ADD:
                std::cout << "OP_ADD" << std::endl;
                return offset + 1;
            case OP_SUBTRACT:
                std::cout << "OP_SUBTRACT" << std::endl;
                return offset + 1;
            case OP_MULTIPLY:
                std::cout << "OP_MULTIPLY" << std::endl;
                return offset + 1;
            case OP_DIVIDE:
                std::cout << "OP_DIVIDE" << std::endl;
                return offset + 1;
            case OP_NOT:
                std::cout << "OP_NOT" << std::endl;
                return offset + 1;
            case OP_EQUAL:
                std::cout << "OP_EQUAL" << std::endl;
                return offset + 1;
            case OP_GREATER:
                std::cout << "OP_GREATER" << std::endl;
                return offset + 1;
            case OP_LESS:
                std::cout << "OP_LESS" << std::endl;
                return offset + 1;
            case OP_JUMP:
                {
                    uint16_t jump = (uint16_t)((code[offset + 1] << 8) | code[offset + 2]);
                    std::cout << std::left << std::setw(16) << "OP_JUMP" << offset << " -> " << offset + 3 + jump << std::endl;
                    return offset + 3;
                }
            case OP_JUMP_IF_FALSE:
                {
                    uint16_t jump = (uint16_t)((code[offset + 1] << 8) | code[offset + 2]);
                    std::cout << std::left << std::setw(16) << "OP_JUMP_IF_FALSE" << offset << " -> " << offset + 3 + jump << std::endl;
                    return offset + 3;
                }
            case OP_POP:
                std::cout << "OP_POP" << std::endl;
                return offset + 1;
            case OP_DEFINE_GLOBAL:
                {
                    uint8_t constant = code[offset + 1];
                    std::cout << std::left << std::setw(16) << "OP_DEFINE_GLOBAL" << (int)constant << " '";
                    printValue(constants[constant]);
                    std::cout << "'" << std::endl;
                    return offset + 2;
                }
            case OP_GET_GLOBAL:
                {
                    uint8_t constant = code[offset + 1];
                    std::cout << std::left << std::setw(16) << "OP_GET_GLOBAL" << (int)constant << " '";
                    printValue(constants[constant]);
                    std::cout << "'" << std::endl;
                    return offset + 2;
                }
            case OP_SET_GLOBAL:
                {
                    uint8_t constant = code[offset + 1];
                    std::cout << std::left << std::setw(16) << "OP_SET_GLOBAL" << (int)constant << " '";
                    printValue(constants[constant]);
                    std::cout << "'" << std::endl;
                    return offset + 2;
                }
            case OP_CALL:
                {
                    uint8_t argCount = code[offset + 1];
                    std::cout << std::left << std::setw(16) << "OP_CALL" << (int)argCount << std::endl;
                    return offset + 2;
                }
            case OP_PRINT:
                std::cout << "OP_PRINT" << std::endl;
                return offset + 1;
            default:
                std::cout << "Unknown opcode " << (int)instruction << std::endl;
                return offset + 1;
        }
    }
}
