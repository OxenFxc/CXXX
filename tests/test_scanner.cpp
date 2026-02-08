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

int main() {
    test_scanner();
    std::cout << "Scanner Test Passed." << std::endl;
    return 0;
}
