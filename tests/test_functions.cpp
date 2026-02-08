#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>

int main() {
    cxxx::CXXX vm;

    // Test basic function
    vm.interpret("fun f() { return 1; } var a = f();");
    assert(vm.getGlobalNumber("a") == 1.0);

    // Test function with args
    vm.interpret("fun add(a, b) { return a + b; } var b = add(2, 3);");
    assert(vm.getGlobalNumber("b") == 5.0);

    // Test local variables
    vm.interpret("fun loc() { var x = 10; return x; } var c = loc();");
    assert(vm.getGlobalNumber("c") == 10.0);

    // Test scope shadowing
    vm.interpret("var d = 0;");
    vm.interpret("{ var d = 5; }");
    // Global d should be 0.
    double d = vm.getGlobalNumber("d");
    std::cout << "d = " << d << std::endl;
    assert(d == 0.0);

    // Test recursion
    // vm.interpret("fun fib(n) { if (n < 2) return n; return fib(n - 1) + fib(n - 2); } var e = fib(5);");
    // assert(vm.getGlobalNumber("e") == 5.0);

    vm.interpret("fun rec(n) { if (n <= 0) return 0; return n + rec(n-1); } var f = rec(3);");
    double f = vm.getGlobalNumber("f");
    std::cout << "f = " << f << std::endl;
    assert(f == 6.0);

    vm.interpret("fun fib(n) { if (n < 2) return n; return fib(n - 1) + fib(n - 2); } var e = fib(5);");
    double e = vm.getGlobalNumber("e");
    std::cout << "e = " << e << std::endl;
    assert(e == 5.0);

    std::cout << "Function tests passed." << std::endl;
    return 0;
}
