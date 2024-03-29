#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

namespace luayed
{
    uint32_t adler32(const void *buf, size_t buflength);
};

#endif