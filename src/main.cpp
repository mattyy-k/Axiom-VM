#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <cassert>
#include <unordered_map>
#include <cctype>
#include <queue>
#include <chrono>
#include <map>
using namespace std;

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

enum class Opcode {
    PUSH,
    POP,
    NEG,
    NOT,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    CALL,
    RET,

    ALLOC_STRING,
    ALLOC_ARRAY,
    GET_INDEX,
    SET_INDEX,

    GET_LOCAL,
    SET_LOCAL,
    GET_GLOBAL,
    SET_GLOBAL,

    LESSTHAN,
    LESSEQUAL,
    GRTRTHAN,
    GRTREQUAL,
    EQUAL,
    NOTEQUAL,
    PRINT,
    JUMP_IF_FALSE,
    JUMP,

    HALT
};

string opcodeName(Opcode op) {
    switch(op) {
        case Opcode::PUSH: return "PUSH";
        case Opcode::POP: return "POP";
        case Opcode::NEG: return "NEG";
        case Opcode::NOT: return "NOT";
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::MOD: return "MOD";

        case Opcode::CALL: return "CALL";
        case Opcode::RET: return "RET";

        case Opcode::ALLOC_STRING: return "ALLOC_STRING";
        case Opcode::ALLOC_ARRAY: return "ALLOC_ARRAY";
        case Opcode::GET_INDEX: return "GET_INDEX";
        case Opcode::SET_INDEX: return "SET_INDEX";

        case Opcode::GET_LOCAL: return "GET_LOCAL";
        case Opcode::SET_LOCAL: return "SET_LOCAL";
        case Opcode::GET_GLOBAL: return "GET_GLOBAL";
        case Opcode::SET_GLOBAL: return "SET_GLOBAL";

        case Opcode::LESSTHAN: return "LT";
        case Opcode::LESSEQUAL: return "LE";
        case Opcode::GRTRTHAN: return "GT";
        case Opcode::GRTREQUAL: return "GE";
        case Opcode::EQUAL: return "EQ";
        case Opcode::NOTEQUAL: return "NE";

        case Opcode::PRINT: return "PRINT";
        case Opcode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case Opcode::JUMP: return "JUMP";

        case Opcode::HALT: return "HALT";
        default: return "UNKNOWN";
    }
}

int instructionLength(Opcode op) {
    switch(op) {
        case Opcode::PUSH: return 2;

        case Opcode::GET_LOCAL:
        case Opcode::SET_LOCAL:
        case Opcode::ALLOC_ARRAY:
        case Opcode::ALLOC_STRING:
        case Opcode::JUMP:
        case Opcode::JUMP_IF_FALSE:
            return 2;

        case Opcode::CALL:
            return 3;

        default:
            return 1;
    }
}
void disassemble(const vector<int>& bc) {
    int i = 0;

    while (i < bc.size()) {
        Opcode op = (Opcode)bc[i];
        cout << i << ": " << opcodeName(op);

        switch(op) {
            case Opcode::PUSH:
                cout << " " << bc[i + 1];
                break;

            case Opcode::GET_LOCAL:
            case Opcode::SET_LOCAL:
                cout << " slot=" << bc[i + 1];
                break;

            case Opcode::ALLOC_ARRAY:
                cout << " size=" << bc[i + 1];
                break;

            case Opcode::ALLOC_STRING:
                cout << " const_index=" << bc[i + 1];
                break;

            case Opcode::JUMP:
            case Opcode::JUMP_IF_FALSE:
                cout << " -> " << bc[i + 1];
                break;

            case Opcode::CALL:
                cout << " argc=" << bc[i + 1]
                     << " target=" << bc[i + 2];
                break;

            default:
                break;
        }
        cout << "\n";
        i += instructionLength(op);
    }
    cout << "\n";
}

enum class ValueType {
    INT,
    NIL,
    BOOL,
    OBJECT // for references to heap objects
};

enum class HeapType {
    STRING,
    ARRAY
};
int allocatedSinceLastGC = 0;

enum class TokenType {
    LEFT_PAREN, // (
    RIGHT_PAREN, // )
    LEFT_BRACE, // {
    RIGHT_BRACE, // }
    SEMICOLON, // ;
    LEFT_BRACKET, // [
    RIGHT_BRACKET, // ]

    COMMA, // ,
    PLUS, // +
    MINUS, // -
    MULTIPLY, // *
    DIVIDE, // /
    MOD, // %

    EQUAL, // =
    NOT, // !
    GRTR_THAN, // >
    LESS_THAN, // <
    GRTREQL, // >=
    LESSEQUAL, // <=
    NOTEQUAL, // !=
    EQUAL_EQUAL, // ==

    IDENTIFIER,
    INTEGER,
    STRING,

    TAB,
    CARR_RETURN,

    TRUE,
    FALSE,

    LET,
    FUN,
    IF,
    ELSE,
    RETURN,
    WHILE,
    NIL,
    PRINT,
    ENDOF
};

int instructionsExecuted = 0;
int maxStackDepth = 0;
int heapAllocations = 0;
int gcRuns = 0;
int objectsCollected = 0;
int constantFolds = 0;
int gcTime = 0;

bool hadError = false;

struct Token {
    TokenType type;
    string lexeme;
    int line;
};
class Compiler;

struct Expr {
    virtual ~Expr() = default; // to enable polymorphism and consistent treatment of all expressions.
    virtual void compile(Compiler &c){};
    virtual Expr* fold() { return this; }
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual void compile(Compiler& c){};
    virtual void fold(){};
};

class Compiler {
    public:
    vector<int> bytecode;
    vector<int> nextLocalSlot;
    int functionDepth;
    vector<unordered_map<string, int>> scopeStack;
    vector<string> stringConstants;

    Compiler() {
        nextLocalSlot.push_back(0);
        unordered_map<string, int> baseFrame;
        scopeStack.push_back(baseFrame);
        functionDepth = 0;
    } // pushing mainframe details
    
    Opcode opcodeFor(TokenType t) {
        switch (t) {
            case TokenType::PLUS: return Opcode::ADD;
            case TokenType::MINUS: return Opcode::SUB;
            case TokenType::MULTIPLY: return Opcode::MUL;
            case TokenType::DIVIDE: return Opcode::DIV;
            case TokenType::MOD: return Opcode::MOD;

            case TokenType::EQUAL_EQUAL: return Opcode::EQUAL;
            case TokenType::NOTEQUAL: return Opcode::NOTEQUAL;
            case TokenType::LESS_THAN: return Opcode::LESSTHAN;
            case TokenType::LESSEQUAL: return Opcode::LESSEQUAL;
            case TokenType::GRTR_THAN: return Opcode::GRTRTHAN;
            case TokenType::GRTREQL: return Opcode::GRTREQUAL;

            default: assert(false);
        }
    }
    int resolveLocal(const string& name) { // Search inner to outer
        for (int i = scopeStack.size() - 1; i >= 0; --i) {
            auto it = scopeStack[i].find(name);
            if (it != scopeStack[i].end()) {
                return it->second; // Found the slot index
            }
        }
        return -1; // Not found
    }
    // resolveLocal enforces the invariant that nested blocks can see the variables of inner blocks, but not vice versa.
    unordered_map<string, int> functions;

    vector<int> compileProgram(vector<Stmt*> stmts){
        for (auto stmt : stmts) {
            stmt->fold();
        } //constant folding

        for (auto stmt : stmts){
            stmt->compile(*this);
        } //compilation

        bytecode.push_back((int)Opcode::HALT);
        for (auto stmt : stmts){
            delete stmt;
        } //deleting the AST

        return bytecode;
    }
};
void compileError(const string& message, int line) {
    cerr << BOLD << RED << "[Compile Error]" << RESET
         << " Line " << line << ": " << message << "\n";
    hadError = true;
}
struct Instr { // for peephole optimizer
    Opcode op;
    vector<int> args;
    int size;
};
Instr readInstr(const vector<int>& bc, int i) { // instruction reader helper
    Opcode op = (Opcode)bc[i];
    Instr ins;
    ins.op = op;

    switch(op) {
        case Opcode::PUSH:
        case Opcode::GET_LOCAL:
        case Opcode::SET_LOCAL:
        case Opcode::ALLOC_ARRAY:
        case Opcode::ALLOC_STRING:
        case Opcode::JUMP:
        case Opcode::JUMP_IF_FALSE:
            ins.args = { bc[i+1] };
            ins.size = 2;
            break;

        case Opcode::CALL:
            ins.args = { bc[i+1], bc[i+2] };
            ins.size = 3;
            break;

        default:
            ins.size = 1;
    }
    return ins;
}
void emitInstr(vector<int>& out, const Instr& ins) { // helper to emit instructions
    out.push_back((int)ins.op);
    for (int x : ins.args) out.push_back(x);
}

vector<int> optimize(const vector<int> bc) { //actual peephole optimizer
    vector<int> out;

    for (int i = 0; i < bc.size();) {
        Instr a = readInstr(bc, i);

        if (i + a.size < bc.size()) {
            Instr b = readInstr(bc, i + a.size);

            if (a.op == Opcode::PUSH && b.op == Opcode::POP) { // Pattern: [PUSH X, POP] -> overall nothing; skip both
                i += a.size + b.size; // skip both
                continue;
            }
            if (a.op == Opcode::GET_LOCAL && b.op == Opcode::SET_LOCAL && a.args[0] == b.args[0]) { //Pattern: [GET_LOCAL X, SET_LOCAL X]
                i += a.size + b.size;
                continue;
            }
            if (a.op == Opcode::PUSH && b.op == Opcode::JUMP_IF_FALSE && a.args[0] == 1) { // Pattern: [PUSH true, JUMP_IF_FALSE X] -> skip both
                i += a.size + b.size;
                continue;
            }
            if (a.op == Opcode::JUMP && a.args[0] == i + 2) {
                i += 2;
                continue;
            }
        }
        emitInstr(out, a);
        i += a.size;
    }
    return out;
}

struct Value { //tagged union
    ValueType tag;
    union {
        int intVal;
        bool boolVal;
        int objectHandle;
        //later-> float, etc.
    } data;
    
    // static factory functions:
    
    static Value Int(int x){
        Value v;
        v.tag = ValueType::INT;
        v.data.intVal = x;
        return v;
    }

    static Value Bool(bool x){
        Value v;
        v.tag = ValueType::BOOL;
        v.data.boolVal = x;
        return v;
    }

    static Value Nil(){
        Value v;
        v.tag = ValueType::NIL;
        return v;
    }

    static Value Object(int handle){
        Value v;
        v.tag = ValueType::OBJECT;
        v.data.objectHandle = handle;
        return v;
    }
    Value(){ tag = ValueType::NIL; } // to prevent accidental default construction.
};

struct LiteralExpr : Expr {
    Value value;

    void compile (Compiler &c){
        c.bytecode.push_back((int)Opcode::PUSH);
        int val = value.tag == ValueType::INT ? value.data.intVal : (int)value.data.boolVal; 
        // (assuming the value is either an integer or a boolean)
        c.bytecode.push_back(val);
    }

    LiteralExpr(Value v) : value(v) {}
};
struct BinaryExpr : Expr {
    Expr* left;
    TokenType op;
    Expr* right;

    Expr* fold() override {
        left = left->fold();
        right = right->fold();

        auto l = dynamic_cast<LiteralExpr*>(left);
        auto r = dynamic_cast<LiteralExpr*>(right);

        if (op == TokenType::PLUS) {
            if (l && l->value.data.intVal == 0) {
                delete left;
                return right;
            }
            if (r && r->value.data.intVal == 0) {
                delete right;
                return left;
            }
        }
        if (op == TokenType::MULTIPLY) {
            if (l && l->value.data.intVal == 1) {
                delete left;
                return right;
            }
            if (r && r->value.data.intVal == 1) {
                delete right;
                return left;
            }
        }
        if (op == TokenType::MULTIPLY) {
            if ((l && l->value.data.intVal == 0) ||
                (r && r->value.data.intVal == 0)) {
                delete left;
                delete right;
                return new LiteralExpr(Value::Int(0));
            }
        }

        if (l && r) {
            constantFolds++;
            int lv = (l->value.tag == ValueType::BOOL) ? l->value.data.boolVal : l->value.data.intVal;
            int rv = (r->value.tag == ValueType::BOOL) ? r->value.data.boolVal : r->value.data.intVal;
            delete left;
            delete right;

            int result;

            switch(op) {
                case TokenType::PLUS: result = lv + rv; break;
                case TokenType::MINUS: result = lv - rv; break;
                case TokenType::MULTIPLY: result = lv * rv; break;
                case TokenType::DIVIDE:
                    if (rv == 0) return this; // don't fold unsafe
                    result = lv / rv;
                    break;
                case TokenType::MOD:
                    if (rv == 0) return this;
                    result = lv % rv;
                    break;
                case TokenType::EQUAL_EQUAL: return new LiteralExpr(Value::Bool(lv == rv));
                case TokenType::NOTEQUAL: return new LiteralExpr(Value::Bool(lv != rv));
                case TokenType::LESS_THAN: return new LiteralExpr(Value::Bool(lv < rv));
                case TokenType::LESSEQUAL: return new LiteralExpr(Value::Bool(lv <= rv));
                case TokenType::GRTR_THAN: return new LiteralExpr(Value::Bool(lv > rv));
                case TokenType::GRTREQL: return new LiteralExpr(Value::Bool(lv >= rv));

                default:
                    return this;
            }
        return new LiteralExpr(Value::Int(result));
        }
        return this;
    }

    void compile(Compiler& c) {
        left->compile(c);
        right->compile(c);
        c.bytecode.push_back((int)c.opcodeFor(op));
    }

    BinaryExpr(Expr* l, TokenType o, Expr* r) : left(l), right(r), op(o) {}
    ~BinaryExpr() {
        delete left;
        delete right;
    }
};

struct UnaryExpr : Expr {
    TokenType op;
    Expr* exp;

    Expr* fold() override {
        exp = exp->fold();

        auto l = dynamic_cast<LiteralExpr*>(exp);
        if (!l) return this;

        if (op == TokenType::MINUS) {
            int v = (l->value.tag == ValueType::BOOL) ? l->value.data.boolVal : l->value.data.intVal;
            delete exp;
            constantFolds++;
            return new LiteralExpr(Value::Int(-v));
        }
        if (op == TokenType::NOT) {
            bool v = l->value.data.boolVal;
            delete exp;
            constantFolds++;
            return new LiteralExpr(Value::Bool(!v));
        }
        return this;
    }

    void compile(Compiler &c){
        exp->compile(c);
        if (op == TokenType::MINUS) c.bytecode.push_back((int)Opcode::NEG);
        else if (op == TokenType::NOT) c.bytecode.push_back((int)Opcode::NOT);
    }

    UnaryExpr(TokenType o, Expr* e) : op(o), exp(e) {}
    ~UnaryExpr() {
        delete exp;
    }
};
struct GroupingExpr : Expr {
    Expr* exp;

    void compile(Compiler &c){
        exp->compile(c);
    }
    ~GroupingExpr() {
        delete exp;
    }

    GroupingExpr(Expr* e) : exp(e) {}
};
struct IdentifierExpr : Expr {
    string name;
    int line;

    IdentifierExpr(string n, int l) : name(n), line(l) {}
    void compile(Compiler& c){
        //auto it = c.scopeStack.back().find(name); we have to look through all scopes, not just the top one
        int slot = c.resolveLocal(name);
        if (slot == -1) { compileError("Undefined variable '" + name + "'", line); return; }
        
        c.bytecode.push_back((int)Opcode::GET_LOCAL);
        c.bytecode.push_back(slot);
    }
};
struct ErrorExpr : Expr {
    int line;
    ErrorExpr(int l) : line(l) {}
};
struct StringLiteralExpr : Expr {
    string value;
    StringLiteralExpr(string v) : value(v) {}

    void compile(Compiler& c) {
        int index = c.stringConstants.size();
        c.stringConstants.push_back(value);
        c.bytecode.push_back((int)Opcode::ALLOC_STRING);
        c.bytecode.push_back(index);
    }
};
struct ArrayLiteralExpr : Expr {
    vector<Expr*> elements;
    ArrayLiteralExpr(vector<Expr*> e) : elements(e) {}
    ~ArrayLiteralExpr() {
        for (auto e : elements) delete e;
    }

    Expr* fold() override {
        for (int i = 0; i < (int)elements.size(); i++)
            elements[i] = elements[i]->fold();
        return this;
    }

    void compile(Compiler& c) {
        c.bytecode.push_back((int)Opcode::ALLOC_ARRAY);
        c.bytecode.push_back(elements.size());
        for (int i = 0; i < (int)elements.size(); i++) {
            c.bytecode.push_back((int)Opcode::PUSH);
            c.bytecode.push_back(i);
            elements[i]->compile(c);
            c.bytecode.push_back((int)Opcode::SET_INDEX);
        }
    }
};
struct IndexExpr : Expr {
    Expr* array;
    Expr* index;
    IndexExpr(Expr* a, Expr* i) : array(a), index(i) {}
    ~IndexExpr() { delete array; delete index; }
    Expr* fold() override {
        array = array->fold();
        index = index->fold();
        return this;
    }
    void compile(Compiler& c) {
        array->compile(c);
        index->compile(c);
        c.bytecode.push_back((int)Opcode::GET_INDEX);
    }
};
struct IndexAssignStmt : Stmt {
    Expr* array;
    Expr* index;
    Expr* value;
    IndexAssignStmt(Expr* a, Expr* i, Expr* v) : array(a), index(i), value(v) {}
    ~IndexAssignStmt() { delete array; delete index; delete value; }
    void fold() {
        array = array->fold();
        index = index->fold();
        value = value->fold();
    }
    void compile(Compiler& c) {
        array->compile(c);
        index->compile(c);
        value->compile(c);
        c.bytecode.push_back((int)Opcode::SET_INDEX);
        c.bytecode.push_back((int)Opcode::POP);
    }
};

struct ExprStmt : Stmt { // compile an expression
    Expr* expr;
    ExprStmt(Expr* e) : expr(e) {}
    ~ExprStmt() {
        delete expr;
    }
    void fold() {
        expr = expr->fold();
    }
    void compile(Compiler& c){
        expr->compile(c);
        c.bytecode.push_back((int)Opcode::POP);
    }
};
struct VarDeclStmt : Stmt { // introduce a name and store a local value on the callstack
    string name;
    Expr* initializerExpr;
    int line;
    VarDeclStmt(string n, Expr* e, int l) : name(n), initializerExpr(e), line(l) {}
    ~VarDeclStmt() {
        delete initializerExpr;
    }

    void fold() {
        initializerExpr = initializerExpr->fold();
    }

    void compile(Compiler& c){
        initializerExpr->compile(c); // push compiled value onto the stack
        c.scopeStack.back()[name] = c.nextLocalSlot.back(); 

        c.bytecode.push_back((int)Opcode::SET_LOCAL); // push setlocal back into bytecode
        c.bytecode.push_back(c.nextLocalSlot.back());
        c.nextLocalSlot.back()++;
        c.bytecode.push_back((int)Opcode::POP); // pop compiled value from stack
    }
};
struct AssignmentStmt : Stmt { // modify an existing variable (variable must exist on the top of the callstack)
    string name;
    int line;
    Expr* valueExpr;
    AssignmentStmt(string n, Expr* e, int l) : name(n), valueExpr(e), line(l) {}
    ~AssignmentStmt() {
        delete valueExpr;
    }

    void fold() {
        valueExpr = valueExpr->fold();
    }

    void compile(Compiler& c){
        valueExpr->compile(c); //compile value
        c.bytecode.push_back((int)Opcode::SET_LOCAL); // emit set local
        int slot = c.resolveLocal(name);
        if (slot == -1) { cerr << "Undefined variable.\n"; return; }
        
        c.bytecode.push_back(slot); // emit index of variable in the locals vector
        c.bytecode.push_back((int)Opcode::POP);
    }
};
struct ErrorStmt : Stmt {
    int line;
    ErrorStmt(int l) : line(l) {
        assert(0 > 1);
    }
    void compile(Compiler& c){
        cerr << "Error Statement encountered.";
        return; // compile to nothing
    }
};
struct AssignmentExpr : Expr {
    string name;
    Expr* valueExpr;
    AssignmentExpr(string n, Expr* e) : name(n), valueExpr(e) {}

    void compile(Compiler& c) {
        valueExpr->compile(c);
        int slot = c.resolveLocal(name);
        if (slot == -1) { cerr << "Undefined variable in assignment: " << name << "\n"; return; }
        c.bytecode.push_back((int)Opcode::SET_LOCAL);
        c.bytecode.push_back(slot);
    }
};
struct PrintStmt : Stmt {
    Expr* expr;
    PrintStmt(Expr* e) : expr(e) {}
    ~PrintStmt() {
        delete expr;
    }

    void fold() {
        expr = expr->fold();
    }

    void compile(Compiler& c) {
        expr->compile(c);
        c.bytecode.push_back((int)Opcode::PRINT);
    }
};
struct BlockStmt : Stmt {
    vector<Stmt*> stmts;
    BlockStmt(vector<Stmt*> v) : stmts(v) {}
    ~BlockStmt() {
        for (auto i : stmts) {
            delete i;
        }
    }

    void fold() {
        for (auto stmt : stmts) {
            stmt->fold();
        }
    }

    void compile(Compiler& c){
        int savedSlot = c.nextLocalSlot.back();
        c.scopeStack.push_back({});

        for (auto stmt : stmts){
            stmt->compile(c);
        }

        c.scopeStack.pop_back();
        c.nextLocalSlot.back() = savedSlot;
    }
    void compile (Compiler& c, vector<string>& params) { // overloaded, exclusively for function declarations
        unordered_map<string, int> temp;
        c.nextLocalSlot.push_back(0);

        for (auto param : params) {
            temp[param] = c.nextLocalSlot.back()++;
        } // all function arguments mapped to slots in the locals array.
        c.scopeStack.push_back(temp);
        for (auto stmt : stmts){
            stmt->compile(c);
        }
        c.nextLocalSlot.pop_back();
        c.scopeStack.pop_back(); // pop scope
    }
};
struct IfStmt : Stmt {
    Expr* cond;
    Stmt* ifBlock;
    Stmt* elseBlock; // initialize to nullptr if there is no else block.

    IfStmt(Expr* e, Stmt* b, Stmt* s) : cond(e), ifBlock(b), elseBlock(s) {}
    ~IfStmt() {
        delete cond;
        delete ifBlock;
        if (elseBlock) delete elseBlock;
    }

    void fold() {
        cond = cond->fold();
        ifBlock->fold();
        if (elseBlock) elseBlock->fold();
    }

    int emitJump(Opcode jumptype, Compiler& c){
        c.bytecode.push_back((int)jumptype);
        return c.bytecode.size();
    }
    void patchJump(int jumpInd, Compiler& c){
        c.bytecode[jumpInd] = c.bytecode.size();
    }
    void compile(Compiler& c){
        cond->compile(c);
        int jumpIndex = emitJump(Opcode::JUMP_IF_FALSE, c);
        c.bytecode.push_back(0); //temporary value
        ifBlock->compile(c);

        if (elseBlock) {
            int skipElse = emitJump(Opcode::JUMP, c);
            c.bytecode.push_back(0); // placeholder
            patchJump(jumpIndex, c); // if condition false jumps to here (start of else)
            elseBlock->compile(c);
            patchJump(skipElse, c); // if condition true jumps to here (past else)
        } else {
            patchJump(jumpIndex, c); // false jumps past the then block
        }
    }
};
struct WhileStmt : Stmt {
    Expr* cond;
    Stmt* block;
    WhileStmt(Expr* e, Stmt* b) : cond(e), block(b) {}

    void fold() {
        cond = cond->fold();
        block->fold();
    }

    int emitJump(Opcode jumptype, Compiler& c){
        c.bytecode.push_back((int)jumptype);
        return c.bytecode.size();
    }
    void emitJump(Compiler& c, Opcode jumptype, int ls){
        c.bytecode.push_back((int)Opcode::JUMP);
        c.bytecode.push_back(ls);
    }
    void patchJump(int jumpInd, Compiler& c){
        c.bytecode[jumpInd] = c.bytecode.size();
    }
    void compile(Compiler& c){
        int loopStart = c.bytecode.size();
        cond->compile(c);
        int jumpIndex = emitJump(Opcode::JUMP_IF_FALSE, c);
        c.bytecode.push_back(0); //temporary value
        block->compile(c);
        emitJump(c, Opcode::JUMP, loopStart);
        patchJump(jumpIndex, c);
    }
};
struct FunctionStmt : Stmt { // for function declaration
    string name;
    vector<string> params;
    Stmt* block;
    FunctionStmt(string n, vector<string> p, Stmt* b) : name(n), params(p), block(b) {}
    ~FunctionStmt() { 
        delete block; 
    }

    void fold() {
        block->fold();
    }

    int emitJump(Opcode jumptype, Compiler& c){
        c.bytecode.push_back((int)jumptype);
        return c.bytecode.size();
    }

    void compile(Compiler& c) {
        c.functionDepth++;
        int placeholder = emitJump(Opcode::JUMP, c);
        c.bytecode.push_back(0); // emit temporary placeholder
        c.functions[name] = c.bytecode.size(); // map function name to bytecode index

        auto callerScopes = c.scopeStack;
        auto callerSlots = c.nextLocalSlot;

        c.scopeStack.clear(); // necessary for enforcing function scope.

        BlockStmt* blck = dynamic_cast<BlockStmt*>(block);
        blck->compile(c, params);

        //Restore the caller's context
        c.scopeStack = callerScopes;
        c.nextLocalSlot = callerSlots;

        c.bytecode.push_back((int)Opcode::PUSH);

        c.bytecode.push_back(0); // default return value
        c.bytecode.push_back((int)Opcode::RET); // implicit return path (every function must end with a RET)
        //the previous two lines were to prevent functions who do not have an explicit return, from falling off and leaking into the surrounding scope.

        c.bytecode[placeholder] = c.bytecode.size(); // update the placeholder
        c.functionDepth--;
    }
};
struct ReturnStmt : Stmt { // for function return value
    Expr* value;
    ReturnStmt(Expr* v) : value(v)  {}
    ~ReturnStmt() {
        delete value;
    }

    void fold() {
        value = value->fold();
    }
    void compile(Compiler& c) {
        if (c.functionDepth == 0) {
            cerr << "Error: Return statement outside of function.\n";
            //compiler error
            return;
        }
        value->compile(c);
        c.bytecode.push_back((int)Opcode::RET);
    }
};
struct CallExpr : Expr { // for function call
    string callee;
    vector<Expr*> arguments;
    int line;
    CallExpr(string c, vector<Expr*> a, int l) : callee(c), arguments(a), line(l) {}
    ~CallExpr() {
        for (auto i : arguments) {
            delete i;
        }
    }
    void compile(Compiler& c) {
        for (auto exp : arguments) {
            exp->compile(c);
        } //push all of the function arguments to the stack before issuing CALL.
        c.bytecode.push_back((int)Opcode::CALL);
        c.bytecode.push_back(arguments.size()); // paramCount
        auto it = c.functions.find(callee);
        if (it != c.functions.end()) c.bytecode.push_back(it->second); // jump to
        //else compiler error
    }
};

class Lexer {
    public:
    int linenum;
    string source;

    Lexer(string src) : source(src), linenum(1) {}

    unordered_map<string, TokenType> keywords = {
        {"let", TokenType::LET},
        {"fun", TokenType::FUN},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"return", TokenType::RETURN},
        {"nil", TokenType::NIL},
        {"print", TokenType::PRINT}
    };
    
    vector<Token> tokens;
    int index = 0;

    // helper functions:
    char peek(){
        if (index + 1 < source.size()) { return source[index + 1]; }
        return '\0';
    }
    char advance(){
        if (index + 1 < source.size()) return source[index++];
        else { index++; return '\0'; }
        index++;
        return '\0';
    }
    bool match(char c){
        if (index + 1 < source.size()) {
            if (source[index + 1] == c){
                index++;
                return true;
            }
            else return false;
        }
        index++;
        return false;
    }

    vector<Token> scanTokens() {
        while (index < source.size()){
            if (source[index] == ' ') { index++; continue; }
            else if (source[index] == '\n') { linenum++; index++; continue; }
            else if (source[index] == '\t') { 
                continue; // useless token #1
            }
            else if (source[index] == '\r') { 
                continue; // useless token #2
            }

            char c = source[index];
            string tem;

            if (c == '(') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '(';
                temp.type = TokenType::LEFT_PAREN;
                tokens.push_back(temp);
                index++;
            }
            else if (c == ')') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = ')';
                temp.type = TokenType::RIGHT_PAREN;
                tokens.push_back(temp);
                index++;
            }
            else if (c == ';') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = ';';
                temp.type = TokenType::SEMICOLON;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '{') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '{';
                temp.type = TokenType::LEFT_BRACE;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '}') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '}';
                temp.type = TokenType::RIGHT_BRACE;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '[') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '[';
                temp.type = TokenType::LEFT_BRACKET;
                tokens.push_back(temp);
                index++;
            }
            else if (c == ']') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = ']';
                temp.type = TokenType::RIGHT_BRACKET;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '+') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '+';
                temp.type = TokenType::PLUS;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '-') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '-';
                temp.type = TokenType::MINUS;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '*') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '*';
                temp.type = TokenType::MULTIPLY;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '/') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '/';
                temp.type = TokenType::DIVIDE;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '%') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = '%';
                temp.type = TokenType::MOD;
                tokens.push_back(temp);
                index++;
            }
            else if (c == ',') {
                Token temp;
                temp.line = linenum;
                temp.lexeme = ',';
                temp.type = TokenType::COMMA;
                tokens.push_back(temp);
                index++;
            }
            else if (c == '!'){
                if (match('=')) {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = "!=";
                    temp.type = TokenType::NOTEQUAL;
                    tokens.push_back(temp);
                    index++;
                }
                else {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = "!";
                    temp.type = TokenType::NOT;
                    tokens.push_back(temp);
                    index++;
                }
            }
            else if (c == '>'){
                if (match('=')) {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = ">=";
                    temp.type = TokenType::GRTREQL;
                    tokens.push_back(temp);
                    index++;
                }
                else {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = ">";
                    temp.type = TokenType::GRTR_THAN;
                    tokens.push_back(temp);
                    index++;
                }
            }
            else if (c == '<'){
                if (match('=')) {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = "<=";
                    temp.type = TokenType::LESSEQUAL;
                    tokens.push_back(temp);
                    index++;
                }
                else {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = "<";
                    temp.type = TokenType::LESS_THAN;
                    tokens.push_back(temp);
                    index++;
                }
            }
            else if (c == '='){
                if (match('=')) {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = "==";
                    temp.type = TokenType::EQUAL_EQUAL;
                    tokens.push_back(temp);
                    index++;
                }
                else {
                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = "=";
                    temp.type = TokenType::EQUAL;
                    tokens.push_back(temp);
                    index++;
                }
            }
            else if (c == '"') {
                int start = ++index; // skip opening quote
                while (index < source.size() && source[index] != '"') {
                    index++;
                }
                string str = source.substr(start, index - start);
                index++; // skip closing quote
                Token temp;
                temp.line = linenum;
                temp.lexeme = str;
                temp.type = TokenType::STRING;
                tokens.push_back(temp);
            }
            else {
                if (isalpha(c) || c == '_'){
                    int start = index;
                    while (index < source.size() && (isalnum(peek()) || peek() == '_')) { index++; }
                    index++;

                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = source.substr(start, index-start);
                    temp.type = TokenType::IDENTIFIER;

                    if (temp.lexeme == "true") {
                        temp.type = TokenType::TRUE;
                    } else if (temp.lexeme == "false") {
                        temp.type = TokenType::FALSE;
                    }

                    if (keywords.count(temp.lexeme)) temp.type = keywords[temp.lexeme];
                    tokens.push_back(temp);
                }
                else if (isdigit(c)){
                    int start = index;
                    while (index < source.size() && isdigit(peek())) index++;
                    index++;

                    Token temp;
                    temp.line = linenum;
                    temp.lexeme = source.substr(start, index-start);
                    temp.type = TokenType::INTEGER;
                    tokens.push_back(temp);
                }
                else {
                    cerr << "Unknown character on line " << linenum << ".\n";
                    index++;
                }
            }
        }
        Token temp;
        temp.line = linenum;
        temp.lexeme = "EOF";
        temp.type = TokenType::ENDOF;
        tokens.push_back(temp);

        return tokens;
    }
};

class Parser {
    public:
    vector<Token> tokens;
    int index;
    Parser(vector<Token> t) : tokens(t), index(0) {}

    // helper functions:
    Token peek (){ // look at the current token
        if (index < tokens.size()) return tokens[index];
        Token temp;
        temp.type = TokenType::ENDOF;
        return temp;
    }
    void advance(){ //move to the next token
        index++;
    }
    bool match(TokenType t){ //match and increment to the next token
        if (index < tokens.size() && tokens[index].type == t) { index++; return true; }
        return false;
    }
    bool check(TokenType t){ // boolean check current token without increment
        if (index < tokens.size() && tokens[index].type == t) return true;
        return false;
    }
    Token nextCheck(){ // check the next token
        if (index + 1 < tokens.size()) return tokens[index + 1];
        Token temp;
        temp.type = TokenType::ENDOF;
        return temp;
    }
    vector<Stmt*> parseProgram() {
        vector<Stmt*> stmts;
        while (peek().type != TokenType::ENDOF){
            stmts.push_back(parseStatement());
        }
        return stmts;
    }
    Stmt* parseStatement(){
        if (peek().type == TokenType::IDENTIFIER && nextCheck().type == TokenType::LEFT_BRACKET) {
            string name = peek().lexeme;
            int line = peek().line;
            advance(); // consume identifier
            advance(); // consume '['

            Expr* idx = parseExpression();
            if (!match(TokenType::RIGHT_BRACKET)) compileError("Expected ']'", line);
            if (!match(TokenType::EQUAL)) compileError("Expected '='", line);
            Expr* val = parseExpression();

            if (check(TokenType::SEMICOLON)) advance();
            else compileError("Expected ';'", line);
            return new IndexAssignStmt(new IdentifierExpr(name, line), idx, val);
        }
        if (match(TokenType::PRINT)) {
            Expr* value = parseExpression();
            if (check(TokenType::SEMICOLON)) advance();
            else compileError("Expected ';'", peek().line);
            return new PrintStmt(value);
            perror("");
        }
        if (peek().type == TokenType::IDENTIFIER) {
            if (nextCheck().type == TokenType::EQUAL){
                string temp = peek().lexeme;
                int line = peek().line;
                advance();
                advance();
                Stmt* stmt = new AssignmentStmt(temp, parseExpression(), line);
                if (peek().type == TokenType::SEMICOLON) advance();
                return stmt;
            }
            else {
                Stmt* stmt = new ExprStmt(parseExpression());
                if (peek().type == TokenType::SEMICOLON) advance();
                return stmt;
            }
        }
        else if (match(TokenType::LET)){
            if (peek().type == TokenType::IDENTIFIER){
                string temp = peek().lexeme;
                int line = peek().line;
                advance();

                if (peek().type == TokenType::EQUAL) advance();
                else compileError("Expected '='", line);

                Stmt* stmt = new VarDeclStmt(temp, parseExpression(), line);
                if (peek().type == TokenType::SEMICOLON) advance();
                else compileError("Expected ';'", line);
                return stmt;
            }
        }
        else if (match(TokenType::IF)){
            if (match(TokenType::LEFT_PAREN)){
                Expr* cond = parseExpression();
                if (!match(TokenType::RIGHT_PAREN)) {
                    compileError("Expected ')'", peek().line);
                }
                
                Stmt* block = parseStatement();
                Stmt* elseBlock = nullptr;
                if (match(TokenType::ELSE)) {
                    elseBlock = parseStatement();
                }
                Stmt* ifstmt = new IfStmt(cond, block, elseBlock);
                return ifstmt;
            }
            else compileError("Expected '('", peek().line);
        }
        else if (match(TokenType::FUN)){
            if (check(TokenType::IDENTIFIER)){
                string name = peek().lexeme;
                int line = peek().line;
                advance();

                if (!match(TokenType::LEFT_PAREN)) {
                    compileError("Expected '('", line);
                }
                vector<string> params;

                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        if (!check(TokenType::IDENTIFIER)) {
                            compileError("Expected parameter name", peek().line);
                            //compiler error
                        }
                        params.push_back(peek().lexeme);
                        advance();
                    } while (match(TokenType::COMMA));
                }
                if (!match(TokenType::RIGHT_PAREN)){
                    compileError("Expected ')' near function declaration", line);
                }
                Stmt* block = parseStatement();
                //cout << "Function compiled successfully.\n";
                return new FunctionStmt(name, params, block);
            }
            // else compiler error
        }
        else if (match(TokenType::RETURN)){
            Expr* val = parseExpression();
            if (!match(TokenType::SEMICOLON)){
                compileError("Expected ';'", peek().line);
            }
            return new ReturnStmt(val);
        }
        else if (match(TokenType::WHILE)){
            if (match(TokenType::LEFT_PAREN)){
                Expr* cond = parseExpression();
                if (!match(TokenType::RIGHT_PAREN)) {
                    compileError("Expected ';'", peek().line);
                }
                
                Stmt* block = parseStatement();
                Stmt* whstmt = new WhileStmt(cond, block);
                return whstmt;
            }
            else compileError("Expected '('", peek().line);
        }
        else if (match(TokenType::LEFT_BRACE)){
            vector<Stmt*> stmts;
            while (peek().type != TokenType::RIGHT_BRACE && peek().type != TokenType::ENDOF){
                stmts.push_back(parseStatement());
            }
            advance(); //consume right brace
            return new BlockStmt(stmts);
        }
        else{
            Stmt* stmt = new ExprStmt(parseExpression());
            if (peek().type == TokenType::SEMICOLON) advance();
            else compileError("Expected ';'", peek().line);
            
            return stmt;
        }
        compileError("Error: Unexpected token at statement", peek().line);
        advance();
        Stmt* error = new ErrorStmt(peek().line);
        return error;

    }

    Expr* parseExpression() {
    return parseAssignment(); // New top of the chain
    }

    Expr* parseAssignment() {
        Expr* expr = parseEquality(); // Fall through to math

        if (match(TokenType::EQUAL)) {
            Token equals = tokens[index - 1];
            Expr* value = parseAssignment(); // Right-associative

            if (IdentifierExpr* i = dynamic_cast<IdentifierExpr*>(expr)) {
                Expr* assign = new AssignmentExpr(i->name, value);
                delete i; // Free the old node!
                return assign;
            }
            cerr << "Invalid assignment target.\n";
        }

        return expr;
    }
    Expr* parseEquality(){
        Expr* left = parseComparison();
        while (check(TokenType::EQUAL_EQUAL) || check(TokenType::NOTEQUAL)){
            TokenType operand = peek().type;
            advance();
            Expr* right = parseComparison();

            left = new BinaryExpr(left, operand, right); // reassigning it to left to support chaining
        }
        return left;
    }
    Expr* parseComparison(){
        Expr* left = parseTerm();
        while (check(TokenType::GRTR_THAN) || check(TokenType::GRTREQL) || check(TokenType::LESS_THAN) || check(TokenType::LESSEQUAL)){
            TokenType operand = peek().type;
            advance();
            Expr* right = parseTerm();

            left = new BinaryExpr(left, operand, right); // reassigning it to left to support chains
        }
        return left;
    }
    Expr* parseTerm(){
        Expr* left = parseFactor();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)){
            TokenType operand = peek().type;
            advance();
            Expr* right = parseFactor();

            left = new BinaryExpr(left, operand, right); // again, reassigning it to left to support chaining
        }
        
        return left;
    }
    Expr* parseFactor(){
        Expr* left = parseUnary();
        while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE) || check(TokenType::MOD)){
            TokenType operand = peek().type;
            advance();
            Expr* right = parseUnary();

            left = new BinaryExpr(left, operand, right);
        }
        return left;
    }
    Expr* parseUnary(){
        if (peek().type == TokenType::MINUS || peek().type == TokenType::NOT){
            Token temp = peek();
            advance();
            Expr* expr = parseUnary();
            Expr* exp = new UnaryExpr(temp.type, expr);
            return exp;
        }
        else return parsePrimary();
    }
    Expr* parsePrimary() {
        Expr* expr = nullptr;

        if (match(TokenType::TRUE)) {
            expr = new LiteralExpr(Value::Bool(true));
        }
        else if (match(TokenType::FALSE)) {
            expr = new LiteralExpr(Value::Bool(false));
        }
        else if (peek().type == TokenType::INTEGER) {
            int val = stoi(peek().lexeme);
            advance();
            Value value;
            value.tag = ValueType::INT;
            value.data.intVal = val;
            expr = new LiteralExpr(value);
        }
        else if (peek().type == TokenType::STRING) {
            string str = peek().lexeme;
            advance();
            expr = new StringLiteralExpr(str);
        }
        else if (peek().type == TokenType::LEFT_BRACKET) {
            advance();
            vector<Expr*> elements;
            if (!check(TokenType::RIGHT_BRACKET)) {
                do {
                    elements.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            if (!match(TokenType::RIGHT_BRACKET))
                compileError("Expected ']' after array elements", peek().line);
            expr = new ArrayLiteralExpr(elements);
        }
        else if (peek().type == TokenType::IDENTIFIER) {
            if (nextCheck().type == TokenType::LEFT_PAREN) {
                string callee = peek().lexeme;
                int line = peek().line;
                vector<Expr*> arguments;
                advance();
                advance();
                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        arguments.push_back(parseExpression());
                    } while (match(TokenType::COMMA));
                }
                if (!match(TokenType::RIGHT_PAREN))
                    compileError("Expected ')' near function call", peek().line);
                expr = new CallExpr(callee, arguments, line);
            }
            else {
                Token temp = peek();
                advance();
                expr = new IdentifierExpr(temp.lexeme, temp.line);
            }
        }
        else if (peek().type == TokenType::LEFT_PAREN) {
            advance();
            Expr* inner = parseExpression();
            if (peek().type == TokenType::RIGHT_PAREN) advance();
            expr = new GroupingExpr(inner);
        }
        else {
            int line = peek().line;
            compileError("Expected expression on line", line);
            advance();
            return new ErrorExpr(line);
        }
        // index chaining — runs for any expression
        while (check(TokenType::LEFT_BRACKET)) {
            advance();
            Expr* idx = parseExpression();
            if (!match(TokenType::RIGHT_BRACKET))
                compileError("Expected ']'", peek().line);
            expr = new IndexExpr(expr, idx);
        }
        return expr;
    }
};

struct HeapObject {
    HeapType type;
    int size;
    string st;
    vector<Value> arr;
    bool marked; // has reference from root (false -> destroyed, true -> alive)
    bool free;

    static HeapObject String(string str){
        HeapObject ob;
        ob.type = HeapType::STRING;
        ob.st = str;
        ob.size = str.size();
        ob.marked = false;
        ob.free = false;
        return ob;
    }
    static HeapObject Array(int n){
        HeapObject ob;
        ob.type = HeapType::ARRAY;
        ob.arr = vector<Value>(n, Value::Nil());
        ob.size = n;
        ob.marked = false;
        ob.free = false;
        return ob;
    }
    private:
        HeapObject(){}
};

struct callFrame {
    int returnIP;
    int frameBase;
    int paramCount;
    vector<Value> locals;
    callFrame(int ip, int fb, int pc) : returnIP(ip), frameBase(fb), paramCount(pc) {}
};

struct VM {
    int ip;
    vector<Value> opst; // operand stack
    vector<callFrame> callst; //call stack

    vector<int> bc; //bytecode
    vector<string> constants; // constant pool (for now)

    vector<HeapObject> heap; //heap
    VM(){ 
        ip = 0;
        callst.push_back(callFrame(-1, 0, 0));
    }
};

queue<int> freedheap;
void markObject(int ob, VM &vm){
    assert(ob < vm.heap.size());
    assert(!vm.heap[ob].free);
    if (vm.heap[ob].marked) return;

    vm.heap[ob].marked = true;
    if (vm.heap[ob].type == HeapType::ARRAY){
        int n = vm.heap[ob].arr.size();
        for (int i = 0; i < n; i++){
            if (vm.heap[ob].arr[i].tag == ValueType::OBJECT) {
                assert(vm.heap[ob].arr[i].data.objectHandle < vm.heap.size());
                markObject(vm.heap[ob].arr[i].data.objectHandle, vm);
            }
        }
    }
}
void markRoots(VM &vm){
    int n = vm.opst.size();
    for (const auto& val : vm.opst) {
        if (val.tag == ValueType::OBJECT) markObject(val.data.objectHandle, vm);
    }
    for (const auto& x : vm.callst){
        for (const auto& v : x.locals){
            if (v.tag == ValueType::OBJECT) markObject(v.data.objectHandle, vm); // marking locals on the callstack.
        }
    }
}
void collectGarbage(VM &vm){
    // Mark Phase (recursive):
    markRoots(vm);
    // Sweep Phase:
    int n = vm.heap.size();
    for (int i = 0; i < n; i++){
        if (vm.heap[i].free) continue;
        if (!vm.heap[i].marked) {
            vm.heap[i].free = true;
            freedheap.push(i);
            objectsCollected++;

            if (vm.heap[i].type == HeapType::ARRAY) vm.heap[i].arr.erase(vm.heap[i].arr.begin(), vm.heap[i].arr.end());
            else vm.heap[i].st.erase();
        }
        else vm.heap[i].marked = false;
    }
    allocatedSinceLastGC = 0;
}
int opcodeCount[(int)Opcode::HALT + 1] = {0};

int main(int argc, char* argv[]) {
    VM vm;
    string filename = "program.vm"; // default
    bool showBytecode = false;
    bool showStats = false;
    bool showOpcodeCount = false;
    bool showCompileTime = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--bytecode" || arg == "-b") showBytecode = true;
        else if (arg == "--stats" || arg == "-s") showStats = true;
        else if (arg == "--opcodes" || arg == "-o") showOpcodeCount = true;
        else if (arg == "--compile" || arg == "-c") showCompileTime = true;
        else if (arg == "--all" || arg == "-a") {
            showBytecode = true;
            showStats = true;
            showOpcodeCount = true;
        }
        else filename = arg; // anything that isn't a flag is the filename
    }
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: could not open file '" << filename << "'\n";
        return 1;
    }
    string src(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    ); // take the entire program as input string.
    Lexer lexer(src);
    Compiler c;

    auto startCompile = std::chrono::high_resolution_clock::now();
    vector<Token> tokens = lexer.scanTokens();
    Parser parser(tokens);
    vector<Stmt*> stmts = parser.parseProgram();
    
    auto raw = c.compileProgram(stmts);
    vm.bc = optimize(raw);

    auto endCompile = std::chrono::high_resolution_clock::now();
    auto durationCompile = std::chrono::duration_cast<std::chrono::microseconds>(endCompile - startCompile);

    vm.constants = c.stringConstants; // transfer constant pool

    if (showBytecode) disassemble(vm.bc); // print bytecode for debugging purposes.
    if (hadError) {
        cerr << BOLD << RED << "Compilation failed. Aborting.\n" << RESET;
        return 1;
    }

    bool running = true;
    auto start = std::chrono::high_resolution_clock::now();
    while (running){
        instructionsExecuted++;
        assert(vm.ip >= 0 && vm.ip < vm.bc.size());
        Opcode oc = (Opcode) vm.bc[vm.ip];
        switch (oc){
            case Opcode::PUSH: {
                Value value = Value::Int(vm.bc[++vm.ip]);
                vm.opst.push_back(value);
                maxStackDepth = max(maxStackDepth, (int)vm.opst.size());

                //cout << "Pushed " << value.data.intVal << endl; // all such prints are for debugging purposes
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::POP:{
                assert(vm.opst.size() >= 1);
                vm.opst.pop_back();

                //cout << "Popped" << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::ADD:{
                assert(vm.opst.size() >= 2);
                Value op2 = vm.opst.back();
                assert(op2.tag == ValueType::INT || op2.tag == ValueType::BOOL); // as bool is implicitly convertible to int
                vm.opst.pop_back();

                Value op1 = vm.opst.back();
                assert(op1.tag == ValueType::INT || op1.tag == ValueType::BOOL);
                vm.opst.pop_back();
                op1.data.intVal = (op1.tag == ValueType::BOOL ? (int)op1.data.boolVal : op1.data.intVal);
                op2.data.intVal = (op2.tag == ValueType::BOOL ? (int)op2.data.boolVal : op2.data.intVal);
                vm.opst.push_back(Value::Int(op1.data.intVal+op2.data.intVal));

                //cout << "Added " << op1.data.intVal << " and " << op2.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::SUB:{
                assert(vm.opst.size() >= 2);
                Value op2 = vm.opst.back();
                assert(op2.tag == ValueType::INT || op2.tag == ValueType::BOOL);
                vm.opst.pop_back();

                Value op1 = vm.opst.back();
                assert(op1.tag == ValueType::INT || op1.tag == ValueType::BOOL);
                vm.opst.pop_back();

                op1.data.intVal = (op1.tag == ValueType::BOOL ? (int)op1.data.boolVal : op1.data.intVal);
                op2.data.intVal = (op2.tag == ValueType::BOOL ? (int)op2.data.boolVal : op2.data.intVal);
                vm.opst.push_back(Value::Int(op1.data.intVal-op2.data.intVal));

                //cout << "Subtracted " << op1.data.intVal << " and " << op2.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::MUL:{
                assert(vm.opst.size() >= 2);
                Value op2 = vm.opst.back();
                assert(op2.tag == ValueType::INT || op2.tag == ValueType::BOOL);
                vm.opst.pop_back();

                Value op1 = vm.opst.back();
                assert(op1.tag == ValueType::INT || op1.tag == ValueType::BOOL);
                vm.opst.pop_back();

                op1.data.intVal = (op1.tag == ValueType::BOOL ? (int)op1.data.boolVal : op1.data.intVal);
                op2.data.intVal = (op2.tag == ValueType::BOOL ? (int)op2.data.boolVal : op2.data.intVal);
                vm.opst.push_back(Value::Int(op1.data.intVal*op2.data.intVal));

                //cout << "Multiplied " << op1.data.intVal << " and " << op2.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::DIV:{
                assert(vm.opst.size() >= 2);
                Value op2 = vm.opst.back();
                assert(op2.tag == ValueType::INT || op2.tag == ValueType::BOOL);
                assert(op2.data.intVal != 0);
                vm.opst.pop_back();

                Value op1 = vm.opst.back();
                assert(op1.tag == ValueType::INT || op1.tag == ValueType::BOOL);
                vm.opst.pop_back();

                op1.data.intVal = (op1.tag == ValueType::BOOL ? (int)op1.data.boolVal : op1.data.intVal);
                op2.data.intVal = (op2.tag == ValueType::BOOL ? (int)op2.data.boolVal : op2.data.intVal);
                vm.opst.push_back(Value::Int(op1.data.intVal/op2.data.intVal));

                //cout << "Divided " << op1.data.intVal << " and " << op2.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::MOD:{
                assert(vm.opst.size() >= 2);
                Value op2 = vm.opst.back();
                assert((op2.tag == ValueType::INT || op2.tag == ValueType::BOOL));
                assert(op2.data.intVal != 0);
                vm.opst.pop_back();

                Value op1 = vm.opst.back();
                assert(op1.tag == ValueType::INT || op1.tag == ValueType::BOOL);
                vm.opst.pop_back();

                op1.data.intVal = (op1.tag == ValueType::BOOL ? (int)op1.data.boolVal : op1.data.intVal);
                op2.data.intVal = (op2.tag == ValueType::BOOL ? (int)op2.data.boolVal : op2.data.intVal);
                vm.opst.push_back(Value::Int(op1.data.intVal%op2.data.intVal));

                //cout << "Mod " << op1.data.intVal << " and " << op2.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::HALT:{
                running = false;
                opcodeCount[(int)oc]++;
                break;
            }
            case Opcode::CALL:{
                assert(vm.ip + 3 < vm.bc.size());
                int paramCount = vm.bc[vm.ip + 1];
                callFrame cf(vm.ip + 3, vm.opst.size() - paramCount, paramCount);
                cf.locals.resize(paramCount);
                for (int i = paramCount - 1; i > -1; i--) {
                    cf.locals[i] = vm.opst.back();
                    vm.opst.pop_back();
                } // local variable values copied to local slots
                vm.callst.push_back(cf);
                vm.ip = vm.bc[vm.ip + 2];

                //cout << "Called @ " << vm.ip << "\n";
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::ALLOC_STRING:{
                assert(vm.bc.size() > vm.ip + 1);
                int index = vm.bc[++vm.ip];

                string str = vm.constants[index]; //bytecode references strings in a constant pool, since it cannot pass strings on its own.
                int handle;

                if (!freedheap.empty()) {
                    handle = freedheap.front();
                    freedheap.pop();
                    vm.heap[handle] = HeapObject::String(str); // reuse the slot
                } else {
                    handle = vm.heap.size();
                    vm.heap.push_back(HeapObject::String(str)); // grow the heap
                }
                allocatedSinceLastGC++;

                if (allocatedSinceLastGC > 50) {
                    auto startGC = std::chrono::high_resolution_clock::now();
                    collectGarbage(vm);
                    auto endGC = std::chrono::high_resolution_clock::now();
                    auto durationGC = std::chrono::duration_cast<std::chrono::microseconds>(endGC - startGC);
                    gcTime += durationGC.count();
                    gcRuns++;
                }
                vm.opst.push_back(Value::Object(handle));
                maxStackDepth = max(maxStackDepth, (int)vm.opst.size());

                //cout << "Allocated string" << str << endl;
                opcodeCount[(int)oc]++;
                vm.ip++;
                continue;
            }
            case Opcode::ALLOC_ARRAY:{
                int n = vm.bc[++vm.ip];
                int handle;
                if (!freedheap.empty()) {
                    handle = freedheap.front();
                    freedheap.pop();
                    vm.heap[handle] = HeapObject::Array(n);
                } else {
                    handle = vm.heap.size();
                    vm.heap.push_back(HeapObject::Array(n));
                }
                
                allocatedSinceLastGC++;
                if (allocatedSinceLastGC > 50) {
                    auto startGC = std::chrono::high_resolution_clock::now();
                    collectGarbage(vm);
                    auto endGC = std::chrono::high_resolution_clock::now();
                    auto durationGC = std::chrono::duration_cast<std::chrono::microseconds>(endGC - startGC);
                    gcTime += durationGC.count();
                    gcRuns++;
                }
                vm.opst.push_back(Value::Object(handle));
                maxStackDepth = max(maxStackDepth, (int)vm.opst.size());

                //cout << "Allocated array\n";
                opcodeCount[(int)oc]++;
                vm.ip++;
                continue;
            }
            case Opcode::GET_INDEX:{
                Value n = vm.opst.back();
                assert(n.tag == ValueType::INT);
                vm.opst.pop_back();
                
                Value ref = vm.opst.back();
                assert(ref.tag == ValueType::OBJECT);
                vm.opst.pop_back();
                assert(n.data.intVal < vm.heap[ref.data.objectHandle].arr.size() && n.data.intVal >= 0 
                && vm.heap[ref.data.objectHandle].type == HeapType::ARRAY);

                Value fetch = vm.heap[ref.data.objectHandle].arr[n.data.intVal];
                vm.opst.push_back(fetch);
                maxStackDepth = max(maxStackDepth, (int)vm.opst.size());

                //cout << "Got heap value at index " << n.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::SET_INDEX:{
                Value value = vm.opst.back();
                vm.opst.pop_back();

                Value index = vm.opst.back();
                assert(index.tag == ValueType::INT);
                vm.opst.pop_back();

                Value ref = vm.opst.back();
                assert(ref.tag == ValueType::OBJECT);
                assert(index.data.intVal < vm.heap[ref.data.objectHandle].arr.size() && index.data.intVal >= 0 
                && vm.heap[ref.data.objectHandle].type == HeapType::ARRAY);
                //vm.opst.pop_back();

                vm.heap[ref.data.objectHandle].arr[index.data.intVal] = value;

                //cout << "Set heap value at index " << index.data.intVal << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::GRTRTHAN:{
                Value right = vm.opst.back();
                vm.opst.pop_back();

                Value left = vm.opst.back();
                vm.opst.pop_back();

                bool ret = left.data.intVal > right.data.intVal;
                vm.opst.push_back(Value::Bool(ret));

                //cout << "Compared > and pushed boolean " << ret << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::GRTREQUAL:{
                Value right = vm.opst.back();
                vm.opst.pop_back();

                Value left = vm.opst.back();
                vm.opst.pop_back();

                bool ret = left.data.intVal >= right.data.intVal;
                vm.opst.push_back(Value::Bool(ret));

                //cout << "compared >= and pushed boolean " << ret << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::EQUAL:{
                assert(vm.opst.size() > 1);
                Value right = vm.opst.back();
                vm.opst.pop_back();

                Value left = vm.opst.back();
                vm.opst.pop_back();

                bool ret = (left.data.intVal == right.data.intVal);
                vm.opst.push_back(Value::Bool(ret));

                //cout << "Compared == and pushed boolean " << ret << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::LESSTHAN:{
                Value right = vm.opst.back();
                vm.opst.pop_back();

                Value left = vm.opst.back();
                vm.opst.pop_back();

                bool ret = left.data.intVal < right.data.intVal;
                vm.opst.push_back(Value::Bool(ret));

                //cout << "Compared < and pushed boolean " << ret << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::LESSEQUAL:{
                Value right = vm.opst.back();
                vm.opst.pop_back();

                Value left = vm.opst.back();
                vm.opst.pop_back();

                bool ret = left.data.intVal <= right.data.intVal;
                vm.opst.push_back(Value::Bool(ret));

                //cout << "Compared <= and pushed boolean " << ret << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::NOTEQUAL:{
                Value right = vm.opst.back();
                vm.opst.pop_back();

                Value left = vm.opst.back();
                vm.opst.pop_back();

                bool ret = left.data.intVal != right.data.intVal;
                vm.opst.push_back(Value::Bool(ret));

                //cout << "Compared != and pushed boolean " <<  ret << endl;
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::GET_LOCAL: {
                int n = vm.bc[++vm.ip];
                if (!(n >= 0 && n < vm.callst.back().locals.size())) {
                    cerr << "Runtime Error\n";
                    running = false;
                    continue;
                }
                vm.opst.push_back(vm.callst.back().locals[n]);
                maxStackDepth = max(maxStackDepth, (int)vm.opst.size());

                //cout << "Pushed local @ " << n << "\n";
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::SET_LOCAL:{
                int n = vm.bc[++vm.ip];
                if (n < 0) {
                    cerr << "Runtime Error\n";
                    running = false;
                    continue;
                }
                Value val = vm.opst.back();
                //vm.opst.pop_back(); incorrect because we already emit POP after SET_LOCAL, so the VM ends up popping twice -> stack underflow
                callFrame &frame = vm.callst.back();
                
                if (n >= frame.locals.size()) frame.locals.resize(n + 1);
                frame.locals[n] = val;

                //cout << "Set local\n";
                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::NEG:{
                assert(vm.opst.back().tag == ValueType::INT);
                int v = vm.opst.back().data.intVal;
                vm.opst.pop_back();
                vm.opst.push_back(Value::Int(-v));

                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::NOT:{
                assert(vm.opst.back().tag == ValueType::BOOL);
                bool v = vm.opst.back().data.boolVal;
                vm.opst.pop_back();
                vm.opst.push_back(Value::Bool(!v));

                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::RET:{
                assert(!vm.callst.empty());
                callFrame temp = vm.callst.back();
                int base = temp.frameBase;
                int retIP = temp.returnIP;
                
                bool hasReturn = vm.opst.size() > base;
                if (hasReturn) {
                    Value returnValue = vm.opst.back(); // only initialise a value if it will correspond to a real runtime value.
                    vm.opst.resize(base);
                    vm.opst.push_back(returnValue);
                } else {
                    vm.opst.resize(base);
                }

                vm.callst.pop_back(); // call stack cleanup
                vm.ip = retIP;

                //cout << "Returned to: " << retIP << "\n";
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::PRINT: {
                Value v = vm.opst.back();
                vm.opst.pop_back();

                switch (v.tag) {
                    case ValueType::INT:
                        cout << v.data.intVal << "\n";
                        break;
                    case ValueType::BOOL:
                        cout << (v.data.boolVal ? "true" : "false") << "\n";
                        break;
                    case ValueType::NIL:
                        cout << "nil\n";
                        break;
                    case ValueType::OBJECT: {
                        HeapObject& obj = vm.heap[v.data.objectHandle];
                        if (obj.type == HeapType::STRING) cout << obj.st << "\n";
                        else cout << "<array>\n";
                        break;
                    }
                    default:
                        cout << "<object>\n";
                }

                vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::JUMP_IF_FALSE: {
                assert(vm.opst.size() > 0);
                Value val = vm.opst.back();
                vm.opst.pop_back();
                assert(val.tag == ValueType::BOOL);

                int n = vm.bc[++vm.ip];
                if (!val.data.boolVal) vm.ip = n;
                else vm.ip++;
                opcodeCount[(int)oc]++;
                continue;
            }
            case Opcode::JUMP: {
                int n = vm.bc[++vm.ip];
                vm.ip = n;
                opcodeCount[(int)oc]++;
                continue;
            }
            default:{
                perror("Wrong opcode");
                running = false;
                instructionsExecuted--;
                break;
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    if (showCompileTime) cout << "\n---------------------------------\n| Compile time: " << durationCompile.count() << " microseconds |\n---------------------------------\n\n";

    if (showStats) {
        cout << "\n$>--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--<$\n" << instructionsExecuted << 
        " instruction(s) executed in " << duration.count() << " microseconds.\n";
        cout << "Number of garbage collections: " << gcRuns << "\n";
        cout << "Number of objects collected: " << objectsCollected << "\n";
        cout << "Total GC Time: " << gcTime << " microseconds\n";
        cout << "Maximum stack depth: " << maxStackDepth << "\n";
        cout << "Constant folds: " << constantFolds << "\n";
        cout << "$>--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--<$" << "\n\n";
    }

    if (showOpcodeCount) {
        cout << "##############\n";
        for (int i = 0; i < 29; i++) {
            cout << opcodeName((Opcode)i) << ": " << opcodeCount[i] << "\n";
        }
        cout << "##############\n";
    }
}
