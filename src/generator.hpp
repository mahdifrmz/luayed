#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include "runtime.hpp"
#include "luabin.hpp"

class LuaGenerator final : IGenerator
{
private:
    Lua *rt;
    GenFunction *gfn;
    size_t add_const(LuaValue value);

public:
    LuaGenerator(Lua *rt);

    void emit(Opcode opcode);
    size_t len();
    void seti(size_t idx, lbyte b);

    size_t const_number(lnumber num);
    size_t const_string(const char *str);

    void pushf(fidx_t fidx);
    void popf();

    size_t upval(fidx_t fidx, size_t offset);

    void meta_parcount(size_t parcount);
    void meta_hookmax(size_t hookmax);
};

#endif