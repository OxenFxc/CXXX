#include "../src/include/cxxx.h"
#include <iostream>
#include <cassert>
#include <string>

int main() {
    cxxx::CXXX vm;
    vm.loadStdLib();

    std::string script = R"(
class Node {
  init(val, prev, next) {
    this.val = val;
    this.prev = prev;
    this.next = next;
  }
}

class Tape {
  init() {
    this.current = Node(0, nil, nil);
  }

  inc() {
    this.current.val = this.current.val + 1;
  }

  dec() {
    this.current.val = this.current.val - 1;
  }

  left() {
    if (this.current.prev == nil) {
      this.current.prev = Node(0, nil, this.current);
    }
    this.current = this.current.prev;
  }

  right() {
    if (this.current.next == nil) {
      this.current.next = Node(0, this.current, nil);
    }
    this.current = this.current.next;
  }

  get() {
    return this.current.val;
  }
}

fun interpret(code) {
  var tape = Tape();
  var ip = 0;
  var codeLen = len(code);

  while (ip < codeLen) {
    var c = strAt(code, ip);
    if (c == "+") {
      tape.inc();
    } else if (c == "-") {
      tape.dec();
    } else if (c == ">") {
      tape.right();
    } else if (c == "<") {
      tape.left();
    } else if (c == ".") {
      print(tape.get());
    } else if (c == "[") {
       if (tape.get() == 0) {
         var depth = 1;
         while (depth > 0) {
           ip = ip + 1;
           if (ip >= codeLen) break;
           var cc = strAt(code, ip);
           if (cc == "[") depth = depth + 1;
           else if (cc == "]") depth = depth - 1;
         }
       }
    } else if (c == "]") {
       if (tape.get() != 0) {
         var depth = 1;
         while (depth > 0) {
           ip = ip - 1;
           if (ip < 0) break;
           var cc = strAt(code, ip);
           if (cc == "]") depth = depth + 1;
           else if (cc == "[") depth = depth - 1;
         }
       }
    }
    ip = ip + 1;
  }
  return tape.get();
}

// Simple test: Set cell to 5 (+++++)
var code1 = "+++++";
print("Running code1: " + code1);
var res1 = interpret(code1);
print("Result 1: " + res1);

// Test moves and loops: Add 2 + 3
// Cell 0 = 2, Cell 1 = 3.
// Loop: dec Cell 1, inc Cell 0, repeat until Cell 1 is 0.
var code2 = "++ > +++ [ < + > - ] < .";
print("Running code2: " + code2);
var res2 = interpret(code2);
print("Result 2: " + res2);
)";

    cxxx::InterpretResult result = vm.interpret(script);
    assert(result == cxxx::InterpretResult::OK);

    assert(vm.getGlobalNumber("res1") == 5.0);
    assert(vm.getGlobalNumber("res2") == 5.0);

    std::cout << "Turing completeness test passed." << std::endl;
    return 0;
}
