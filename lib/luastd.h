#ifndef LUASTD_H
#define LUASTD_H

#include "runtime.h"
#include "interpreter.h"
#include "lua.h"

extern const char liblua_code[];

namespace luayed
{
    namespace luastd
    {
        string luavalue_to_string(Lua *lua);

        size_t print(Lua *lua);
        size_t unpack(Lua *lua);
        size_t tostring(Lua *lua);
        size_t load(Lua *lua);
        size_t type(Lua *lua);
        size_t error(Lua *lua);
        size_t pcall(Lua *lua);

        void libinit(Lua *lua);
        void liblua_init(Lua *lua);
        void libcpp_init(Lua *lua);
    };
};

#endif