#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "ast.hpp"

typedef unsigned char lbyte;
typedef double lnumber;

using namespace ast;

enum Instruction
{
    IAdd = 0x10,
    ISub = 0x11,
    IMult = 0x12,
    IFlrDiv = 0x13,
    IFltDiv = 0x14,
    IMod = 0x15,
    IPow = 0x16,
    IConcat = 0x17,
    IBOr = 0x18,
    IBAnd = 0x19,
    IBXor = 0x1a,
    ISHR = 0x1b,
    ISHL = 0x1c,

    ILength = 0x20,
    INegate = 0x21,
    INot = 0x22,
    IBNot = 0x23,

    IEq = 0x30,
    INe = 0x31,
    IGe = 0x32,
    IGt = 0x33,
    ILe = 0x34,
    ILt = 0x35,

    ITGet = 0x40,
    ITSet = 0x41,
    ITNew = 0x42,
    IGGet = 0x43,
    IGSet = 0x44,
    INil = 0x45,
    ITrue = 0x46,
    IFalse = 0x47,

    ITList = 0xc0,
    IRet = 0xc2,

    IFVargs = 0x54,

    ICall = 0xd0,
    IVargs = 0xd4,
    IJmp = 0xd6,
    ICjmp = 0xd8,

    INConst = 0xe0,
    ISConst = 0xe2,
    IFConst = 0xe4,

    ILocal = 0xf0,
    ILStore = 0xf2,
    IBLocal = 0xf4,
    IBLStore = 0xf6,
    IUpvalue = 0xf8,
    IUStore = 0xfa,

    IPush = 0xfc,
    IPop = 0xfe
};

class Lfunction
{
public:
    vector<lbyte> text;
    vector<lnumber> nconst;
    vector<const char *> sconst;
    vector<Lfunction> fconst;
    size_t parlen = 0;

    void push(lbyte b);
    size_t number(lnumber n);
    size_t func(Lfunction fn);
    size_t cstr(const char *s);
    size_t clen();
    string stringify();
    lbyte &operator[](size_t index);

    ~Lfunction();
    Lfunction &operator=(const Lfunction &other) = delete;
    Lfunction(const Lfunction &other) = delete;

    Lfunction(Lfunction &&other) = default;
    Lfunction &operator=(Lfunction &&other) = default;
    Lfunction() = default;
};

struct Opcode
{
    lbyte count;
    lbyte bytes[5];

    Opcode(lbyte op, size_t idx);
    Opcode(lbyte op, size_t idx1, size_t idx2);
    Opcode(lbyte op);
};

class Compiler
{
private:
    Lfunction func;
    vector<Opcode> ops;
    vector<lbyte> vstack;
    vector<size_t> breaks;
    bool method = false;

    size_t len();
    void emit(Opcode op);
    void ops_flush();
    void ops_push(Opcode op);
    size_t const_number(lnumber n);
    size_t const_string(const char *s);
    size_t vstack_nearest_nil();
    MetaMemory *varmem(Noderef lvalue);
    void compile_node(Noderef node);
    void compile_decl(Noderef node);
    void compile_ret(Noderef node);
    void compile_block(Noderef node);
    void compile_primary(Noderef node, size_t expect);
    void compile_table(Noderef node);
    void compile_name(Noderef node);
    void compile_function(Noderef node);
    void compile_identifier(Noderef node);
    void compile_call(Noderef node, size_t expect);
    void compile_methcall(Noderef node, size_t expect);
    void compile_assignment(Noderef node, bool attrib);
    void compile_lvalue(Noderef node);
    void compile_varlist(Noderef node, bool attrib);
    void compile_explist(Noderef node, size_t vcount);
    void compile_lvalue_primary(Noderef node);
    void compile_if(Noderef node);
    void compile_while(Noderef node);
    void compile_repeat(Noderef node);
    void compile_exp(Noderef node);
    void compile_logic(Noderef node);
    void compile_numeric_for(Noderef node);
    void compile_generic_for(Noderef node);
    void compile_break();
    void compile_exp_e(Noderef node, size_t expect);
    void loop_start();
    void loop_end();
    size_t arglist_count(Noderef arglist);
    lbyte translate_token(TokenKind kind, bool bin);

public:
    Lfunction compile(Ast ast);
};

#endif