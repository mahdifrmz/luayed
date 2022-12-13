#ifndef LUABIN_HPP
#define LUABIN_HPP

#include "luadef.hpp"

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

    IConst = 0xe0,
    IFConst = 0xe2,

    ILocal = 0xf0,
    ILStore = 0xf2,
    IBLocal = 0xf4,
    IBLStore = 0xf6,
    IUpvalue = 0xf8,
    IUStore = 0xfa,

    IPush = 0xfc,
    IPop = 0xfe
};

struct Upvalue
{
    fidx_t fidx;
    size_t offset;

    friend bool operator==(const Upvalue &l, const Upvalue &r);
};

struct Opcode
{
    lbyte count;
    lbyte bytes[5];

    Opcode(lbyte op, size_t idx);
    Opcode(lbyte op, size_t idx1, size_t idx2);
    Opcode(lbyte op);
};

class IGenerator
{
public:
    virtual void emit(Opcode opcode) = 0;
    virtual size_t len() = 0;
    virtual void seti(size_t idx, lbyte b) = 0;

    virtual size_t const_number(lnumber num) = 0;
    virtual size_t const_string(const char *str) = 0;

    virtual void pushf(fidx_t fidx) = 0;
    virtual void popf() = 0;

    virtual size_t upval(fidx_t fidx, size_t offset) = 0;

    virtual void meta_parcount(size_t parcount) = 0;
    virtual void meta_hookmax(size_t hookmax) = 0;
};

#endif