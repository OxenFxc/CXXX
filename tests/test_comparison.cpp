#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    // Less than
    vm.interpret("var a = 1 < 2;");
    assert(vm.getGlobalBool("a") == true);

    vm.interpret("var b = 2 < 1;");
    assert(vm.getGlobalBool("b") == false);

    // Equality
    vm.interpret("var c = 1 == 1;");
    assert(vm.getGlobalBool("c") == true);

    vm.interpret("var d = 1 != 1;");
    assert(vm.getGlobalBool("d") == false);

    // Less than or equal
    vm.interpret("var e = 1 <= 1;");
    assert(vm.getGlobalBool("e") == true);

    vm.interpret("var e2 = 1 <= 2;");
    assert(vm.getGlobalBool("e2") == true);

    vm.interpret("var e3 = 2 <= 1;");
    assert(vm.getGlobalBool("e3") == false);

    // Greater than or equal
    vm.interpret("var f = 1 >= 1;");
    assert(vm.getGlobalBool("f") == true);

    vm.interpret("var f2 = 2 >= 1;");
    assert(vm.getGlobalBool("f2") == true);

    vm.interpret("var f3 = 1 >= 2;");
    assert(vm.getGlobalBool("f3") == false);

    // Not
    vm.interpret("var g = !true;");
    assert(vm.getGlobalBool("g") == false);

    vm.interpret("var h = !nil;");
    assert(vm.getGlobalBool("h") == true);

    std::cout << "Comparison tests passed." << std::endl;
    return 0;
}
