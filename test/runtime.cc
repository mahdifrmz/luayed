#include <runtime.h>
#include "test.h"
#include "values.h"
#include <lstrep.h>
#include <cstring>

using namespace luayed;

void rt_assert(bool rsl, const char *message, size_t idx = 0)
{
    string mes = "runtime : ";
    mes.append(message);
    if (idx)
    {
        mes.append(" [");
        mes.append(std::to_string(idx));
        mes.append("]");
    }
    test_assert(rsl, mes.c_str());
}

void test_pushpop()
{
    const char *mes = "stack operations";
    LuaRuntime rt(nullptr);
    rt.set_lua_interface(&rt);
    vector<LuaValue> values = {
        rt.create_nil(),
        rt.create_number(200),
        rt.create_number(13),
        rt.create_nil(),
        rt.create_boolean(true),
    };
    rt_assert(rt.stack_size() == 0, mes, 1);
    pipe(&rt, values);
    rt_assert(rt.stack_size() == values.size(), mes, 2);
    vector<LuaValue> stack = drain(&rt);
    rt_assert(values == stack, mes, 3);
}

void test_create_values()
{
    const char *mes = "value creation";
    LuaRuntime rt(nullptr);

    LuaValue values[3] = {
        rt.create_nil(),
        rt.create_number(11),
        rt.create_boolean(false),
    };

    rt_assert(values[0].kind == LuaType::LVNil, mes, 1);
    rt_assert(values[1].kind == LuaType::LVNumber, mes, 2);
    rt_assert(values[1].data.n == 11, mes, 3);
    rt_assert(values[2].kind == LuaType::LVBool, mes, 4);
    rt_assert(values[2].data.b == false, mes, 5);
}

size_t lfcxx1(void *r)
{
    LuaRuntime *rt = (LuaRuntime *)r;
    vector<LuaValue> stack = drain(rt);
    for (size_t i = 0; i < stack.size(); i++)
    {
        stack[i].data.n = stack[i].data.n + 2;
    }
    stack.push_back(rt->create_boolean(true));
    stack.push_back(rt->create_boolean(false));
    pipe(rt, stack);
    return rt->stack_size() - 2;
}

void test_cxx_calls_cxx_extra()
{
    const char *mes = "CXX calls CXX returning extra args";
    LuaRuntime rt(nullptr);
    rt.set_lua_interface(&rt);
    rt.stack_push(rt.create_cppfn(lfcxx1));

    pipe(&rt,
         {
             rt.create_number(8),
             rt.create_number(15),
             rt.create_number(-44),
             rt.create_number(0),
             rt.create_number(600),
         });
    rt.call(5, 5);

    rt_assert(rt.stack_size() == 4, mes, 1);
    vector<LuaValue> stack = drain(&rt);
    vector<LuaValue> expected = {
        rt.create_number(-42),
        rt.create_number(2),
        rt.create_number(602),
        rt.create_boolean(true),
    };
    bool rsl = stack == expected;
    rt_assert(rsl, mes, 2);
    if (!rsl)
    {
        std::cerr << "stack:\n"
                  << stack
                  << "expected:\n"
                  << expected;
    }
}

size_t lfcxx2(void *r)
{
    LuaRuntime *rt = (LuaRuntime *)r;
    rt->stack_push(rt->create_boolean(true));
    return 1;
}

void test_cxx_calls_cxx_less()
{
    const char *mes = "CXX calls CXX returning less args";
    LuaRuntime rt(nullptr);
    rt.set_lua_interface(&rt);
    rt.stack_push(rt.create_number(14));
    rt.stack_push(rt.create_cppfn(lfcxx2));

    pipe(&rt,
         {
             rt.create_number(8),
             rt.create_number(15),
             rt.create_number(-44),
             rt.create_number(0),
             rt.create_number(600),
         });
    rt.call(5, 5);

    rt_assert(rt.stack_size() == 5, mes, 1);
    vector<LuaValue> stack = drain(&rt);
    vector<LuaValue> expected = {
        rt.create_number(14),
        rt.create_boolean(true),
        rt.create_nil(),
        rt.create_nil(),
        rt.create_nil(),
    };
    bool rsl = stack == expected;
    rt_assert(rsl, mes, 2);
    if (!rsl)
    {
        std::cerr << "stack:\n"
                  << stack
                  << "expected:\n"
                  << expected;
    }
}

void test_cxx_calls_cxx()
{
    test_cxx_calls_cxx_extra();
    test_cxx_calls_cxx_less();
}

size_t lfcxx3(void *r)
{
    LuaRuntime *rt = (LuaRuntime *)r;
    size_t count = rt->stack_pop().data.n;
    for (size_t i = 0; i < count; i++)
    {
        rt->stack_push(rt->create_number(1));
    }
    return count;
}

size_t lfcxx4(void *r)
{
    LuaRuntime *rt = (LuaRuntime *)r;
    vector<LuaValue> stack = drain(rt);
    for (size_t i = 0; i < stack.size(); i++)
        stack[i].data.n = -stack[i].data.n;
    pipe(rt, stack);
    return rt->stack_size();
}

size_t lfcxx5(void *r)
{
    LuaRuntime *rt = (LuaRuntime *)r;
    rt->stack_push(rt->create_boolean(true));
    rt->stack_push(rt->create_boolean(true));
    rt->stack_push(rt->create_cppfn(lfcxx4));
    rt->stack_push(rt->create_cppfn(lfcxx3));
    rt->stack_push(rt->create_number(3));
    rt->call(1, 0);
    rt->call(0, 0);
    return 0;
}

void test_lua_calls_cxx()
{
    LuaRuntime rt(nullptr);
    rt.set_test_mode(true);
    rt.set_lua_interface(&rt);
    rt.stack_push(rt.create_cppfn(lfcxx5));
    rt.call(0, 6);
    vector<LuaValue> stack = drain(&rt);
    vector<LuaValue> expected = {
        rt.create_number(-1),
        rt.create_number(-1),
        rt.create_number(-1),
        rt.create_nil(),
        rt.create_nil(),
    };
    bool rsl = stack == expected;
    rt_assert(rsl, "Lua calls CXX", 1);
    if (!rsl)
    {
        std::cerr << "stack:\n"
                  << stack
                  << "expected:\n"
                  << expected;
    }
}

void test_string_creation()
{
    LuaRuntime rt(nullptr);
    LuaValue v = rt.create_string("sample lua string");
    bool rsl = strcmp(v.as<const char *>(), "sample lua string") == 0;
    rt_assert(rsl, "string creation", 1);
}
void test_string_concatenation()
{
    LuaRuntime rt(nullptr);
    LuaValue v = rt.create_string("sample lua", " string");
    bool rsl = strcmp(v.as<const char *>(), "sample lua string") == 0;
    rt_assert(rsl, "string concatenation", 1);
}
void test_string_from_number()
{
    LuaRuntime rt(nullptr);
    LuaValue v = rt.create_string(8.11);
    bool rsl = strcmp(v.as<const char *>(), "8.11") == 0;
    rt_assert(rsl, "string from number", 1);
}
void test_string_interning()
{
    const char *str = "sample lua";
    LuaRuntime rt(nullptr);
    LuaValue v1 = rt.create_string(str);
    LuaValue v2 = rt.create_string(str);
    bool rsl = v1.as<const char *>() == v2.as<const char *>();
    rt_assert(rsl, "string interning", 1);
}
void test_string()
{
    test_string_creation();
    test_string_concatenation();
    test_string_interning();
    test_string_from_number();
}

void test_calls()
{
    test_cxx_calls_cxx();
    test_lua_calls_cxx();
}

void runtime_tests()
{
    test_pushpop();
    test_create_values();
    test_calls();
    test_string();
}