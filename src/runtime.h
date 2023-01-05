#ifndef RUNTIME_h
#define RUNTIME_h

#include <map>
#include <vector>
#include "luabin.h"
#include "virtuals.h"
#include "set.h"

#define lstrnull nullptr

class LuaValue;
class LuaRuntime;
class Frame;
class GenFunction;
typedef size_t (*LuaRTCppFunction)(void *);
struct LuaFunction;

class Lfunction
{
public:
    size_t codelen = 0;
    size_t uplen = 0;
    size_t rolen = 0;
    size_t hookmax = 0;
    size_t parcount = 0;
    fidx_t fidx = 0;

    lbyte *text();
    Upvalue *ups();
    LuaValue *rodata();

    ~Lfunction();
    Lfunction &operator=(const Lfunction &other) = delete;
    Lfunction(const Lfunction &other) = delete;

    Lfunction(Lfunction &&other) = default;
    Lfunction &operator=(Lfunction &&other) = default;
    Lfunction() = default;
};

class LuaTable
{
private:
    std::map<LuaValue, LuaValue> map;

public:
    void set(LuaValue key, LuaValue value);
    LuaType get(LuaValue key);
};

struct Frame
{
    size_t ss;
    size_t sp;
    size_t ip;
    LuaValue fn;
    LuaValue error;
    Frame *prev;
    size_t hookptr;

    // number of args this frame is supposed to return
    size_t exp_count;
    // number of extra args supplied to this function
    size_t vargs_count;
    // number of values returned to this function after an expect-free call
    size_t ret_count;

    bool is_Lua();
    Lfunction *bin();
    size_t parcount();
    size_t hookmax();
    LuaValue *stack();
    LuaValue *vargs();
    size_t vargcount();
    Hook **uptable();
    Hook **hooktable();
    size_t stack_address(size_t idx);
};
struct GenFunction
{
    GenFunction *prev;
    vector<lbyte> text;
    vector<LuaValue> rodata;
    vector<Upvalue> upvalues;
    fidx_t fidx;
    size_t parcount;
    size_t hookmax;
};
struct lstr_t
{
    hash_t hash;
    size_t len;

    const char *cstr()
    {
        return (const char *)(this + 1);
    }
};

typedef lstr_t *lstr_p;

class LuaRuntime : public IRuntime, public IAllocator
{
private:
    Set<lstr_p> lstrset;
    Frame *frame;
    IInterpreter *interpreter;
    vector<Lfunction *> functable;
    void *lua_interface = nullptr;

    void new_frame(size_t stack_size);
    void destroy_frame();
    void copy_values(Frame *fsrc, Frame *fdest, size_t count);
    void push_nils(Frame *fsrc, size_t count);

    LuaValue *stack();
    LuaValue *args();
    Hook **hooktable();
    Hook **uptable();
    Lfunction *bin();

    void *allocate(size_t size);
    void deallocate(void *ptr);

public:
    LuaRuntime(IInterpreter *interpreter);
    void set_lua_interface(void *lua_interface);

    LuaValue create_nil();
    LuaValue create_boolean(bool b);
    LuaValue create_number(lnumber n);
    LuaValue create_string(const char *s);
    LuaValue create_string(const char *s1, const char *s2);
    LuaValue create_table();
    Lfunction *create_binary(GenFunction *gfn);
    LuaValue create_cppfn(LuaRTCppFunction fn);
    LuaValue create_luafn(fidx_t fidx);

    void fncall(size_t argc, size_t retc);
    void fnret(size_t count);
    void set_error(LuaValue value);
    LuaValue get_error();
    fidx_t gen_fidx();
    Lfunction *bin(size_t fidx);

    LuaValue stack_pop();
    void stack_push(LuaValue value);
    LuaValue stack_read(size_t idx);
    void stack_write(size_t idx, LuaValue value);
    LuaValue stack_back_read(size_t idx);
    void stack_back_write(size_t idx, LuaValue value);
    void hookpush();
    void hookpop();
    LuaValue arg(size_t idx);
    size_t load_ip();
    void save_ip(size_t sp);
    Hook *upvalue(size_t idx);
    LuaValue rodata(size_t idx);
    lbyte *text();
    size_t stack_size();
};

struct LuaFunction
{
    void *fn;
    bool is_lua;

    Lfunction *binary();
    LuaRTCppFunction native();
};

#endif