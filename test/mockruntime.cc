#include "mockruntime.h"
#include "values.h"
#include <lstrep.h>
#include <map>
#include <set>
#include <cstring>

#define MOCK_RUNTIME_FAULT_POP 1
#define MOCK_RUNTIME_FAULT_IDX 2
#define MOCK_RUNTIME_FAULT_BACKIDX 3
#define MOCK_RUNTIME_FAULT_ARGIDX 4
#define MOCK_RUNTIME_FAULT_CONSTIDX 5

using namespace luayed;

void Intercept::enable()
{
    this->used = true;
}
void Intercept::enable(size_t arg1)
{
    this->used = true;
    this->arg1 = arg1;
    this->arg2 = 0;
}
void Intercept::enable(size_t arg1, size_t arg2)
{
    this->used = true;
    this->arg1 = arg1;
    this->arg2 = arg2;
}
bool Intercept::check(size_t arg1, size_t arg2)
{
    return this->used && this->arg1 == arg1 && this->arg2 == arg2;
}

size_t MockRuntime::back_stack(size_t idx)
{
    return this->stack.size() - idx;
}
LuaValue MockRuntime::create_nil()
{
    return lvnil();
}
LuaValue MockRuntime::create_boolean(bool b)
{
    return lvbool(b);
}
LuaValue MockRuntime::create_number(lnumber n)
{
    return lvnumber(n);
}
LuaValue MockRuntime::create_string(const char *s)
{
    return lvstring(s);
}
LuaValue MockRuntime::create_string(const char *s1, const char *s2)
{
    string str;
    str.append(s1);
    str.append(s2);
    return lvstring(str.c_str());
}
size_t MockRuntime::argcount()
{
    return this->args.size();
}
LuaValue MockRuntime::create_string(lnumber n)
{
    return lvstring(to_string(n).c_str());
}
size_t MockRuntime::length(const char *str)
{
    return strlen(str);
}
LuaValue MockRuntime::create_table()
{
    return lvtable();
}
LuaValue MockRuntime::create_luafn(fidx_t fidx)
{
    this->icp_luafn.enable(fidx);
    return lvnil();
}

LuaValue MockRuntime::stack_pop()
{
    if (!this->stack.size())
    {
        throw MOCK_RUNTIME_FAULT_POP;
    }
    LuaValue v = this->stack.back();
    this->stack.pop_back();
    return v;
}
void MockRuntime::stack_push(LuaValue value)
{
    this->stack.push_back(value);
}
LuaValue MockRuntime::stack_read(size_t idx)
{
    if (idx >= this->stack.size())
    {
        throw MOCK_RUNTIME_FAULT_IDX;
    }
    return this->stack[idx];
}
void MockRuntime::stack_write(size_t idx, LuaValue value)
{
    if (idx >= this->stack.size())
    {
        throw MOCK_RUNTIME_FAULT_IDX;
    }
    this->stack[idx] = value;
}
LuaValue MockRuntime::stack_back_read(size_t idx)
{
    if (!idx)
        throw MOCK_RUNTIME_FAULT_BACKIDX;
    idx = this->stack.size() - idx;
    if (idx >= this->stack.size())
    {
        throw MOCK_RUNTIME_FAULT_BACKIDX;
    }
    return this->stack[idx];
}
void MockRuntime::stack_back_write(size_t idx, LuaValue value)
{
    if (!idx)
        throw MOCK_RUNTIME_FAULT_BACKIDX;
    idx = this->stack.size() - idx;
    if (idx >= this->stack.size())
    {
        throw MOCK_RUNTIME_FAULT_BACKIDX;
    }
    this->stack[idx] = value;
}
void MockRuntime::hookpush()
{
    this->icp_hookpush.enable();
}
void MockRuntime::hookpop()
{
    this->icp_hookpop.enable();
}
LuaValue MockRuntime::arg(size_t idx)
{
    if (idx >= this->args.size())
    {
        throw MOCK_RUNTIME_FAULT_ARGIDX;
    }
    return this->args[idx];
}
void MockRuntime::add_upvalue(LuaValue value)
{
    this->upvalues.push_back(value);
    Hook hook;
    hook.is_detached = false;
    hook.val = lvnil();
    hook.original = &this->upvalues.back();
    this->upvalue_hooks.push_back(hook);
}
void MockRuntime::add_detached_upvalue(LuaValue value)
{
    Hook hook;
    hook.is_detached = true;
    hook.val = value;
    hook.original = nullptr;
    this->upvalue_hooks.push_back(hook);
}
Hook *MockRuntime::upvalue(size_t idx)
{
    if (idx >= this->upvalue_hooks.size())
        return nullptr;
    return &this->upvalue_hooks[idx];
}
LuaValue MockRuntime::rodata(size_t idx)
{
    if (idx >= this->constants.size())
    {
        throw MOCK_RUNTIME_FAULT_CONSTIDX;
    }
    return this->constants[idx];
}
lbyte *MockRuntime::text()
{
    return &this->instructions.front();
}
LuaValue MockRuntime::chunkname()
{
    return this->create_nil();
}
void MockRuntime::check_garbage_collection()
{
}
dbginfo_t *MockRuntime::dbgmd()
{
    return nullptr;
}
void MockRuntime::set_stack(vector<LuaValue> stack)
{
    this->stack = stack;
}
void MockRuntime::set_constants(vector<LuaValue> constants)
{
    this->constants = constants;
}
void MockRuntime::set_args(vector<LuaValue> args)
{
    this->args = args;
}
void MockRuntime::set_text(vector<Bytecode> text)
{
    this->instructions.clear();
    for (size_t i = 0; i < text.size(); i++)
    {
        Bytecode op = text[i];
        for (lbyte j = 0; j < op.count; j++)
        {
            this->instructions.push_back(op.bytes[j]);
        }
    }
}
void MockRuntime::store_ip(size_t ip)
{
    this->ip = ip;
}
size_t MockRuntime::load_ip()
{
    return this->ip;
}

vector<LuaValue> &MockRuntime::get_stack()
{
    return this->stack;
}
void MockRuntime::set_error(LuaValue value)
{
    this->error = value;
    this->has_error = true;
}
LuaValue MockRuntime::get_error()
{
    return this->error;
}
bool MockRuntime::error_metadata()
{
    return true;
}
void MockRuntime::error_metadata(bool md)
{
}
bool MockRuntime::error_raised()
{
    return this->has_error;
}
void MockRuntime::table_set(LuaValue t, LuaValue k, LuaValue v)
{
    vector<LuaValue> *mtp = t.as<vector<LuaValue> *>();
    for (size_t i = 0; i < mtp->size(); i += 2)
    {
        if ((*mtp)[i] == k)
        {
            (*mtp)[i + 1] = v;
            return;
        }
    }
    mtp->push_back(k);
    mtp->push_back(v);
}
LuaValue MockRuntime::table_get(LuaValue t, LuaValue k)
{
    vector<LuaValue> *mtp = t.as<vector<LuaValue> *>();
    for (size_t i = 0; i < mtp->size(); i += 2)
    {
        if ((*mtp)[i] == k)
        {
            return (*mtp)[i + 1];
        }
    }
    return lvnil();
}
LuaValue MockRuntime::table_global()
{
    return this->global;
}
size_t MockRuntime::extras()
{
    return 0;
}
void MockRuntime::extras(size_t count)
{
}