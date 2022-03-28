#ifndef __UTILS_H__
#define __UTILS_H__

#include <linux/slab.h>

/* AUTO FREE */
#ifndef autofree
#define autofree __attribute__((cleanup(free_stack)))
inline void free_stack(void *pt);
#endif

/* SWAP */
#ifndef SWAP
#define SWAP(a, b, type) \
    do {                 \
        type *__x = (a); \
        type *__y = (b); \
        *__x ^= *__y;    \
        *__y ^= *__x;    \
        *__x ^= *__y;    \
    } while (0)
#endif

#endif
