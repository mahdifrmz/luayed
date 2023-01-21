#ifndef GC_H
#define GC_H

#include "runtime.h"
#include "table.h"

class GarbageCollector : public IGarbageCollector
{
    gc_header *scanlifo;
    gc_header dummy;

    void scan(gc_header *obj);
    void scan(Hook *hook);
    void scan(Table *table);
    void scan(LuaValue *val);
    void scan(LuaFunction *fn);

public:
    GarbageCollector();
    void scan(LuaRuntime *rt);
    void reference(void *ptr);
};

#endif