#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    std::cout << "Testing Basic Closure..." << std::endl;

    vm.interpret(
        "var a = 10;"
        "fun makeClosure() {"
        "  var a = 20;"
        "  fun closure() {"
        "    return a;"
        "  }"
        "  return closure;"
        "}"
        "var fn = makeClosure();"
        "var val = fn();"
    );
    assert(vm.getGlobalNumber("val") == 20.0);

    std::cout << "Testing Closure Mutation..." << std::endl;

    vm.interpret(
        "var b = 1;"
        "fun counter() {"
        "  var i = 0;"
        "  fun inc() {"
        "    i = i + 1;"
        "    return i;"
        "  }"
        "  return inc;"
        "}"
        "var c = counter();"
        "var c1 = c();"
        "var c2 = c();"
    );
    assert(vm.getGlobalNumber("c1") == 1.0);
    assert(vm.getGlobalNumber("c2") == 2.0);

    std::cout << "Closure Tests Passed." << std::endl;
    return 0;
}
