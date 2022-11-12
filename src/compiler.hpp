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

    ICall = 0x50,
    IJmp = 0x51,
    ICjmp = 0x52,
    Iret = 0x53,

    INConst = 0x60,
    ISConst = 0x62,
    IFConst = 0x64,

    ILocal = 0x70,
    ILStore = 0x72,
    IBLocal = 0x74,
    IBLStore = 0x76,
    IUpvalue = 0x78,
    IUStore = 0x7a,

    IPush = 0x7c,
    IPop = 0x7e
};

class Lfunction
{
private:
    vector<lbyte> text;
    vector<lnumber> nconst;
    vector<const char *> sconst;

public:
    void push(lbyte b);
    size_t number(lnumber n);
    size_t cstr(const char *s);
    size_t clen();
    lbyte opcode(size_t index);
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
    lbyte bytes[3];

    Opcode(lbyte op, size_t idx);
    Opcode(lbyte op);
};

class Compiler
{
private:
    vector<Lfunction> funcs;
    vector<Lfunction> current;
    vector<Opcode> ops;
    vector<lbyte> vstack;

    Lfunction &cur();
    void emit(Opcode op);
    void ops_flush();
    void ops_push(Opcode op);
    size_t const_number(lnumber n);
    size_t const_string(const char *s);
    size_t vstack_nearest_nil();
    MetaMemory *varmem(Noderef lvalue);
    void newf();
    void endf();
    void compile_node(Noderef node);
    void compile_decl(Noderef node);
    void compile_block(Noderef node);
    void compile_primary(Noderef node);
    void compile_identifier(Noderef node);
    void compile_assignment(Noderef node);
    void compile_lvalue(Noderef node);
    void compile_varlist(Noderef node);
    void compile_explist(Noderef node, size_t vcount);
    void compile_lvalue_primary(Noderef node);
    void compile_exp(Noderef node);
    void compile_exp_e(Noderef node, size_t expect);
    lbyte translate_token(TokenKind kind, bool bin);

public:
    vector<Lfunction> compile(Ast ast);
};

#endif