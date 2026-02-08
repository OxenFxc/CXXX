#ifndef cxxx_scanner_h
#define cxxx_scanner_h

#include <string>
#include <string_view>

namespace cxxx {

    enum TokenType {
        // Single-character tokens.
        TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
        TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
        TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

        // One or two character tokens.
        TOKEN_BANG, TOKEN_BANG_EQUAL,
        TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
        TOKEN_GREATER, TOKEN_GREATER_EQUAL,
        TOKEN_LESS, TOKEN_LESS_EQUAL,
        TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
        TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,
        TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS,
        TOKEN_QUESTION, TOKEN_COLON,

        // Literals.
        TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

        // Keywords.
        TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
        TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
        TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
        TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

        TOKEN_ERROR, TOKEN_EOF
    };

    struct Token {
        TokenType type;
        const char* start;
        int length;
        int line;
    };

    class Scanner {
    public:
        Scanner(const char* source);
        Token scanToken();

    private:
        const char* start;
        const char* current;
        int line;

        bool isAtEnd();
        char advance();
        char peek();
        char peekNext();
        bool match(char expected);
        Token makeToken(TokenType type);
        Token errorToken(const char* message);
        void skipWhitespace();
        TokenType checkKeyword(int start, int length, const char* rest, TokenType type);
        TokenType identifierType();
        Token identifier();
        Token number();
        Token string();
    };

}

#endif
