#include "compiler.h"
#include "scanner.h"
#include <iostream>
#include <cstdlib>
#include <vector>

namespace cxxx {

    struct Parser {
        Token current;
        Token previous;
        bool hadError;
        bool panicMode;
    };

    enum Precedence {
        PREC_NONE,
        PREC_ASSIGNMENT,  // =
        PREC_OR,          // or
        PREC_AND,         // and
        PREC_EQUALITY,    // == !=
        PREC_COMPARISON,  // < > <= >=
        PREC_TERM,        // + -
        PREC_FACTOR,      // * /
        PREC_UNARY,       // ! -
        PREC_CALL,        // . ()
        PREC_PRIMARY
    };

    struct CompilerInstance;

    typedef void (*ParseFn)(CompilerInstance*);

    struct ParseRule {
        ParseFn prefix;
        ParseFn infix;
        Precedence precedence;
    };

    struct CompilerInstance {
        Parser parser;
        Scanner* scanner;
        Chunk* compilingChunk;
    };

    ParseRule* getRule(TokenType type);

    Chunk* currentChunk(CompilerInstance* compiler) {
        return compiler->compilingChunk;
    }

    void errorAt(CompilerInstance* compiler, Token* token, const char* message) {
        if (compiler->parser.panicMode) return;
        compiler->parser.panicMode = true;
        std::cerr << "[line " << token->line << "] Error";

        if (token->type == TOKEN_EOF) {
            std::cerr << " at end";
        } else if (token->type == TOKEN_ERROR) {
            // Nothing.
        } else {
            std::cerr << " at '";
            std::cerr << std::string(token->start, token->length);
            std::cerr << "'";
        }

        std::cerr << ": " << message << std::endl;
        compiler->parser.hadError = true;
    }

    void error(CompilerInstance* compiler, const char* message) {
        errorAt(compiler, &compiler->parser.previous, message);
    }

    void errorAtCurrent(CompilerInstance* compiler, const char* message) {
        errorAt(compiler, &compiler->parser.current, message);
    }

    void advance(CompilerInstance* compiler) {
        compiler->parser.previous = compiler->parser.current;

        for (;;) {
            compiler->parser.current = compiler->scanner->scanToken();
            if (compiler->parser.current.type != TOKEN_ERROR) break;

            errorAtCurrent(compiler, compiler->parser.current.start);
        }
    }

    void consume(CompilerInstance* compiler, TokenType type, const char* message) {
        if (compiler->parser.current.type == type) {
            advance(compiler);
            return;
        }
        errorAtCurrent(compiler, message);
    }

    void emitByte(CompilerInstance* compiler, uint8_t byte) {
        currentChunk(compiler)->write(byte, compiler->parser.previous.line);
    }

    void emitBytes(CompilerInstance* compiler, uint8_t byte1, uint8_t byte2) {
        emitByte(compiler, byte1);
        emitByte(compiler, byte2);
    }

    void emitReturn(CompilerInstance* compiler) {
        emitByte(compiler, OP_RETURN);
    }

    void emitConstant(CompilerInstance* compiler, Value value) {
        emitBytes(compiler, OP_CONSTANT, (uint8_t)currentChunk(compiler)->addConstant(value));
    }

    // Forward declarations
    void expression(CompilerInstance* compiler);
    void parsePrecedence(CompilerInstance* compiler, Precedence precedence);

    void binary(CompilerInstance* compiler) {
        TokenType operatorType = compiler->parser.previous.type;
        ParseRule* rule = getRule(operatorType);
        parsePrecedence(compiler, (Precedence)(rule->precedence + 1));

        switch (operatorType) {
            case TOKEN_PLUS: emitByte(compiler, OP_ADD); break;
            case TOKEN_MINUS: emitByte(compiler, OP_SUBTRACT); break;
            case TOKEN_STAR: emitByte(compiler, OP_MULTIPLY); break;
            case TOKEN_SLASH: emitByte(compiler, OP_DIVIDE); break;
            default: return; // Unreachable.
        }
    }

    void grouping(CompilerInstance* compiler) {
        expression(compiler);
        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }

    void number(CompilerInstance* compiler) {
        // null-terminate the token string to use strtod, or use from_chars (C++17)
        // Since `start` is in a big buffer, we can't null terminate easily.
        // We'll create a temp string.
        std::string s(compiler->parser.previous.start, compiler->parser.previous.length);
        double value = std::stod(s);
        emitConstant(compiler, NUMBER_VAL(value));
    }

    void unary(CompilerInstance* compiler) {
        TokenType operatorType = compiler->parser.previous.type;

        // Compile the operand.
        parsePrecedence(compiler, PREC_UNARY);

        // Emit the operator instruction.
        switch (operatorType) {
            case TOKEN_MINUS: emitByte(compiler, OP_NEGATE); break;
            default: return; // Unreachable.
        }
    }

    ParseRule rules[] = {
        {grouping, NULL,   PREC_NONE},       // TOKEN_LEFT_PAREN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RIGHT_PAREN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_LEFT_BRACE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RIGHT_BRACE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_COMMA
        {NULL,     NULL,   PREC_NONE},       // TOKEN_DOT
        {unary,    binary, PREC_TERM},       // TOKEN_MINUS
        {NULL,     binary, PREC_TERM},       // TOKEN_PLUS
        {NULL,     NULL,   PREC_NONE},       // TOKEN_SEMICOLON
        {NULL,     binary, PREC_FACTOR},     // TOKEN_SLASH
        {NULL,     binary, PREC_FACTOR},     // TOKEN_STAR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_BANG
        {NULL,     NULL,   PREC_NONE},       // TOKEN_BANG_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_EQUAL_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_GREATER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_GREATER_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_LESS
        {NULL,     NULL,   PREC_NONE},       // TOKEN_LESS_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_IDENTIFIER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_STRING
        {number,   NULL,   PREC_NONE},       // TOKEN_NUMBER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_AND
        {NULL,     NULL,   PREC_NONE},       // TOKEN_CLASS
        {NULL,     NULL,   PREC_NONE},       // TOKEN_ELSE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FALSE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FOR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FUN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_IF
        {NULL,     NULL,   PREC_NONE},       // TOKEN_NIL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_OR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_PRINT
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RETURN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_SUPER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_THIS
        {NULL,     NULL,   PREC_NONE},       // TOKEN_TRUE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_VAR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_WHILE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_ERROR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_EOF
    };

    ParseRule* getRule(TokenType type) {
        return &rules[type];
    }

    void parsePrecedence(CompilerInstance* compiler, Precedence precedence) {
        advance(compiler);
        ParseFn prefixRule = getRule(compiler->parser.previous.type)->prefix;
        if (prefixRule == NULL) {
            error(compiler, "Expect expression.");
            return;
        }

        prefixRule(compiler);

        while (precedence <= getRule(compiler->parser.current.type)->precedence) {
            advance(compiler);
            ParseFn infixRule = getRule(compiler->parser.previous.type)->infix;
            infixRule(compiler);
        }
    }

    void expression(CompilerInstance* compiler) {
        parsePrecedence(compiler, PREC_ASSIGNMENT);
    }

    bool compile(const std::string& source, Chunk* chunk) {
        Scanner scanner(source.c_str());
        CompilerInstance compiler;
        compiler.scanner = &scanner;
        compiler.compilingChunk = chunk;
        compiler.parser.hadError = false;
        compiler.parser.panicMode = false;

        advance(&compiler);
        expression(&compiler);
        consume(&compiler, TOKEN_EOF, "Expect end of expression.");

        emitReturn(&compiler);

        #ifdef DEBUG_PRINT_CODE
        if (!compiler.parser.hadError) {
            chunk->disassemble("code");
        }
        #endif

        return !compiler.parser.hadError;
    }
}
