#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    std::cout << "Testing Super Call..." << std::endl;
    vm.interpret(
        "class A {"
        "  method() { return 10; }"
        "}"
        "class B < A {"
        "  method() { return super.method() + 5; }"
        "}"
        "var b = B();"
        "var res = b.method();"
    );
    assert(vm.getGlobalNumber("res") == 15.0);

    std::cout << "Testing Indirect Super Call..." << std::endl;

    vm.interpret(
        "class Base {"
        "  say() { return 1; }"
        "}"
        "class Derived < Base {"
        "  getClosure() {"
        "    fun closure() {"
        "      return super.say();"
        "    }"
        "    return closure;"
        "  }"
        "}"
        "var d = Derived();"
        "var fn = d.getClosure();"
        "var res2 = fn();"
    );
    assert(vm.getGlobalNumber("res2") == 1.0);

    std::cout << "Super Tests Passed." << std::endl;
    return 0;
}
