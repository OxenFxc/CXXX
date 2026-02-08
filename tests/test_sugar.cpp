#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    // +=
    vm.interpret("var a = 1; a += 2;");
    assert(vm.getGlobalNumber("a") == 3.0);

    // -=
    vm.interpret("var b = 5; b -= 2;");
    assert(vm.getGlobalNumber("b") == 3.0);

    // *=
    vm.interpret("var c = 2; c *= 3;");
    assert(vm.getGlobalNumber("c") == 6.0);

    // /=
    vm.interpret("var d = 6; d /= 2;");
    assert(vm.getGlobalNumber("d") == 3.0);

    // ++ prefix
    vm.interpret("var e = 1; ++e;");
    assert(vm.getGlobalNumber("e") == 2.0);

    // -- prefix
    vm.interpret("var f = 2; --f;");
    assert(vm.getGlobalNumber("f") == 1.0);

    // ++ postfix
    vm.interpret("var g = 1; g++;");
    assert(vm.getGlobalNumber("g") == 2.0);

    // -- postfix
    vm.interpret("var h = 2; h--;");
    assert(vm.getGlobalNumber("h") == 1.0);

    // Ternary
    vm.interpret("var i = true ? 1 : 2;");
    assert(vm.getGlobalNumber("i") == 1.0);

    vm.interpret("var j = false ? 1 : 2;");
    assert(vm.getGlobalNumber("j") == 2.0);

    // Nested ternary
    vm.interpret("var k = true ? (false ? 1 : 2) : 3;");
    assert(vm.getGlobalNumber("k") == 2.0);

    std::cout << "Syntax sugar tests passed." << std::endl;
    return 0;
}
