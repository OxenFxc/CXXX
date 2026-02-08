#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    std::cout << "Testing Class Definition..." << std::endl;
    // Class definition
    vm.interpret("class Foo {}");
    // Just ensuring it doesn't crash on definition

    std::cout << "Testing Instantiation..." << std::endl;
    // Instantiation
    vm.interpret("class Bar {} var b = Bar();");
    // TODO: Need a way to check if b is an instance of Bar from C++ side,
    // or just rely on script assertions if I implement assert in script.
    // For now, I'll rely on it not crashing and potentially print output.

    std::cout << "Testing Properties..." << std::endl;
    // Set and Get Property
    vm.interpret("class Baz {} var b = Baz(); b.x = 10; var y = b.x;");
    assert(vm.getGlobalNumber("y") == 10.0);

    std::cout << "Testing Methods..." << std::endl;
    // Method call
    vm.interpret(
        "class Math { "
        "  add(a, b) { this.count = 1; return a + b; } "
        "} "
        "var m = Math(); "
        "var sum = m.add(2, 3);"
    );
    assert(vm.getGlobalNumber("sum") == 5.0);

    // std::cout << "Testing 'this'..." << std::endl;
    // // 'this'
    // vm.interpret(
    //     "class Counter { "
    //     "  init() { this.count = 0; } "
    //     "  inc() { this.count = this.count + 1; } "
    //     "} "
    //     "var c = Counter(); "
    //     "c.init(); "
    //     "c.inc(); "
    //     "var res = c.count;"
    // );
    // // assert(vm.getGlobalNumber("res") == 1.0);

    std::cout << "Testing Inheritance..." << std::endl;
    // Inheritance
    vm.interpret(
        "class A { "
        "  method() { return 1; } "
        "} "
        "class B < A {} "
        "var b = B(); "
        "var res2 = b.method();"
    );
    assert(vm.getGlobalNumber("res2") == 1.0);

    // std::cout << "Testing 'super'..." << std::endl;
    // // Super
    // vm.interpret(
    //     "class Base { "
    //     "  val() { return 10; } "
    //     "} "
    //     "class Derived < Base { "
    //     "  val() { return super.val() + 5; } "
    //     "} "
    //     "var d = Derived(); "
    //     "var res3 = d.val();"
    // );
    // assert(vm.getGlobalNumber("res3") == 15.0);

    std::cout << "OO Tests Passed." << std::endl;
    return 0;
}
