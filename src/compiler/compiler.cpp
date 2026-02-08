#include "compiler.h"
#include "scanner.h"
#include "../vm/object.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
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
        PREC_TERNARY,     // ? :
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

    struct Local {
        Token name;
        int depth;
        bool isCaptured;
    };

    struct Upvalue {
        uint8_t index;
        bool isLocal;
    };

    enum FunctionType {
        TYPE_FUNCTION,
        TYPE_SCRIPT,
        TYPE_METHOD,
        TYPE_INITIALIZER
    };

    struct Loop {
        Loop* enclosing;
        int start;
        int scopeDepth;
        bool isLoop;
        std::vector<int> breakJumps;
    };

    struct Compiler {
        struct Compiler* enclosing;
        ObjFunction* function;
        FunctionType type;

        Local locals[256];
        int localCount;
        Upvalue upvalues[256];
        int upvalueCount;
        int scopeDepth;
        Loop* loop;
    };

    struct ClassCompiler {
        struct ClassCompiler* enclosing;
        bool hasSuperclass;
    };

    struct CompilerInstance {
        Scanner* scanner;
        Table* internTable;
        Parser parser;
        Compiler* compiler;
        ClassCompiler* currentClass;
    };

    ParseRule* getRule(TokenType type);

    Chunk* currentChunk(CompilerInstance* compiler) {
        return &compiler->compiler->function->chunk;
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

    Token syntheticToken(const char* text) {
        Token token;
        token.start = text;
        token.length = (int)strlen(text);
        return token;
    }

    void emitReturn(CompilerInstance* compiler) {
        if (compiler->compiler->type == TYPE_INITIALIZER) {
            emitBytes(compiler, OP_GET_LOCAL, 0);
        } else {
            emitConstant(compiler, NIL_VAL());
        }
        emitByte(compiler, OP_RETURN);
    }

    uint8_t identifierConstant(CompilerInstance* compiler, Token* name, Table* internTable) {
        ObjString* string = copyString(name->start, name->length, internTable);
        return makeConstant(compiler, OBJ_VAL((Obj*)string));
    }

    bool identifiersEqual(Token* a, Token* b) {
        if (a->length != b->length) return false;
        return memcmp(a->start, b->start, a->length) == 0;
    }

    void addLocal(CompilerInstance* compiler, Token name) {
        if (compiler->compiler->localCount == 256) {
            error(compiler, "Too many local variables in function.");
            return;
        }

        Local* local = &compiler->compiler->locals[compiler->compiler->localCount++];
        local->name = name;
        local->depth = -1;
        local->isCaptured = false;
        // std::string n(name.start, name.length);
        // std::cout << "DEBUG: Added local '" << n << "' at index " << compiler->compiler->localCount - 1 << std::endl;
    }

    void declareVariable(CompilerInstance* compiler) {
        if (compiler->compiler->scopeDepth == 0 && compiler->compiler->type == TYPE_SCRIPT) return;

        Token* name = &compiler->parser.previous;

        for (int i = compiler->compiler->localCount - 1; i >= 0; i--) {
            Local* local = &compiler->compiler->locals[i];
            if (local->depth != -1 && local->depth < compiler->compiler->scopeDepth) {
                break;
            }

            if (identifiersEqual(name, &local->name)) {
                error(compiler, "Already a variable with this name in this scope.");
            }
        }

        addLocal(compiler, *name);
    }

    void parseVariable(CompilerInstance* compiler, const char* errorMessage, bool& isLocal) {
        consume(compiler, TOKEN_IDENTIFIER, errorMessage);

        declareVariable(compiler);
        if (compiler->compiler->scopeDepth > 0) {
            isLocal = true;
            return; // 0;
        }

        isLocal = false;
        // return identifierConstant(compiler, &compiler->parser.previous, compiler->internTable);
    }

    void markInitialized(CompilerInstance* compiler) {
        if (compiler->compiler->scopeDepth == 0 && compiler->compiler->type == TYPE_SCRIPT) return;
        compiler->compiler->locals[compiler->compiler->localCount - 1].depth =
            compiler->compiler->scopeDepth;
    }

    int resolveLocal(CompilerInstance* compilerInstance, Compiler* compiler, Token* name) {
        for (int i = compiler->localCount - 1; i >= 0; i--) {
            Local* local = &compiler->locals[i];
            if (identifiersEqual(name, &local->name)) {
                if (local->depth == -1) {
                    error(compilerInstance, "Can't read local variable in its own initializer.");
                }
                return i;
            }
        }
        return -1;
    }

    int addUpvalue(CompilerInstance* compilerInstance, Compiler* compiler, uint8_t index, bool isLocal) {
        int upvalueCount = compiler->upvalueCount;

        for (int i = 0; i < upvalueCount; i++) {
            Upvalue* upvalue = &compiler->upvalues[i];
            if (upvalue->index == index && upvalue->isLocal == isLocal) {
                return i;
            }
        }

        if (upvalueCount == 256) {
            error(compilerInstance, "Too many closure variables in function.");
            return 0;
        }

        compiler->upvalues[upvalueCount].isLocal = isLocal;
        compiler->upvalues[upvalueCount].index = index;
        return compiler->upvalueCount++;
    }

    int resolveUpvalue(CompilerInstance* compilerInstance, Compiler* compiler, Token* name) {
        if (compiler->enclosing == nullptr) return -1;

        int local = resolveLocal(compilerInstance, compiler->enclosing, name);
        if (local != -1) {
            compiler->enclosing->locals[local].isCaptured = true;
            return addUpvalue(compilerInstance, compiler, (uint8_t)local, true);
        }

        int upvalue = resolveUpvalue(compilerInstance, compiler->enclosing, name);
        if (upvalue != -1) {
            return addUpvalue(compilerInstance, compiler, (uint8_t)upvalue, false);
        }

        return -1;
    }

    // Forward declarations
    void expression(CompilerInstance* compiler);
    void parsePrecedence(CompilerInstance* compiler, Precedence precedence);
    void variable(CompilerInstance* compiler, bool canAssign);
    int emitJump(CompilerInstance* compiler, uint8_t instruction);
    void patchJump(CompilerInstance* compiler, int offset);

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
        uint8_t getOp, setOp;
        int arg = resolveLocal(compiler, compiler->compiler, &name);
        if (arg != -1) {
            getOp = OP_GET_LOCAL;
            setOp = OP_SET_LOCAL;
        } else if ((arg = resolveUpvalue(compiler, compiler->compiler, &name)) != -1) {
            getOp = OP_GET_UPVALUE;
            setOp = OP_SET_UPVALUE;
        } else {
            arg = identifierConstant(compiler, &name, compiler->internTable);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        if (canAssign && match(compiler, TOKEN_EQUAL)) {
            expression(compiler);
            emitBytes(compiler, setOp, (uint8_t)arg);
        } else if (canAssign && match(compiler, TOKEN_PLUS_EQUAL)) {
             emitBytes(compiler, getOp, (uint8_t)arg);
             expression(compiler);
             emitByte(compiler, OP_ADD);
             emitBytes(compiler, setOp, (uint8_t)arg);
        } else if (canAssign && match(compiler, TOKEN_MINUS_EQUAL)) {
             emitBytes(compiler, getOp, (uint8_t)arg);
             expression(compiler);
             emitByte(compiler, OP_SUBTRACT);
             emitBytes(compiler, setOp, (uint8_t)arg);
        } else if (canAssign && match(compiler, TOKEN_STAR_EQUAL)) {
             emitBytes(compiler, getOp, (uint8_t)arg);
             expression(compiler);
             emitByte(compiler, OP_MULTIPLY);
             emitBytes(compiler, setOp, (uint8_t)arg);
        } else if (canAssign && match(compiler, TOKEN_SLASH_EQUAL)) {
             emitBytes(compiler, getOp, (uint8_t)arg);
             expression(compiler);
             emitByte(compiler, OP_DIVIDE);
             emitBytes(compiler, setOp, (uint8_t)arg);
        } else if (canAssign && match(compiler, TOKEN_PLUS_PLUS)) {
             emitBytes(compiler, getOp, (uint8_t)arg);
             emitBytes(compiler, getOp, (uint8_t)arg);
             emitConstant(compiler, NUMBER_VAL(1));
             emitByte(compiler, OP_ADD);
             emitBytes(compiler, setOp, (uint8_t)arg);
             emitByte(compiler, OP_POP);
        } else if (canAssign && match(compiler, TOKEN_MINUS_MINUS)) {
             emitBytes(compiler, getOp, (uint8_t)arg);
             emitBytes(compiler, getOp, (uint8_t)arg);
             emitConstant(compiler, NUMBER_VAL(1));
             emitByte(compiler, OP_SUBTRACT);
             emitBytes(compiler, setOp, (uint8_t)arg);
             emitByte(compiler, OP_POP);
        } else {
            emitBytes(compiler, getOp, (uint8_t)arg);
        }
    }

    void variable(CompilerInstance* compiler, bool canAssign) {
        namedVariable(compiler, compiler->parser.previous, canAssign);
    }

    void dot(CompilerInstance* compiler, bool canAssign) {
        consume(compiler, TOKEN_IDENTIFIER, "Expect property name.");
        uint8_t name = identifierConstant(compiler, &compiler->parser.previous, compiler->internTable);

        if (canAssign && match(compiler, TOKEN_EQUAL)) {
            expression(compiler);
            emitBytes(compiler, OP_SET_PROPERTY, name);
        } else if (match(compiler, TOKEN_LEFT_PAREN)) {
            uint8_t argCount = argumentList(compiler);
            emitBytes(compiler, OP_INVOKE, name);
            emitByte(compiler, argCount);
        } else {
            emitBytes(compiler, OP_GET_PROPERTY, name);
        }
    }

    void this_(CompilerInstance* compiler, bool canAssign) {
        if (compiler->currentClass == nullptr) {
            error(compiler, "Can't use 'this' outside of a class.");
            return;
        }
        variable(compiler, false);
    }

    void super_(CompilerInstance* compiler, bool canAssign) {
        if (compiler->currentClass == nullptr) {
            error(compiler, "Can't use 'super' outside of a class.");
        } else if (!compiler->currentClass->hasSuperclass) {
            error(compiler, "Can't use 'super' in a class with no superclass.");
        }

        consume(compiler, TOKEN_DOT, "Expect '.' after 'super'.");
        consume(compiler, TOKEN_IDENTIFIER, "Expect superclass method name.");
        uint8_t name = identifierConstant(compiler, &compiler->parser.previous, compiler->internTable);

        namedVariable(compiler, syntheticToken("this"), false);
        if (match(compiler, TOKEN_LEFT_PAREN)) {
            uint8_t argCount = argumentList(compiler);
            namedVariable(compiler, syntheticToken("super"), false);
            emitBytes(compiler, OP_SUPER_INVOKE, name);
            emitByte(compiler, argCount);
        } else {
            namedVariable(compiler, syntheticToken("super"), false);
            emitBytes(compiler, OP_GET_SUPER, name);
        }
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
            case TOKEN_INSTANCEOF:    emitByte(compiler, OP_INSTANCEOF); break;
            default: return; // Unreachable.
        }
    }

    void prefixMutate(CompilerInstance* compiler, bool canAssign) {
        TokenType operatorType = compiler->parser.previous.type;
        consume(compiler, TOKEN_IDENTIFIER, "Expect variable.");
        Token name = compiler->parser.previous;

        uint8_t getOp, setOp;
        int arg = resolveLocal(compiler, compiler->compiler, &name);
        if (arg != -1) {
            getOp = OP_GET_LOCAL;
            setOp = OP_SET_LOCAL;
        } else if ((arg = resolveUpvalue(compiler, compiler->compiler, &name)) != -1) {
            getOp = OP_GET_UPVALUE;
            setOp = OP_SET_UPVALUE;
        } else {
            arg = identifierConstant(compiler, &name, compiler->internTable);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        // ++i
        emitBytes(compiler, getOp, (uint8_t)arg);
        emitConstant(compiler, NUMBER_VAL(1));
        if (operatorType == TOKEN_PLUS_PLUS) {
            emitByte(compiler, OP_ADD);
        } else {
            emitByte(compiler, OP_SUBTRACT);
        }
        emitBytes(compiler, setOp, (uint8_t)arg);
    }

    void ternary(CompilerInstance* compiler, bool canAssign) {
        // Condition on stack.
        int thenJump = emitJump(compiler, OP_JUMP_IF_FALSE);
        emitByte(compiler, OP_POP);

        parsePrecedence(compiler, PREC_ASSIGNMENT);

        int elseJump = emitJump(compiler, OP_JUMP);

        patchJump(compiler, thenJump);
        emitByte(compiler, OP_POP);

        consume(compiler, TOKEN_COLON, "Expect ':' after '?' expression.");
        parsePrecedence(compiler, PREC_ASSIGNMENT);

        patchJump(compiler, elseJump);
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

    void string_literal(CompilerInstance* compiler, bool canAssign) {
        emitConstant(compiler, OBJ_VAL((Obj*)copyString(compiler->parser.previous.start + 1,
                                                        compiler->parser.previous.length - 2,
                                                        compiler->internTable)));
    }

    ParseRule rules[] = {
        {grouping, call,   PREC_CALL},       // TOKEN_LEFT_PAREN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RIGHT_PAREN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_LEFT_BRACE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RIGHT_BRACE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_COMMA
        {NULL,     dot,    PREC_CALL},       // TOKEN_DOT
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
        {NULL,     NULL,   PREC_NONE},       // TOKEN_PLUS_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_MINUS_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_STAR_EQUAL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_SLASH_EQUAL
        {prefixMutate, NULL,   PREC_NONE},   // TOKEN_PLUS_PLUS
        {prefixMutate, NULL,   PREC_NONE},   // TOKEN_MINUS_MINUS
        {NULL,     ternary,PREC_TERNARY},    // TOKEN_QUESTION
        {NULL,     NULL,   PREC_NONE},       // TOKEN_COLON
        {variable, NULL,   PREC_NONE},       // TOKEN_IDENTIFIER
        {string_literal, NULL,   PREC_NONE}, // TOKEN_STRING
        {number,   NULL,   PREC_NONE},       // TOKEN_NUMBER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_AND
        {NULL,     NULL,   PREC_NONE},       // TOKEN_BREAK
        {NULL,     NULL,   PREC_NONE},       // TOKEN_CASE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_CLASS
        {NULL,     NULL,   PREC_NONE},       // TOKEN_CONTINUE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_DEFAULT
        {NULL,     NULL,   PREC_NONE},       // TOKEN_ELSE
        {literal,  NULL,   PREC_NONE},       // TOKEN_FALSE
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FOR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_FUN
        {NULL,     NULL,   PREC_NONE},       // TOKEN_IF
        {NULL,     binary, PREC_COMPARISON}, // TOKEN_INSTANCEOF
        {literal,  NULL,   PREC_NONE},       // TOKEN_NIL
        {NULL,     NULL,   PREC_NONE},       // TOKEN_OR
        {NULL,     NULL,   PREC_NONE},       // TOKEN_PRINT
        {NULL,     NULL,   PREC_NONE},       // TOKEN_RETURN
        {super_,   NULL,   PREC_NONE},       // TOKEN_SUPER
        {NULL,     NULL,   PREC_NONE},       // TOKEN_SWITCH
        {this_,    NULL,   PREC_NONE},       // TOKEN_THIS
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

        declareVariable(compiler);
        if (compiler->compiler->scopeDepth > 0 || compiler->compiler->type != TYPE_SCRIPT) return 0;

        return identifierConstant(compiler, &compiler->parser.previous, compiler->internTable);
    }

    void defineVariable(CompilerInstance* compiler, uint8_t global) {
        if (compiler->compiler->scopeDepth > 0 || compiler->compiler->type != TYPE_SCRIPT) {
            markInitialized(compiler);
            return;
        }
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

    void emitLoop(CompilerInstance* compiler, int loopStart) {
        emitByte(compiler, OP_LOOP);

        int offset = currentChunk(compiler)->code.size() - loopStart + 2;
        if (offset > UINT16_MAX) error(compiler, "Loop body too large.");

        emitByte(compiler, (offset >> 8) & 0xff);
        emitByte(compiler, offset & 0xff);
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

    void beginScope(CompilerInstance* compiler) {
        compiler->compiler->scopeDepth++;
    }

    void endScope(CompilerInstance* compiler) {
        compiler->compiler->scopeDepth--;
        while (compiler->compiler->localCount > 0 &&
               compiler->compiler->locals[compiler->compiler->localCount - 1].depth > compiler->compiler->scopeDepth) {
            if (compiler->compiler->locals[compiler->compiler->localCount - 1].isCaptured) {
                emitByte(compiler, OP_CLOSE_UPVALUE);
            } else {
                emitByte(compiler, OP_POP);
            }
            compiler->compiler->localCount--;
        }
    }

    void block(CompilerInstance* compiler) {
        while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
            declaration(compiler);
        }
        consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    }

    void statement(CompilerInstance* compiler); // Forward declaration

    void beginLoop(CompilerInstance* compiler, Loop* loop) {
        loop->enclosing = compiler->compiler->loop;
        loop->start = 0;
        loop->scopeDepth = compiler->compiler->scopeDepth;
        loop->isLoop = true;
        compiler->compiler->loop = loop;
    }

    void endLoop(CompilerInstance* compiler) {
        if (compiler->compiler->loop == nullptr) return;

        for (int jump : compiler->compiler->loop->breakJumps) {
            patchJump(compiler, jump);
        }

        compiler->compiler->loop = compiler->compiler->loop->enclosing;
    }

    void breakStatement(CompilerInstance* compiler) {
        if (compiler->compiler->loop == nullptr) {
            error(compiler, "Can't use 'break' outside of a loop or switch.");
        }
        consume(compiler, TOKEN_SEMICOLON, "Expect ';' after 'break'.");

        if (compiler->compiler->loop == nullptr) return;

        for (int i = compiler->compiler->localCount - 1;
             i >= 0 && compiler->compiler->locals[i].depth > compiler->compiler->loop->scopeDepth;
             i--) {
             if (compiler->compiler->locals[i].isCaptured) {
                 emitByte(compiler, OP_CLOSE_UPVALUE);
             } else {
                 emitByte(compiler, OP_POP);
             }
        }

        compiler->compiler->loop->breakJumps.push_back(emitJump(compiler, OP_JUMP));
    }

    void continueStatement(CompilerInstance* compiler) {
        Loop* loop = compiler->compiler->loop;
        while (loop != nullptr && !loop->isLoop) {
            loop = loop->enclosing;
        }

        if (loop == nullptr) {
            error(compiler, "Can't use 'continue' outside of a loop.");
        }
        consume(compiler, TOKEN_SEMICOLON, "Expect ';' after 'continue'.");

        for (int i = compiler->compiler->localCount - 1;
             i >= 0 && compiler->compiler->locals[i].depth > loop->scopeDepth;
             i--) {
             if (compiler->compiler->locals[i].isCaptured) {
                 emitByte(compiler, OP_CLOSE_UPVALUE);
             } else {
                 emitByte(compiler, OP_POP);
             }
        }

        emitLoop(compiler, loop->start);
    }

    void switchStatement(CompilerInstance* compiler) {
        consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");

        beginScope(compiler);
        expression(compiler);
        addLocal(compiler, syntheticToken(" switch_temp "));
        markInitialized(compiler);

        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after switch value.");
        consume(compiler, TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

        Loop switchLoop;
        beginLoop(compiler, &switchLoop);
        switchLoop.isLoop = false;
        switchLoop.start = -1;

        int previousCaseSkip = -1;

        while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
            if (match(compiler, TOKEN_CASE) || match(compiler, TOKEN_DEFAULT)) {
                TokenType type = compiler->parser.previous.type;

                if (previousCaseSkip != -1) {
                    // Implicit break from previous case
                    int jumpToEnd = emitJump(compiler, OP_JUMP);
                    switchLoop.breakJumps.push_back(jumpToEnd);

                    patchJump(compiler, previousCaseSkip);
                    emitByte(compiler, OP_POP); // Pop boolean result of previous condition (wait, NO)
                    // Wait, previousCaseSkip jumps over BODY.
                    // The stack before jump is [bool] (result of equal).
                    // So at previousCaseSkip target, stack has [bool].
                    // So we must pop it.
                }

                if (type == TOKEN_CASE) {
                    emitBytes(compiler, OP_GET_LOCAL, (uint8_t)(compiler->compiler->localCount - 1));
                    expression(compiler);
                    consume(compiler, TOKEN_COLON, "Expect ':' after case value.");
                    emitByte(compiler, OP_EQUAL);
                    previousCaseSkip = emitJump(compiler, OP_JUMP_IF_FALSE);
                    emitByte(compiler, OP_POP); // Pop true result
                } else {
                    consume(compiler, TOKEN_COLON, "Expect ':' after default.");
                    previousCaseSkip = -1;
                }
            } else {
                if (previousCaseSkip != -1) {
                    // error(compiler, "Statements must follow a case or default.");
                    // Actually, statements are fine.
                }
                statement(compiler);
            }
        }

        if (previousCaseSkip != -1) {
             patchJump(compiler, previousCaseSkip);
             emitByte(compiler, OP_POP); // Pop boolean result
        }

        consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after switch cases.");
        endLoop(compiler);
        endScope(compiler);
    }

    void whileStatement(CompilerInstance* compiler) {
        int loopStart = currentChunk(compiler)->code.size();

        Loop loop;
        beginLoop(compiler, &loop);
        loop.start = loopStart;

        consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
        expression(compiler);
        consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int exitJump = emitJump(compiler, OP_JUMP_IF_FALSE);
        emitByte(compiler, OP_POP);
        statement(compiler);
        emitLoop(compiler, loopStart);

        patchJump(compiler, exitJump);
        emitByte(compiler, OP_POP);

        endLoop(compiler);
    }

    void forStatement(CompilerInstance* compiler) {
        beginScope(compiler);
        consume(compiler, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
        if (match(compiler, TOKEN_SEMICOLON)) {
            // No initializer.
        } else if (match(compiler, TOKEN_VAR)) {
            varDeclaration(compiler);
        } else {
            expressionStatement(compiler);
        }

        int loopStart = currentChunk(compiler)->code.size();

        Loop loop;
        beginLoop(compiler, &loop);
        loop.start = loopStart;

        int exitJump = -1;
        if (!match(compiler, TOKEN_SEMICOLON)) {
            expression(compiler);
            consume(compiler, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

            // Jump out of the loop if the condition is false.
            exitJump = emitJump(compiler, OP_JUMP_IF_FALSE);
            emitByte(compiler, OP_POP); // Condition.
        }

        if (!match(compiler, TOKEN_RIGHT_PAREN)) {
            int bodyJump = emitJump(compiler, OP_JUMP);
            int incrementStart = currentChunk(compiler)->code.size();
            expression(compiler);
            emitByte(compiler, OP_POP);
            consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

            emitLoop(compiler, loopStart);
            loopStart = incrementStart;
            loop.start = loopStart;
            patchJump(compiler, bodyJump);
        }

        statement(compiler);
        emitLoop(compiler, loopStart);

        if (exitJump != -1) {
            patchJump(compiler, exitJump);
            emitByte(compiler, OP_POP); // Condition.
        }

        endLoop(compiler);
        endScope(compiler);
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

    void returnStatement(CompilerInstance* compiler) {
        if (compiler->compiler->type == TYPE_SCRIPT) {
            error(compiler, "Can't return from top-level code.");
        }

        if (match(compiler, TOKEN_SEMICOLON)) {
            emitReturn(compiler);
        } else {
            expression(compiler);
            consume(compiler, TOKEN_SEMICOLON, "Expect ';' after return value.");
            emitByte(compiler, OP_RETURN);
        }
    }

    void statement(CompilerInstance* compiler) {
        if (match(compiler, TOKEN_PRINT)) {
            printStatement(compiler);
        } else if (match(compiler, TOKEN_FOR)) {
            forStatement(compiler);
        } else if (match(compiler, TOKEN_IF)) {
            ifStatement(compiler);
        } else if (match(compiler, TOKEN_RETURN)) {
            returnStatement(compiler);
        } else if (match(compiler, TOKEN_WHILE)) {
            whileStatement(compiler);
        } else if (match(compiler, TOKEN_BREAK)) {
            breakStatement(compiler);
        } else if (match(compiler, TOKEN_CONTINUE)) {
            continueStatement(compiler);
        } else if (match(compiler, TOKEN_SWITCH)) {
            switchStatement(compiler);
        } else if (match(compiler, TOKEN_LEFT_BRACE)) {
            beginScope(compiler);
            block(compiler);
            endScope(compiler);
        } else {
            expressionStatement(compiler);
        }
    }

    void function(CompilerInstance* compilerInstance, FunctionType type) {
        Compiler compiler;
        compiler.enclosing = compilerInstance->compiler;
        compiler.function = allocateFunction();
        compiler.type = type;
        compiler.localCount = 0;
        compiler.upvalueCount = 0;
        compiler.scopeDepth = 0;
        compiler.loop = nullptr;
        compilerInstance->compiler = &compiler;

        if (type != TYPE_SCRIPT) {
            compiler.function->name = copyString(compilerInstance->parser.previous.start, compilerInstance->parser.previous.length, compilerInstance->internTable);
        }

        Local* local = &compiler.locals[compiler.localCount++];
        local->depth = 0;
        local->name.start = "";
        local->name.length = 0;
        if (type != TYPE_FUNCTION && type != TYPE_SCRIPT) {
            local->name.start = "this";
            local->name.length = 4;
        }

        consume(compilerInstance, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
        if (!check(compilerInstance, TOKEN_RIGHT_PAREN)) {
            do {
                compiler.function->arity++;
                if (compiler.function->arity > 255) {
                    errorAtCurrent(compilerInstance, "Can't have more than 255 parameters.");
                }
                uint8_t constant = parseVariable(compilerInstance, "Expect parameter name.");
                defineVariable(compilerInstance, constant);
            } while (match(compilerInstance, TOKEN_COMMA));
        }
        consume(compilerInstance, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        consume(compilerInstance, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        block(compilerInstance);

        ObjFunction* function = compiler.function;
        function->upvalueCount = compiler.upvalueCount;
        emitReturn(compilerInstance);

        compilerInstance->compiler = compiler.enclosing;

        if (type != TYPE_SCRIPT) {
             emitBytes(compilerInstance, OP_CLOSURE, makeConstant(compilerInstance, OBJ_VAL((Obj*)function)));
             for (int i = 0; i < compiler.upvalueCount; i++) {
                 emitByte(compilerInstance, compiler.upvalues[i].isLocal ? 1 : 0);
                 emitByte(compilerInstance, compiler.upvalues[i].index);
             }
        }
    }

    void method(CompilerInstance* compiler) {
        consume(compiler, TOKEN_IDENTIFIER, "Expect method name.");
        uint8_t constant = identifierConstant(compiler, &compiler->parser.previous, compiler->internTable);

        FunctionType type = TYPE_METHOD;
        if (compiler->parser.previous.length == 4 && memcmp(compiler->parser.previous.start, "init", 4) == 0) {
            type = TYPE_INITIALIZER;
        }
        function(compiler, type);
        emitBytes(compiler, OP_METHOD, constant);
    }

    void classDeclaration(CompilerInstance* compiler) {
        consume(compiler, TOKEN_IDENTIFIER, "Expect class name.");
        Token className = compiler->parser.previous;
        uint8_t nameConstant = identifierConstant(compiler, &className, compiler->internTable);
        declareVariable(compiler);

        emitBytes(compiler, OP_CLASS, nameConstant);
        defineVariable(compiler, nameConstant);

        ClassCompiler classCompiler;
        classCompiler.enclosing = compiler->currentClass;
        classCompiler.hasSuperclass = false;
        compiler->currentClass = &classCompiler;

        if (match(compiler, TOKEN_LESS)) {
            consume(compiler, TOKEN_IDENTIFIER, "Expect superclass name.");
            variable(compiler, false);

            if (identifiersEqual(&className, &compiler->parser.previous)) {
                error(compiler, "A class can't inherit from itself.");
            }

            beginScope(compiler);
            addLocal(compiler, syntheticToken("super"));
            defineVariable(compiler, 0);

            namedVariable(compiler, className, false);
            emitByte(compiler, OP_INHERIT);
            classCompiler.hasSuperclass = true;
        }

        namedVariable(compiler, className, false);
        consume(compiler, TOKEN_LEFT_BRACE, "Expect '{' before class body.");
        while (!check(compiler, TOKEN_RIGHT_BRACE) && !check(compiler, TOKEN_EOF)) {
            method(compiler);
        }
        consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
        emitByte(compiler, OP_POP);

        if (classCompiler.hasSuperclass) {
            endScope(compiler);
        }

        compiler->currentClass = compiler->currentClass->enclosing;
    }

    void funDeclaration(CompilerInstance* compiler) {
        uint8_t global = parseVariable(compiler, "Expect function name.");
        markInitialized(compiler);
        function(compiler, TYPE_FUNCTION);
        defineVariable(compiler, global);
    }

    void declaration(CompilerInstance* compiler) {
        if (match(compiler, TOKEN_CLASS)) {
            classDeclaration(compiler);
        } else if (match(compiler, TOKEN_FUN)) {
            funDeclaration(compiler);
        } else if (match(compiler, TOKEN_VAR)) {
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

    ObjFunction* compile(const std::string& source, Table* internTable) {
        Scanner scanner(source.c_str());
        CompilerInstance compilerInstance;
        compilerInstance.scanner = &scanner;
        compilerInstance.internTable = internTable;
        compilerInstance.parser.hadError = false;
        compilerInstance.parser.panicMode = false;

        Compiler compiler;
        compiler.enclosing = nullptr;
        compiler.function = allocateFunction();
        compiler.type = TYPE_SCRIPT;
        compiler.localCount = 0;
        compiler.upvalueCount = 0;
        compiler.scopeDepth = 0;
        compiler.loop = nullptr;

        Local* local = &compiler.locals[compiler.localCount++];
        local->depth = 0;
        local->isCaptured = false;
        local->name.start = "";
        local->name.length = 0;

        compilerInstance.compiler = &compiler;
        compilerInstance.currentClass = nullptr;

        advance(&compilerInstance);

        while (!match(&compilerInstance, TOKEN_EOF)) {
            declaration(&compilerInstance);
        }

        emitReturn(&compilerInstance);

        ObjFunction* function = compilerInstance.parser.hadError ? nullptr : compiler.function;

        #ifdef DEBUG_PRINT_CODE
        if (!compilerInstance.parser.hadError) {
            function->chunk.disassemble(function->name != nullptr ? function->name->str.c_str() : "<script>");
        }
        #endif

        return function;
    }
}
