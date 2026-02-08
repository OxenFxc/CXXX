#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    std::cout << "Testing instanceof..." << std::endl;
    // 1. Basic instance check
    vm.interpret(
        "class A {} "
        "var a = A(); "
        "var check1 = a instanceof A;"
    );
    if (!vm.getGlobalBool("check1")) {
        std::cerr << "Failed: a instanceof A should be true" << std::endl;
        return 1;
    }

    // 2. Inheritance check
    vm.interpret(
        "class B < A {} "
        "var b = B(); "
        "var check2 = b instanceof A;"
    );
    if (!vm.getGlobalBool("check2")) {
        std::cerr << "Failed: b instanceof A should be true (inheritance)" << std::endl;
        return 1;
    }

    // 3. Negative check
    vm.interpret(
        "class C {} "
        "var c = C(); "
        "var check3 = c instanceof A;"
    );
    if (vm.getGlobalBool("check3")) {
        std::cerr << "Failed: c instanceof A should be false" << std::endl;
        return 1;
    }

    // 4. Non-instance check
    vm.interpret(
        "var check4 = 123 instanceof A;"
    );
    if (vm.getGlobalBool("check4")) {
        std::cerr << "Failed: 123 instanceof A should be false" << std::endl;
        return 1;
    }

    // 5. This context check
    std::cout << "Testing 'this'..." << std::endl;
    vm.interpret(
        "class Counter { "
        "  init() { this.count = 0; } "
        "  inc() { this.count = this.count + 1; } "
        "} "
        "var cnt = Counter(); "
        "cnt.init(); "
        "cnt.inc(); "
        "var res = cnt.count;"
    );
    if (vm.getGlobalNumber("res") != 1.0) {
        std::cerr << "Failed: 'this' context test" << std::endl;
        return 1;
    }

    // 6. Super context check
    std::cout << "Testing 'super'..." << std::endl;
    vm.interpret(
        "class Base { "
        "  val() { return 10; } "
        "} "
        "class Derived < Base { "
        "  val() { return super.val() + 5; } "
        "} "
        "var d = Derived(); "
        "var res3 = d.val();"
    );
    if (vm.getGlobalNumber("res3") != 15.0) {
        std::cerr << "Failed: 'super' context test" << std::endl;
        return 1;
    }

    std::cout << "Completion OOP Tests Passed." << std::endl;
    return 0;
}
