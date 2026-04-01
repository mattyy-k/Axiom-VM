# progression.md — Build Log for My Virtual Machine
## This file is basically the VM’s diary. If something breaks, cries (me), explodes, or achieves sentience — it’s going in here.

## Day 0: Setup and Goals
### Why I'm building a VM: 
Because... Why Not? 😄 
But on a serious note, the project I built before this was a fully-functioning shell, and that really got me hooked to systems. The next logical step is building a Virtual Machine.
And more importantly, because I just really love building stuff.
### What features I want:
I plan to be as masochistic as possible, so short answer: All of them.
Long answer:

#### Phase 1 — Core VM (The Skeleton)
- A bytecode format (my VM’s own language, because why follow instructions?)
- Instruction dispatch loop (aka "the VM hamster wheel")
- A stack machine (push, pop, pray)
- Arithmetic ops (+, -, *, / — but without floating point meltdowns)
- Control flow: jumps, conditionals, loops

#### Phase 2 — Memory Model (Where objects go to live & occasionally die)
- Heap allocator
- Fixed layout for objects (integers, strings, arrays, etc.)
- Frames & call stack
- GC roots and tracing prep

#### Phase 3 — Garbage Collector (The Grim Reaper)
- Mark & Sweep (basic)
- Correct root finding (global vars, stack frames)
- No accidental deletion of living objects (hopefully)
- No memory leaks when GC forgets to sweep (hopefully again)

#### Phase 4 — Parser + Compiler (The VM learns to read)
- Tokenizer
- AST builder
- Compiler -> bytecode
- Functions, variables, local scopes
- Error messages that don’t gaslight users

#### Phase 5 — High-Level Language Features (The spice)
I'll add this as I want:

##### Option A — Performance Flex
- Bytecode optimizer
- Simple JIT (compile hot loops)
- Benchmark suite (compare with Lua/CPython)
- VM potentially gets faster than I do

##### Option B — Developer Experience (the “this is actually fun to code in” route)
- Debugger (step, breakpoints, inspect stack/heap)
- Error messages with line numbers & context
- REPL with syntax highlighting

##### Option C — Language Nerd Heaven (PL theory street cred)
- Closures with lexical scoping
- First-class functions
- Tail call optimization
- Theoretical bragging rights

### Languages/tools I'm using:
-C++
-VS Code
-Caffeine.
### What scares me/excites me:
Whatever scares me also excites me. 😈
### What I expect to learn:
Systems chops!
Also, how to function at 3 AM solely on caffeine fumes.

### Phase 0 (16/12/2025):
```
-Stack-based VM and not Register-based. Reason: fast enough, and much simpler to implement.

-Execution model:
  -The VM is a state machine whose state consists of the instruction pointer, operand stack and heap.
  -Instruction Pointer
  -Dispatch Loop (the heart of the VM)
  -Instruction Dispatch:
    -The first byte read is the opcode
    -The VM uses a giant switch statement to decode or dispatch the instruction.
    
-Value Model:
  -Dynamic typing
  -Value Type: single Value type with Tagged Union semantics.
  -Optimizations will not be implemented at this stage. Instead, I will design clean abstractions that allow future optimization.
  
-Memory Model:
  -Primitive values on the Vm stack; heap objects are referenced via pointers stored in Values.
-Non-goals: No GC, closures, JIT, threads, native FFI, Optimizer, Exceptions, YET. Will be implemented in later phases.
```
### Phase 1 (10/01/2026):

-Enum class Opcode for all opcodes: PUSH, POP, ADD, SUB, MUL, DIV, MOD, HALT. Why enum class and not enum? Because enum allows a lot of shady, implicit shit, and allowing implicit shit in your VM is the best way to shoot yourself in the foot. (For those who are curious about what I mean by 'shady shit': I mean implicit conversions and accidental misuse, which is dangerous in a VM, where explicitness and correctness are critical)

-Stack invariant followed as religiously as my daily call to my mom. :)

-The thing I had to figure out without which I could not have written a single line of code: The VM is a dumb little shit 🙄. It does not (and rightly so) care about anything except the bytecode it's fed, and how to execute that bytecode in the most efficient manner possible.

-One bug I encountered that threw probably one of the longest error messages I've ever seen 💀: switch cases DO NOT create a new scope per case; the entire switch statement is one scope. If you don't care about what that means, just follow this: if you're declaring anything in your switch case, the switch case MUST be wrapped in {} brackets, especially if you're going to re-declare it in another case.<br>
Another bug that had me blaming the compiler for a while was enum casting. And guess what? It was due to my decision to be the good guy and pick enum class over enum. It would not have occured, had I gone with enum instead of enum class (because implicit conversion). :/ Can't get a break in this economy 🙄


### Phase 2 (10/01/2026):
-Added proper function calls via CALL/RET and a separate call stack. Each function invocation gets its own call frame containing a return IP and a frame base into the operand stack. Operand stack holds values only; call stack holds control state only. Mixing the two is how you get early dementia.

-The key realization in this phase is this: A function call isn't wizardry. It's just dropping a 'brb I gotchu' bookmark and jumping. CALL saves where execution should resume and where the callee’s stack slice begins; RET restores both and nukes everything above the frame base. No magic, no hidden state, no 'function objects'.

-My sleep deprived self got bytecode and operand stack mixed up. 💀 I spent 15 minutes trying to gaslight myself into committing a war crime: modifying the bytecode.

-Return values are handled explicitly: if the callee leaves a value on the stack, RET preserves it by popping it, cleaning the stack, and pushing it back. This also explains why stack cleanup is O(1): you’re not deleting values, you’re just rewinding the stack pointer.

-This phase went much smoother than Phase 1. That does NOT bode well 💀. Feels like the calm before the shitstorm.

### Phase 3 (12/01/2026):

Well... I wasn't wrong. That kinda was a shitstorm.

-Stack vs heap clicked pretty early: stack is only for handles to objects, while heap is for objects.

-The Value abstraction was pretty neat. A tagged union. This also incidentally happened to be the main phase shift my stubborn brain couldn't internalize: operand stack values are not integers anymore, they're struct Values, able to store any primitive data type and also references to heap objects. Arrays forced me to stop assuming that everything was an int and finally treat Value as a first-class runtime entity.

-Ran into a couple of segfaults that had me screaming inside in the library (don't do this shit for more than 5 hours a day, kids 🙄). But segfaults were just undefined behaviour surfacing after invariants were violated. Can't even blame anything but my own saturated brain 😶‍🌫️.

-I think this was the point I started feeling like I was building something solid.

### Phase 4 (13/01/2026):

GC was... Less of a nightmare than I'd thought. Or maybe I just finally got some good sleep yesterday.

-The main realisation in this stage was that the heap is a graph. Objects in the heap may point to other objects. For example, if the heap object is an array, some Value elements in it may be ValueType::OBJECT which point to another array, which in turn have elements that point to other objects, and so on. I hope that immediately smells like recursion to you, whoever's reading this. Therefore, **the mark phase is inherently recursive** (or graph-traversal based). Garbage collection is a DFS/BFS graph traversal.

-There **must** be strict separation between the mark phase and the sweep phase. If you mark during sweeping or sweep during marking... it's GGs bro. Garbage collection bugs like this rarely show up now. They slowly make you bleed more and more, until one day your VM can't function anymore.

-This was probably the phase that required the most discipline so far. As I mentioned earlier, garbage collection bugs are pernicious little fucktwats. They're like opps that wait until you're at rock bottom to strike.

Caveats:

-Memory allotted to the heap object payload is freed during GC, but the heap slot is not removed. This is because I use heap index as operand stack object reference. Slot reuse will be implemented in later phases.

-Garbage collection runs every time the number of objects on the heap exceeds 50. But what if the program actually needs more than 50 objects? The VM will end up running GC for every allotment after the 50th allotment. GC is costly: O(heapsize) complexity. A better system will be implemented in later phases.


### Phase 5 (I don't even remember the date bro not gonna lie)

I don't remember the date because I forgot to update this file when I completed that phase 💀. You can probably imagine, dear reader, how fucking DONE with this shit I was at that point. Well, anyway, I'm back now. I think.

This phase is where the project started going from 'glorified switch statement which gobbles up hardcoded bytecode and calls it a good day's work' to 'lowkey a serious language runtime with which you can compile and execute a program'.

The pipeline I decided on was:<br><br>
```
 _________________________________________
|                                         |
|               Source Code               |
|                    ↓                    |
|       Lexer (characters → tokens)       |
|                    ↓                    |
|          Parser (tokens → AST)          |
|                    ↓                    |
|        Compiler (AST → bytecode)        |
|                    ↓                    |
|         VM (bytecode → execution)       |
|_________________________________________|
```
(that box I made around the pipeline architecture took me longer than I expected to make it :/)

This phase was pretty big, so it's best to split it into parts:
#### 5.1: Lexer (Characters → Tokens)
Source code is NOT processed as words or whitespace-separated units, contrary to common belief. Instead, the program is a stream of characters. The lexer converts this into a stream of tokens.
<br>Each struct Token has a type (enum class TokenType in my code), lexeme (which is basically the original word in the program before it was converted to a token), and a line number.
<br>Example: let x = 5 processed to → LET, IDENTIFIER(x), EQUAL, INTEGER(5), SEMICOLON.

<br>-The biggest insight in one line: 
'The lexer does not know what a variable or expression is. It only knows what characters belong together.'

<br><br>Some things that had me lowkey tweaking out in the library:
<br>-Off-by-one errors when advancing the index. THIS was the biggest one, and DAMN was it annoying. It's so deceptively easy to lose synchronicity in your VM. If I had just blindly forged on, it would've come back to bite me in my (well-muscled) butt in later stages. Luckily, my special brand of OCD does NOT let me move on, however much I just wanna die. (Google usually gives you the national helpline when you say shit like that, but my fellow systems people know exactly what I'm talking about)
<br>-peek() assertions failing at end-of-input.
<br>-Double incrementing index.
<br>-Misunderstanding when to advance vs when to inspect.

<br><br>Key lesson: 
Most lexer bugs are pointer movement bugs. Stupid thing slips through your hands faster than a ping-pog ball.

#### Stage 5.2 — Parser (Tokens → AST)
**The big conceptual shift.**
<br>The compiler is a lot dumber than I thought. Most of the logical work is done by the Parser. Just tokens is not enough, the compiler needs structure. That structure is thr Abstract Syntax Tree, which is constructed by the Parser.

<br><br>expression → equality
<br>equality   → comparison (== | != comparison)*
<br>comparison → term (> < >= <= term)*
<br>term       → factor (+ | - factor)*
<br>factor     → unary (* / % unary)*
<br>unary      → (! | -) unary | primary

<br><br>This shit was probably the most mind-blowing concept in the whole VM so far, so bear with me while I fangirl a bit. It's SO fucking neat how the recursion structure enforces precedence. When I first learnt this, I actually closed my laptop and just stared off into space, enjoying the insane dopamine rush from learning it. Some poor guy probably wondered why a random buffoon was giving him a thousand-yard stare in the library lol
Some other interesting things:
<br>-Reassigning left in Binary Expressions (to support chaining).
<br>-Wrapping everything in Expr (basically for the compiler to be able to hit Expr->compile(). Damn, is the compiler a lazy piece of shit. It's like Gilderoy Lockhart from Harry Potter-it has all the good rep despite all of its work having been done by others.)

<br><br>The AST is the last human-readable representation.

#### Stage 5.3 — AST → Bytecode Compilation
Compilation is just a tree walk. A postorder traversal of the AST, if you will.
<br>For example, for binary expressions:

<br>left->compile()
<br>right->compile()
<br>emit opcode

<br><br>This naturally produces stack-based bytecode:

<br><br>PUSH 1
<br>PUSH 2
<br>ADD

<br><br>-The AST itself defines the evaluation order.
<br>-Absolutely NO explicit VM opstack management in the compiler. The compiler doesn't even know that the stack exists, it just emits instructions in the logical order already determined by the AST.

<br><br>Some annoying stuff:
<br>-My caffeine-abused ass didn't realise I had to map TokenType → Opcode. So I spent an embarrassing amount of time having midlife crises at 19 while my compiler emitted TokenType enumerations in the bytecode instead of Opcode ones. I screamed into my pillow (metaphorically, of course) when I realised.
<br>-Grouping expressions literally do nothing. They just forward compilation. Like bruh, stop wasting oxygen 🙄

#### Stage 5.4 — Statements (Expressions Were Not Enough)

This was a considerable mental shift, if not one of the biggest ones:
Expressions compute values. Statements do things.
<br>A program is not a list of expresssions. It's a list of statements.

<br>Parser changed from 
<br>parseExpression()
<br>to
<br>parseProgram() → parseStatement().

<br><br>Statements are what we see when we zoom out. They're the smallest meaningful building block of a program.

#### Stage 5.5 — Locals and Variable Slots

Variables are not stored by name at runtime. 
Instead:

<br>x → slot 0
<br>y → slot 1

<br><br>Compiler maintains:
<br>unordered_map<string, int> varSlots
<br>int nextLocalSlot

<br>VM only sees numeric slots.

<br><br>Important realization: The compiler resolves names. The VM only handles indices.
<br>This separation simplifies runtime execution enormously, and was something I kept botching up: mixing compiler responsibilities with VM behaviour.

<br><br>Bugs encountered
<br>-Missing global callframe (turns out, you have to push a global callframe at the beginning of the program, which made a horrible amount of sense), which incidentally lead to out of bounds local access → segfaults.
<br>-SET_LOCAL popping stack twice
<br>-GET_LOCAL accessing empty locals array

This was where stack discipline became critical.

#### Stage 5.6 — Control Flow (if / while)

This introduced the first non-linear execution. Although you might realise, whoever's reading this, that we did face a similar model earlier. In the CALl/RET phase. The difference here is, the program continues 

<br>Concept DLC unlocked: Backpatching
<br>When compiling, the structure is:
<br>if (cond) {...}, or while(cond){...}.
<br><br>So just compile the condition, push it onto the stack, emit a JUMP_IF_FALSE Opcode, and set the index to which you should jump to. Simple enough, right? Well, except you have no idea where you should jump to during compilation. So emit a random placeholder index, compile the block, and then revisit it to change it. This is what all compilers do.

<br><br>Major sources of confusion/crashes:
<br>-IfStmt must include both the condition AND the Block to be executed if the condition is true.
<br>-Each IfStmt MUST deal with its own compilation. State cannot be globally maintained, because nested blocks can and will overwrite state and cause chaos.
<br>-Not incrementing IP in the VM
<br>-Stack not cleaned after conditions
<br>-Stack cleaned twice after conditions (yes, really. I have moments of profound idiocy, and that shouldn't be surprising.)
<br>-The compiler should NEVER perform arithmetic while emitting/patching jump index. At any point, it should only read the size of the bytecode so far, so be careful in where you do that.

<br><br>I think the biggest and most elusive change in this phase was psychological: Earlier phases felt concept-heavy but straightforward. Phase 5 required trusting abstractions:

<br><br>Lexer doesn’t understand meaning.
<br>Parser doesn’t execute.
<br>Compiler doesn’t store values.
<br>VM doesn’t know variable names.
<br>Each layer does one thing only.

<br><br>Once this separation clicked, development accelerated dramatically, although the code was the most boring and mechanical (but extensive-my code more than doubled in line count in this stage alone) so far. But mixing the layers' responsibilities was definitely what caused the most pain in this phase. You have to trust what you're building; trust the separation of responsibilities between the layers.

<br><br>But okay so basically (hear me out 💀) we ripped the source code apart character-by-character, made tokens from it, assembled those tokens into expressions, and wrapped expressions into statements back again. Kinda retarded, or so I thought when the big picture first became clear to me. But thinking about it like this helped:
<br><br>**Why This Architecture Exists (In Practice):**
<br>At first glance, the pipeline looks unnecessarily complicated. Why not interpret source code directly?
Because each stage removes one kind of complexity:

<br>Lexer removes character-level ambiguity.
<br>Parser removes syntactic ambiguity.
<br>Compiler removes structural complexity.
<br>VM executes only simple instructions.

<br>Each layer converts a complex problem into a simpler one for the next layer.
Without this separation, every part of the system would need to understand everything else.<br><br>

### Phase 6 : Functions, Callstack and Runtime Completion (24/03/2026)
(Incidentally, I just discovered that backticks make your text look like `this`, and be taken literally. Also, ALT + 26 enters an arrow. Siiiiick. Anyway...)<br>
If my roommates tell you that I laughed like a psychopath in the middle of the night when my code finally stopped doing black magic, don't believe them. If you do decide to believe them, in my defence, if you knew how many times I wanted to commit an exorcism on my laptop, you would, too.
<br>But on a more serious note, functions was definitely the most conceptually intricate phase of the VM. But the idea I started with that made things slightly easier was:<br>
`A function is just a bytecode entry point.`
<br>`A call is just jumping after saving context.`
<br>`A return is just restoring that context.`
<br>(Compiling a function involves the same trick as if/else, by the way: backpatching.)
<br>An important realization: Arguments are already on the stack. Return values are pushed onto the stack. No copying or magic needed.
<br><br>The core invariant that everything depended on:

`frameBase = stack.size() - argCount`

Locals are accessed as:

`stack[frameBase + slot]`

If this invariant breaks, the entire call stack collapses.

<br><br>Several additions had to be made to `Compiler`: 
<br>-mapping functions to their bytecode index
<br>-`stack scopeStack` and `function resolveLocal` to enforce lexical scoping: inner blocks can see their parent blocks, but not vice versa.
<br>-`stack nextLocalSlot` to preserve the independence of a function's local variables, and `int functionDepth` to ensure that a return call outside a function is prohibited.
<br><br>Control flow + functions together was the real boss fight. Because they exposed the nitty-gritty details of the decisions I made on the way. Scoping not bulletproof? Locals start leaking outside their blocks. Tiny issue in CALL/RET semantics? Violent segfault with a diabolical error message.
<br>When two or more concepts stress test your VM like that, you can be sure that past sins are gonna come knocking.

<br><br>Some bugs that had me ~~(speaking a language unknown to the human race)~~ dealing with them gracefully:
<br>-CALL using a frameBase that was off by one → "Thanks for playing!" 💀
<br>-If anything is above frameBase on the stack after function return, it's a return value. Parameter count has nothing to do with it, which I learnt by mulishly blundering through a couple of segfaults.
<br>-Using `scopeStack.back["name"]` instead of `resolveLocal(name)` → broken lexical scoping.

##### Testing & Instrumentation
This is where it hit me like a truck: I could literally write codes in this. It's a complete language runtime. So I did. I wrote diabolical test codes:
```Example
fun multiply(a, b) {
    let result = 0;
    let i = 0;
    while (i < b) {
        result = result + a;
        i = i + 1;
    }
    return result;
}

fun power(base, exp) {
    let result = 1;
    let i = 0;
    while (i < exp) {
        result = multiply(result, base);
        i = i + 1;
    }
    return result;
}

fun factorial(n) {
    if (n == 0) return 1;
    return multiply(n, factorial(n - 1));
}

fun fib(n) {
    if (n == 0) return 0;
    if (n == 1) return 1;
    return fib(n - 1) + fib(n - 2);
}

fun sumTo(n) {
    let sum = 0;
    let i = 1;
    while (i < n) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}

fun collatz(n) {
    let steps = 0;
    while (n != 1) {
        if (n == multiply(n / 2, 2)) {
            n = n / 2;
        } else {
            n = multiply(n, 3) + 1;
        }
        steps = steps + 1;
    }
    return steps;
}

print multiply(17, 13);
print power(2, 16);
print factorial(10);
print fib(20);
print sumTo(1000);
print collatz(27);
```
<br>Before I show you the output, adding instrumentation was one of the most satisfying parts of this whole project. Because it made the output look like:
```Output
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
<br>That stats output at the bottom understandably gave me an unholy amount of dopamine. When I ran the numbers, I had to take a step back: My VM was running more than **25 Million instructions Per Second**. Which is surprisingly competitive for a naive stack-based interpreter.
<br>At this point, the VM supports:

<br>- variables and lexical scoping
<br>- control flow (if / while)
<br>- functions and recursion
<br>- stack-based execution
<br>- bytecode interpretation
<br><br>This is no longer a toy interpreter.
<br>This is a complete, minimal programming language runtime.<br><br>

#### Phase 7: Compiler Optimizations, Compiler Errors, Arrays, and Project Completion
The previous phase felt like a boss fight. This one felt like a victory lap. 😈


Compiler errors turned out to be simpler than expected because the parser was already structured defensively:
```
if (match(...)) proceed;
else error;
```
All I had to do was replace placeholders with actual error reporting.

I implemented a `hadError` flag to allow the compiler to accumulate multiple errors in a single pass (instead of failing fast), similar to real-world compilers. Since tokens already carried line numbers, error reporting became precise with minimal extra work.

This reinforced a key lesson: good early design decisions compound. Small structural choices in the parser eliminated entire classes of future complexity.


A nice side-effect of recursive descent: I got `else if` chains for free.

Since `else` accepts any statement, and `if` itself is a statement, nesting naturally handles chained conditionals without any special-case logic.

This was one of those moments where the parsing strategy directly simplifies language semantics. Recursive descent is seriously cool. 🙏

I had to refactor `parsePrimary()` to support index chaining.

Previously, expressions returned immediately, which prevented constructs like:
- `arr[1][2][1]`
- `fn()[3]`

The fix was to delay returning and iteratively wrap the expression in `IndexExpr` nodes while `[` tokens are present.

This small structural change enabled arbitrarily deep indexing with no additional grammar rules.

By the way, arrays can have heterogenous elements in my programming language. So, an array can look like, `[69, "your mom", true, 6.7]`.Take that, statically-typed languages!


**Compiler Optimizations**

Constant folding and peephole optimization behaved exactly as they should—but not as I initially expected.

They had negligible impact on compute-heavy benchmarks (loops, recursion), because most values are only known at runtime. However, they reduced bytecode size in programs dominated by constant expressions.

The key realization:
```
Compile-time optimizations only help when the program exposes compile-time information.
```

In my case:
- arithmetic-heavy loops → no improvement
- constant-heavy programs → measurable reduction

So while local optimizations are working correctly, meaningful gains require global reasoning about values.


**Wow. I'm done.**

This project went from “build a VM” to a full language runtime:

- Lexer, parser, and AST
- Bytecode compiler
- Stack-based virtual machine
- Call stack + function support
- Mark-and-sweep garbage collector
- Error reporting with recovery
- Basic compiler optimizations
- Arrays with dynamic typing and indexing

More importantly, it forced me to think in terms of:
- execution models
- invariants
- memory safety
- tradeoffs between compile-time and runtime work

This isn’t just “code that runs”—it’s a system I can reason about, measure, and improve.

I’m calling the VM complete (for now). What a journey. (If you think I'm unrealistically calm about this in this log, don't worry, I screamed like a banshee in real life lol)

Now it's time to build something on top of it.
