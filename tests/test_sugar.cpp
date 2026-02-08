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

    // Break in while
    vm.interpret("var m = 0; while (true) { m++; if (m == 3) break; }");
    assert(vm.getGlobalNumber("m") == 3.0);

    // Continue in while
    vm.interpret("var n = 0; var o = 0; while (n < 5) { n++; if (n == 3) continue; o++; }");
    // n: 1, o: 1
    // n: 2, o: 2
    // n: 3, o: 2 (continue)
    // n: 4, o: 3
    // n: 5, o: 4
    assert(vm.getGlobalNumber("o") == 4.0);

    // Break in for
    vm.interpret("var p = 0; for (var x = 0; x < 10; x++) { if (x == 3) break; p = x; }");
    assert(vm.getGlobalNumber("p") == 2.0);

    // Continue in for
    vm.interpret("var q = 0; for (var y = 0; y < 5; y++) { if (y == 2) continue; q++; }");
    // 0, 1, 3, 4 -> 4 iterations
    assert(vm.getGlobalNumber("q") == 4.0);

    // Switch case
    vm.interpret("var r = 0; switch (1) { case 1: r = 1; case 2: r = 2; }");
    // No fallthrough, so r should be 1.
    assert(vm.getGlobalNumber("r") == 1.0);

    // Switch default
    vm.interpret("var s = 0; switch (3) { case 1: s = 1; default: s = 3; }");
    assert(vm.getGlobalNumber("s") == 3.0);

    // Switch break
    vm.interpret("var t = 0; switch (1) { case 1: t = 1; break; t = 2; }");
    assert(vm.getGlobalNumber("t") == 1.0);

    std::cout << "Syntax sugar tests passed." << std::endl;
    return 0;
}
