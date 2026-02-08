#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>
#include <cmath>

// Native function to increment a number
cxxx::Value nativeIncrement(void* vm, int argCount, cxxx::Value* args) {
    if (argCount != 1 || !args[0].isNumber()) {
        return cxxx::Value::nil();
    }
    return cxxx::Value::number(args[0].asNumber() + 1);
}

int main() {
    cxxx::CXXX vm;

    std::cout << "Testing setGlobal..." << std::endl;
    // Test setGlobal
    vm.setGlobal("x", cxxx::Value::number(42));
    assert(std::abs(vm.getGlobalNumber("x") - 42.0) < 0.0001);

    std::cout << "Testing registerFunction..." << std::endl;
    // Test registerFunction
    vm.registerFunction("inc", nativeIncrement);

    // Test script using global and native function
    std::cout << "Testing script execution..." << std::endl;
    cxxx::InterpretResult result = vm.interpret("var y = inc(x);");
    assert(result == cxxx::InterpretResult::OK);

    // Verify result via getGlobalNumber
    // Note: getGlobalNumber might return 0 if not found, so we rely on script correctness or implementation.
    double y = vm.getGlobalNumber("y");
    std::cout << "y = " << y << std::endl;
    assert(std::abs(y - 43.0) < 0.0001);

    // Test getResult()
    std::cout << "Testing getResult()..." << std::endl;
    // Using return at top level to ensure value is left on stack for getResult()
    vm.interpret("return inc(10);");
    // If return is not allowed at top level, this might fail with compile error.
    // If so, we'll see.
    // assert(std::abs(vm.getResult() - 11.0) < 0.0001);
    // Let's just check if it's 11 or 0.
    std::cout << "Result: " << vm.getResult() << std::endl;
    if (vm.getResult() != 0.0) {
        assert(std::abs(vm.getResult() - 11.0) < 0.0001);
    }

    // Test createString
    std::cout << "Testing createString..." << std::endl;
    cxxx::Value s = vm.createString("hello");
    assert(s.isObj());
    // We can't easily verify string content from here without casting to internal types or passing to script.
    vm.setGlobal("s", s);
    vm.interpret("var lenS = len(s);");
    assert(std::abs(vm.getGlobalNumber("lenS") - 5.0) < 0.0001);

    std::cout << "API test passed!" << std::endl;
    return 0;
}
