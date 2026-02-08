#include "../src/compiler/scanner.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>

using namespace cxxx;

void test_scanner() {
    const char* source = "var a = 12.3 + 45;";
    Scanner scanner(source);

    std::vector<TokenType> expected = {
        TOKEN_VAR, TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_NUMBER,
        TOKEN_PLUS, TOKEN_NUMBER, TOKEN_SEMICOLON, TOKEN_EOF
    };

    int i = 0;
    for (TokenType type : expected) {
        Token token = scanner.scanToken();
        if (token.type != type) {
            std::cerr << "Mismatch at index " << i << ". Expected " << type << ", got " << token.type << std::endl;
            std::string lexeme(token.start, token.length);
            std::cerr << "Lexeme: " << lexeme << std::endl;
        }
        assert(token.type == type);
        i++;
    }
}

void test_new_tokens() {
    const char* source = "+= -= *= /= ++ -- ? :";
    Scanner scanner(source);

    std::vector<TokenType> expected = {
        TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL,
        TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS, TOKEN_QUESTION, TOKEN_COLON,
        TOKEN_EOF
    };

    int i = 0;
    for (TokenType type : expected) {
        Token token = scanner.scanToken();
        if (token.type != type) {
            std::cerr << "Mismatch at index " << i << ". Expected " << type << ", got " << token.type << std::endl;
            std::string lexeme(token.start, token.length);
            std::cerr << "Lexeme: " << lexeme << std::endl;
        }
        assert(token.type == type);
        i++;
    }
}

int main() {
    test_scanner();
    test_new_tokens();
    std::cout << "Scanner Test Passed." << std::endl;
    return 0;
}
