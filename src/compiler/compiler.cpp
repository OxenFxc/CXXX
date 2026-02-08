#include "compiler.h"
#include "scanner.h"
#include "../vm/object.h"
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

    typedef void (*ParseFn)(CompilerInstance*, bool canAssign);

    struct ParseRule {
        ParseFn prefix;
        ParseFn infix;
        Precedence precedence;
    };

    struct CompilerInstance {
        Scanner* scanner;
        Chunk* compilingChunk;
        Table* internTable;
        Parser parser;
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

    bool match(CompilerInstance* compiler, TokenType type) {
        if (compiler->parser.current.type != type) return false;
        advance(compiler);
        return true;
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

    uint8_t makeConstant(CompilerInstance* compiler, Value value) {
        int constant = currentChunk(compiler)->addConstant(value);
        if (constant > 255) {
            error(compiler, "Too many constants in one chunk.");
            return 0;
        }
        return (uint8_t)constant;
    }

    void emitConstant(CompilerInstance* compiler, Value value) {
        emitBytes(compiler, OP_CONSTANT, makeConstant(compiler, value));
    }

    uint8_t identifierConstant(CompilerInstance* compiler, Token* name, Table* internTable) {
        ObjString* string = copyString(name->start, name->length, internTable);
        return makeConstant(compiler, OBJ_VAL((Obj*)string));
    }

    // Forward declarations
    void expression(CompilerInstance* compiler);
    void parsePrecedence(CompilerInstance* compiler, Precedence precedence);
    void variable(CompilerInstance* compiler, bool canAssign);

    uint8_t argumentList(CompilerInstance* compiler) {
        uint8_t argCount = 0;
        if (!match(compiler, TOKEN_RIGHT_PAREN)) {
            do {
                expression(compiler);
                if (argCount == 255) {
                    error(compiler, "Can't have more than 255 arguments.");
                }
                argCount++;
            } while (match(compiler, TOKEN_COMMA));
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
        }
        return argCount;
    }

    void call(CompilerInstance* compiler, bool canAssign) {
        uint8_t argCount = argumentList(compiler);
        emitBytes(compiler, OP_CALL, argCount);
    }

    void namedVariable(CompilerInstance* compiler, Token name, bool canAssign) {
        uint8_t arg = identifierConstant(compiler, &name, compiler->internTable);

        if (canAssign && match(compiler, TOKEN_EQUAL)) {
            expression(compiler);
            emitBytes(compiler, OP_SET_GLOBAL, arg);
        } else {
            emitBytes(compiler, OP_GET_GLOBAL, arg);
        }
    }

    void variable(CompilerInstance* compiler, bool canAssign) {
        namedVariable(compiler, compiler->parser.previous, canAssign);
    }

    void binary(CompilerInstance* compiler, bool canAssign) {
        TokenType operatorType = compiler->parser.previous.type;
        ParseRule* rule = getRule(operatorType);
        parsePrecedence(compiler, (Precedence)(rule->precedence + 1));

        switch (operatorType) {
            case TOKEN_BANG_EQUAL:    emitBytes(compiler, OP_EQUAL, OP_NOT); break;
            case TOKEN_EQUAL_EQUAL:   emitByte(compiler, OP_EQUAL); break;
            case TOKEN_GREATER:       emitByte(compiler, OP_GREATER); break;
            case TOKEN_GREATER_EQUAL: emitBytes(compiler, OP_LESS, OP_NOT); break;
            case TOKEN_LESS:          emitByte(compiler, OP_LESS); break;
            case TOKEN_LESS_EQUAL:    emitBytes(compiler, OP_GREATER, OP_NOT); break;
            case TOKEN_PLUS:          emitByte(compiler, OP_ADD); break;
            case TOKEN_MINUS:         emitByte(compiler, OP_SUBTRACT); break;
            case TOKEN_STAR:          emitByte(compiler, OP_MULTIPLY); break;
            case TOKEN_SLASH:         emitByte(compiler, OP_DIVIDE); break;
            default: return; // Unreachable.
        }
    }

    void grouping(CompilerInstance* compiler, bool canAssign) {
        expression(compiler);
        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }

    void number(CompilerInstance* compiler, bool canAssign) {
        std::string s(compiler->parser.previous.start, compiler->parser.previous.length);
        double value = std::stod(s);
        emitConstant(compiler, NUMBER_VAL(value));
    }

    void literal(CompilerInstance* compiler, bool canAssign) {
        switch (compiler->parser.previous.type) {
            case TOKEN_FALSE: emitConstant(compiler, BOOL_VAL(false)); break;
            case TOKEN_NIL:   emitConstant(compiler, NIL_VAL()); break;
            case TOKEN_TRUE:  emitConstant(compiler, BOOL_VAL(true)); break;
            default: return; // Unreachable.
        }
    }

    void unary(CompilerInstance* compiler, bool canAssign) {
        TokenType operatorType = compiler->parser.previous.type;

        // Compile the operand.
        parsePrecedence(compiler, PREC_UNARY);

        // Emit the operator instruction.
        switch (operatorType) {
            case TOKEN_BANG:  emitByte(compiler, OP_NOT); break;
            case TOKEN_MINUS: emitByte(compiler, OP_NEGATE); break;
            default: return; // Unreachable.
        }
    }

    // Forward declare call
    void call(CompilerInstance* compiler, bool canAssign);

    ParseRule rules[] = {
        {grouping, call,   PREC_CALL},       // TOKEN_LEFT_PAREN
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
        {unary,    NULL,   PREC_NONE},       // TOKEN_BANG
        {NULL,     binary, PREC_EQUALITY},   // TOKEN_BANG_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_EQUAL
        {NULL,     binary, PREC_EQUALITY},   // TOKEN_EQUAL_EQUAL
        {NULL,     binary, PREC_COMPARISON}, // TOKEN_GREATER
        {NULL,     binary, PREC_COMPARISON}, // TOKEN_GREATER_EQUAL
        {NULL,     binary, PREC_COMPARISON}, // TOKEN_LESS
        {NULL,     binary, PREC_COMPARISON}, // TOKEN_LESS_EQUAL
        {variable, NULL,   PREC_NONE},       // TOKEN_IDENTIFIER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_STRING
        {number,   NULL,   PREC_NONE},       // TOKEN_NUMBER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_AND
        {NULL,     NULL,   PREC_NONE},       // TOKEN_CLASS
        {NULL,     NULL,   PREC_NONE},       // TOKEN_ELSE
        {literal,  NULL,   PREC_NONE},       // TOKEN_FALSE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FOR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FUN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_IF
        {literal,  NULL,   PREC_NONE},       // TOKEN_NIL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_OR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_PRINT
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RETURN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_SUPER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_THIS
        {literal,  NULL,   PREC_NONE},       // TOKEN_TRUE
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

        bool canAssign = precedence <= PREC_ASSIGNMENT;
        prefixRule(compiler, canAssign);

        while (precedence <= getRule(compiler->parser.current.type)->precedence) {
            advance(compiler);
            ParseFn infixRule = getRule(compiler->parser.previous.type)->infix;
            infixRule(compiler, canAssign);
        }

        if (canAssign && match(compiler, TOKEN_EQUAL)) {
             error(compiler, "Invalid assignment target.");
        }
    }

    void expression(CompilerInstance* compiler) {
        parsePrecedence(compiler, PREC_ASSIGNMENT);
    }

    uint8_t parseVariable(CompilerInstance* compiler, const char* errorMessage) {
        consume(compiler, TOKEN_IDENTIFIER, errorMessage);
        return identifierConstant(compiler, &compiler->parser.previous, compiler->internTable);
    }

    void defineVariable(CompilerInstance* compiler, uint8_t global) {
        emitBytes(compiler, OP_DEFINE_GLOBAL, global);
    }

    void varDeclaration(CompilerInstance* compiler) {
        uint8_t global = parseVariable(compiler, "Expect variable name.");

        if (match(compiler, TOKEN_EQUAL)) {
            expression(compiler);
        } else {
            emitConstant(compiler, NIL_VAL());
        }
        consume(compiler, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

        defineVariable(compiler, global);
    }

    void expressionStatement(CompilerInstance* compiler) {
        expression(compiler);
        consume(compiler, TOKEN_SEMICOLON, "Expect ';' after expression.");
        emitByte(compiler, OP_POP);
    }

    void printStatement(CompilerInstance* compiler) {
        expression(compiler);
        consume(compiler, TOKEN_SEMICOLON, "Expect ';' after value.");
        emitByte(compiler, OP_PRINT);
    }

    int emitJump(CompilerInstance* compiler, uint8_t instruction) {
        emitByte(compiler, instruction);
        emitByte(compiler, 0xff);
        emitByte(compiler, 0xff);
        return currentChunk(compiler)->code.size() - 2;
    }

    void patchJump(CompilerInstance* compiler, int offset) {
        // -2 to adjust for the bytecode for the jump offset itself.
        int jump = currentChunk(compiler)->code.size() - offset - 2;

        if (jump > UINT16_MAX) {
            error(compiler, "Too much code to jump over.");
        }

        currentChunk(compiler)->code[offset] = (jump >> 8) & 0xff;
        currentChunk(compiler)->code[offset + 1] = jump & 0xff;
    }

    void declaration(CompilerInstance* compiler);
    void statement(CompilerInstance* compiler);

    bool check(CompilerInstance* compiler, TokenType type) {
        return compiler->parser.current.type == type;
    }

    void block(CompilerInstance* compiler) {
        while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
            declaration(compiler);
        }
        consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    }

    void ifStatement(CompilerInstance* compiler) {
        consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        expression(compiler);
        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int thenJump = emitJump(compiler, OP_JUMP_IF_FALSE);
        emitByte(compiler, OP_POP);
        statement(compiler);

        int elseJump = emitJump(compiler, OP_JUMP);

        patchJump(compiler, thenJump);
        emitByte(compiler, OP_POP);

        if (match(compiler, TOKEN_ELSE)) {
            statement(compiler);
        }
        patchJump(compiler, elseJump);
    }

    void statement(CompilerInstance* compiler) {
        if (match(compiler, TOKEN_PRINT)) {
            printStatement(compiler);
        } else if (match(compiler, TOKEN_IF)) {
            ifStatement(compiler);
        } else if (match(compiler, TOKEN_LEFT_BRACE)) {
            block(compiler);
        } else {
            expressionStatement(compiler);
        }
    }

    void declaration(CompilerInstance* compiler) {
        if (match(compiler, TOKEN_VAR)) {
            varDeclaration(compiler);
        } else {
            statement(compiler);
        }

        if (compiler->parser.panicMode) {
             // synchronize
             compiler->parser.panicMode = false;
             while (compiler->parser.current.type != TOKEN_EOF) {
                 if (compiler->parser.previous.type == TOKEN_SEMICOLON) return;
                 switch (compiler->parser.current.type) {
                     case TOKEN_CLASS:
                     case TOKEN_FUN:
                     case TOKEN_VAR:
                     case TOKEN_FOR:
                     case TOKEN_IF:
                     case TOKEN_WHILE:
                     case TOKEN_PRINT:
                     case TOKEN_RETURN:
                         return;
                     default:
                         ; // Do nothing.
                 }
                 advance(compiler);
             }
        }
    }

    bool compile(const std::string& source, Chunk* chunk, Table* internTable) {
        Scanner scanner(source.c_str());
        CompilerInstance compiler;
        compiler.scanner = &scanner;
        compiler.compilingChunk = chunk;
        compiler.internTable = internTable;
        compiler.parser.hadError = false;
        compiler.parser.panicMode = false;

        advance(&compiler);

        while (!match(&compiler, TOKEN_EOF)) {
            declaration(&compiler);
        }

        emitReturn(&compiler);

        #ifdef DEBUG_PRINT_CODE
        if (!compiler.parser.hadError) {
            chunk->disassemble("code");
        }
        #endif

        return !compiler.parser.hadError;
    }
}
