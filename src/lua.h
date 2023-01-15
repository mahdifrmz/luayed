#ifndef LUA_h
#define LUA_CPP

#include "runtime.h"
#include "interpreter.h"

#define LUA_COMPILE_RESULT_OK 0
#define LUA_COMPILE_RESULT_FAILED 1

#define LUA_TYPE_NIL 0
#define LUA_TYPE_BOOLEAN 1
#define LUA_TYPE_NUMBER 2
#define LUA_TYPE_STRING 3
#define LUA_TYPE_TABLE 4
#define LUA_TYPE_FUNCTION 5

class Lua;

typedef size_t (*LuaCppFunction)(Lua *);

class Lua
{
private:
    LuaRuntime runtime;
    Interpreter interpreter;

public:
    Lua();
    int compile(const char *lua_code, string &errors);
    void push_cppfn(LuaCppFunction cppfn);
    void push_string(const char *str);
    void push_nil();
    void call(size_t arg_count, size_t return_count);
    int kind();

    void set_global();

    void pop();
    size_t top();
    lnumber pop_number();
    bool pop_boolean();
    const char *pop_string();

    bool has_error();
    void push_error();
};

#endif