#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    // Test simple if
    vm.interpret("var a = 0; if (true) { a = 1; }");
    assert(vm.getGlobalNumber("a") == 1.0);

    // Test simple if false
    vm.interpret("var b = 0; if (false) { b = 1; }");
    assert(vm.getGlobalNumber("b") == 0.0);

    // Test if-else
    vm.interpret("var c = 0; if (true) { c = 1; } else { c = 2; }");
    assert(vm.getGlobalNumber("c") == 1.0);

    vm.interpret("var d = 0; if (false) { d = 1; } else { d = 2; }");
    assert(vm.getGlobalNumber("d") == 2.0);

    // Test nested if
    vm.interpret("var e = 0; if (true) { if (true) { e = 1; } }");
    assert(vm.getGlobalNumber("e") == 1.0);

    // Test without braces
    vm.interpret("var f = 0; if (true) f = 1;");
    assert(vm.getGlobalNumber("f") == 1.0);

    std::cout << "Control flow tests passed." << std::endl;
    return 0;
}
