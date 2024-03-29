#ifndef TEST_VALUES_H
#define TEST_VALUES_H

#include <luadef.h>
#include <set>
#include <map>
#include "values.h"

using namespace luayed;

std::set<string> strset;
std::vector<vector<LuaValue> *> tabset;

void luayed::tabset_detroy()
{
    while (tabset.size())
    {
        delete tabset.back();
        tabset.pop_back();
    }
}

LuaValue luayed::lvnil()
{
    LuaValue v;
    v.kind = LuaType::LVNil;
    return v;
}
LuaValue luayed::lvbool(bool b)
{
    LuaValue v;
    v.kind = LuaType::LVBool;
    v.data.b = b;
    return v;
}
LuaValue luayed::lvnumber(lnumber n)
{
    LuaValue v;
    v.kind = LuaType::LVNumber;
    v.data.n = n;
    return v;
}
LuaValue luayed::lvstring(const char *s)
{
    string str = s;
    auto it = strset.cend();
    if ((it = strset.find(s)) != strset.cend())
    {
        s = it->c_str();
    }
    else
    {
        strset.insert(str);
        s = strset.find(str)->c_str();
    }
    LuaValue v;
    v.kind = LuaType::LVString;
    v.data.ptr = (void *)s;
    return v;
}
LuaValue luayed::lvtable()
{
    LuaValue v;
    v.kind = LuaType::LVTable;
    vector<LuaValue> *obj = new vector<LuaValue>;
    v.data.ptr = obj;
    tabset.push_back(obj);
    return v;
}

#endif