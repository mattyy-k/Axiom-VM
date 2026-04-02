# Axiom VM

Axiom VM is a bytecode-compiled scripting language runtime built from scratch in C++.

Source code is compiled via a hand-written lexer and recursive descent parser into a flat bytecode instruction set, which is executed by a stack-based virtual machine with real call frames, lexical scoping, and a mark-and-sweep garbage collector.

This is not a tree-walking interpreter. Source goes in, bytecode comes out, and a VM executes it.

---

## Performance

Benchmarked on a mixed workload combining recursion, iteration, and function calls (fibonacci, power, factorial, collatz):
```
221
65536
3628800
6765
499500
111

$>--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--<$
6452560 instruction(s) executed in 256193 microseconds.
Number of garbage collections: 0
Number of objects collected: 0
Maximum stack depth: 12
$>--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--<$
```

**~25 million VM instructions per second** on this workload.

---

## Features

### Language

- Integers, booleans, strings, arrays (heterogeneous)
- Arithmetic, comparison, and logical operators
- Variables with lexical block scoping
- Functions with parameters and return values
- Recursive functions
- `if / else if / else`
- `while` loops
- Array literals, indexing, and element assignment

---

### Compiler

- Hand-written lexer
- Recursive descent parser
- Single-pass compilation with backpatching for control flow
- Bytecode compiler emitting flat integer instruction stream

**Optimizations**
- Constant folding (e.g. `60 * 60 * 24 * 365 → PUSH`)
- Algebraic simplification (`x * 1`, `x + 0`, `x * 0`)
- Peephole optimizer:
  - removes redundant `PUSH/POP`
  - eliminates no-op local assignments
  - strips dead instructions after `RET`

**Error Handling**
- Line-accurate compiler errors
- Multiple error accumulation via `hadError` flag
- Execution aborted on compile failure

---

### Runtime

- Register-less stack-based virtual machine
- Switch-based dispatch loop
- Call frames with fixed local slots (similar to CPython/Lua execution models)
- Lexical scoping with inner-to-outer resolution
- Heap with free-list reuse
- Stack holds values; heap stores objects (strings, arrays)

---

### Garbage Collector

Axiom uses a mark-and-sweep garbage collector with a free-list allocator.

**Trigger**  
Runs when allocations since last collection exceed a threshold.

**Mark phase**
- Traverses all roots:
  - operand stack
  - locals in every active call frame
- Recursively marks reachable heap objects

**Sweep phase**
- Frees unmarked objects
- Recycles memory via free list

**Demonstrated under allocation-heavy workload:**
```
109452 instruction(s) executed in 7684 microseconds.
Number of garbage collections: 35
Number of objects collected: 1782
Maximum stack depth: 6
```

Correctly handles deep recursion and live references across frames.

---

### Instrumentation

CLI flags expose internal execution:

- Bytecode disassembler
- Execution stats:
  - instruction count
  - elapsed time
  - GC runs
  - objects collected
  - max stack depth
  - constant folds
- Opcode frequency table

---

## Usage
```
axiom program.vm # run
axiom program.vm -s # run + stats
axiom program.vm -b # run + disassemble bytecode
axiom program.vm -c # run + compile time
axiom program.vm -o # opcode frequency
axiom program.vm -a # all instrumentation
```

---

## Example Program

```vm
fun factorial(n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

fun fib(n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    return fib(n - 1) + fib(n - 2);
}

fun sumArray(arr, len) {
    let i = 0;
    let sum = 0;
    while (i < len) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}

let nums = [1,2,3,4,5,6,7,8,9,10];

print factorial(10);
print fib(20);
print sumArray(nums, 10);
```

### Example Bytecode
```bytecode
0: JUMP -> 27
2: GET_LOCAL slot=0
4: PUSH 0
6: EQ
7: JUMP_IF_FALSE -> 12
9: PUSH 1
11: RET
12: GET_LOCAL slot=0
14: GET_LOCAL slot=0
16: PUSH 1
18: SUB
19: CALL argc=1 target=2
22: MUL
23: RET
24: PUSH 0
26: RET
27: JUMP -> 70
29: GET_LOCAL slot=0
31: PUSH 0
33: EQ
34: JUMP_IF_FALSE -> 39
36: PUSH 0
38: RET
39: GET_LOCAL slot=0
41: PUSH 1
43: EQ
44: JUMP_IF_FALSE -> 49
46: PUSH 1
48: RET
49: GET_LOCAL slot=0
51: PUSH 1
53: SUB
54: CALL argc=1 target=29
57: GET_LOCAL slot=0
59: PUSH 2
61: SUB
62: CALL argc=1 target=29
65: ADD
66: RET
67: PUSH 0
69: RET
70: JUMP -> 116
72: PUSH 0
74: SET_LOCAL slot=2
76: POP
77: PUSH 0
79: SET_LOCAL slot=3
81: POP
82: GET_LOCAL slot=2
84: GET_LOCAL slot=1
86: LT
87: JUMP_IF_FALSE -> 110
89: GET_LOCAL slot=3
91: GET_LOCAL slot=0
93: GET_LOCAL slot=2
95: GET_INDEX
96: ADD
97: SET_LOCAL slot=3
99: POP
100: GET_LOCAL slot=2
102: PUSH 1
104: ADD
105: SET_LOCAL slot=2
107: POP
108: JUMP -> 82
110: GET_LOCAL slot=3
112: RET
113: PUSH 0
115: RET
116: ALLOC_ARRAY size=10
118: PUSH 0
120: PUSH 1
122: SET_INDEX
123: PUSH 1
125: PUSH 2
127: SET_INDEX
128: PUSH 2
130: PUSH 3
132: SET_INDEX
133: PUSH 3
135: PUSH 4
137: SET_INDEX
138: PUSH 4
140: PUSH 5
142: SET_INDEX
143: PUSH 5
145: PUSH 6
147: SET_INDEX
148: PUSH 6
150: PUSH 7
152: SET_INDEX
153: PUSH 7
155: PUSH 8
157: SET_INDEX
158: PUSH 8
160: PUSH 9
162: SET_INDEX
163: PUSH 9
165: PUSH 10
167: SET_INDEX
168: SET_LOCAL slot=0
170: POP
171: PUSH 10
173: CALL argc=1 target=2
176: PRINT
177: PUSH 20
179: CALL argc=1 target=29
182: PRINT
183: GET_LOCAL slot=0
185: PUSH 10
187: CALL argc=2 target=72
190: PRINT
191: HALT
```

## Architecture
```
Source Code
    │
    ▼
Lexer                    (hand-written, single pass)
    │
    ▼
Recursive Descent Parser (AST generation)
    │
    ▼
Constant Folding Pass    (AST-level optimization)
    │
    ▼
Bytecode Compiler        (flat instruction stream + backpatching)
    │
    ▼
Peephole Optimizer       (post-pass simplification)
    │
    ▼
Stack-based VM           (execution, call frames, heap, GC)
```
## Future Work
- Tail call optimization
- Closures and first-class functions
Optional type system
JIT compilation for hot paths


## Author

Built by **Maitreya Kulkarni** — NITK IT (2nd year)

Also: [Forge Shell](https://github.com/mattyy-k/Forge-Shell) — a POSIX-compliant shell in C++ used as the development and benchmarking environment for Axiom VM.
